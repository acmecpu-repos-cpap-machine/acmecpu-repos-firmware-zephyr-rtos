/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_membrane_switch);
#if (!CONFIG_BOARD_C204_CORE)
#include "bsp_gpios.h"
#endif
#include "bsp_membrane_switch/bsp_membrane_switch.h"
#include "bsp_membrane_switch_priv.h"
#include "app_thread_configs.h"

struct switch_press_prop {
	const char* asserted_switch_label;
	uint32_t pin;
	BSP_MEMBR_SWITCH_PRESSED_TYPE press_type;
};

/* Static variables */
static struct gpio_callback m_switch_cb_data[BSP_MEMBR_MAX_SWITCH];
static struct press_action_property m_press_action[BSP_MEMBR_MAX_SWITCH] = BSP_MEMBR_PRESS_ACTION_INIT_DATA;
static struct press_action_property m_work;
static sys_slist_t callbacks;

/* Press detection thread variables */
K_THREAD_STACK_DEFINE(m_press_thread_stack, APP_THREAD_STACK_SIZE_MEMBR_SWITCH);
static struct k_thread m_press_thread_data;
static k_tid_t m_press_tid;
struct k_fifo m_switch_fifo;

/* Static functions */
static void membr_switch_handler_thread(void *p1, void *p2, void *p3) {
	struct switch_press_prop *swpp = NULL;
	while (1) {
		/* dequeue a switch press request from the fifo */
		swpp = k_fifo_get(&m_switch_fifo, K_FOREVER);
		if (swpp == NULL) {
			continue;
		}

		/* fire callbacks to forward the request to the user interface module */
		bsp_membr_fire_callbacks(&callbacks, swpp->asserted_switch_label, swpp->pin, swpp->press_type);

		/* free memory */
		free(swpp);
	}
}

static void membr_switch_insert_fifo(const char *asserted_switch_label, uint32_t pin, BSP_MEMBR_SWITCH_PRESSED_TYPE press_type) {
	/* Allocate memory and set values */
	struct switch_press_prop *swpp = (struct switch_press_prop*) calloc(1,
			sizeof(struct switch_press_prop));
	if (swpp == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return;
	}

	swpp->asserted_switch_label = asserted_switch_label;
	swpp->pin = pin;
	swpp->press_type = press_type;

	/* Put data into the fifo */
	k_fifo_put(&m_switch_fifo, swpp);
}

static void edge_press_interrupt_worker(struct k_work *work) {
	struct press_action_property *const work_data = CONTAINER_OF(work,
			struct press_action_property, interrupt_worker);

	int64_t start_time = k_uptime_get();
	do {
		int64_t temp = start_time;
		int64_t duration = k_uptime_delta(&temp);
		if (duration > BSP_MEMBR_SWITCH_EDGE_NORMAL_PRESS_DURATION_MIN) {
			LOG_DBG("SWITCH_PRESSED_NORMAL_EDGE for %s switch detected successfully",
								(work_data->asserted_switch_label));

			membr_switch_insert_fifo(work_data->asserted_switch_label, work_data->asserted_switch_pin, work_data->press_type);

			break;
		}
		k_sleep(K_MSEC(10));
	} while (bsp_membrane_switch_state_get(work_data->asserted_switch_label, work_data->asserted_switch_pin) == SWITCH_ASSERTED);
}

#if (CONFIG_BOARD_C204_CORE)
static void membr_sw_intr_cb(const struct device *dev, struct gpio_callback *cb, const char* sw_label, uint32_t pins) {
#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
static void membr_sw_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
#else
static void membr_sw_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
#endif

	LOG_DBG("membr_sw_intr_cb: 0x%x\n", pins);
	LOG_DBG("membr_sw_intr_cb: %s\n", dev->name);

	struct press_action_property *sw_pa = &m_press_action[0];
	uint32_t asserted_switch = (pins & BSP_MEMBR_SWITCH_MASK_ALL_ASSERTED);

	/* Search for the index of the asserted switch */
	int pa_idx = -1;
	for (int i = 0; i < BSP_MEMBR_MAX_SWITCH; i++) {
#if (CONFIG_BOARD_C204_CORE)
		if (sw_label != NULL) {
			if ((!strcmp(sw_pa[i].asserted_switch_label, sw_label)) &&
					sw_pa[i].asserted_switch == asserted_switch) {
				pa_idx = i;
				break;
			}
		} else {
			if (sw_pa[i].asserted_switch == asserted_switch) {
				pa_idx = i;
				break;
			}
		}
#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
		if (sw_pa[i].asserted_switch == asserted_switch) {
			pa_idx = i;
			break;
		}
#endif	/* (CONFIG_BOARD_C204_CORE) */
	}

	if ((pa_idx >= 0) && (pa_idx < BSP_MEMBR_MAX_SWITCH)) {
		/* check the press type */
		switch (sw_pa[pa_idx].press_type) {
		case SWITCH_PRESSED_NONE: {
			LOG_DBG("SWITCH_PRESSED_NONE");
			break;
		}
		case SWITCH_PRESSED_NORMAL: {
			LOG_DBG("SWITCH_PRESSED_NORMAL");
			/* check if this is the 1st call */
			if ((!sw_pa[pa_idx].detection_continue)
					&& (sw_pa[pa_idx].pressed_start == 0)) {
				sw_pa[pa_idx].detection_continue = true;
				sw_pa[pa_idx].pressed_start = k_uptime_get();
			} else {
				int64_t start = sw_pa[pa_idx].pressed_start;
				int64_t duration = k_uptime_delta(&start);

				if (duration > BSP_MEMBR_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX) {
					/* if the duration is more than BSP_MEMBR_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX
					 * we treat this as incorrect duration because it means that the
					 * last call ended up by not completing the press detection cycle
					 * so we reset the data here and restart the process */
					sw_pa[pa_idx].detection_continue = false;
					sw_pa[pa_idx].pressed_start = 0;
				} else if ((duration > BSP_MEMBR_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MIN)) {
					/* press detection successful */
					LOG_DBG(
							"SWITCH_PRESSED_NORMAL for %s switch detected successfully",
							(sw_pa[pa_idx].asserted_switch_label));

					/* put the data into the switch fifo */
					membr_switch_insert_fifo(
							sw_pa[pa_idx].asserted_switch_label,
							sw_pa[pa_idx].asserted_switch_pin,
							sw_pa[pa_idx].press_type);

					/* reset the data */
					sw_pa[pa_idx].detection_continue = false;
					sw_pa[pa_idx].pressed_start = 0;
				}
			}

			break;
		}
		case SWITCH_PRESSED_LONG: {
			LOG_DBG("SWITCH_PRESSED_LONG");
			break;
		}
		case SWITCH_PRESSED_NORMAL_EDGE_LONG:
			LOG_DBG("SWITCH_PRESSED_NORMAL_EDGE_LONG");
			/* when control reaches here it falls through to the next case.
			 * Only EDGE trigger is detected here. To detect a long press,
			 * the application layer must poll on this switch to confirm
			 * whether the switch is in asserted position or not
			 * */
		case SWITCH_PRESSED_NORMAL_EDGE: {
			LOG_DBG("SWITCH_PRESSED_NORMAL_EDGE");
			/* here we start a worker thread to wait and validate
			 * a switch press event. If validated, the worker adds
			 * the switch data to the fifo.
			 * see function edge_press_interrupt_worker()
			 * */
			m_work.asserted_switch = sw_pa[pa_idx].asserted_switch;
			m_work.asserted_switch_label = sw_pa[pa_idx].asserted_switch_label;
			m_work.press_type = sw_pa[pa_idx].press_type;
			k_work_submit(&m_work.interrupt_worker);
			break;
		}
		}
	} else {
		/* Incorrect array index */
		LOG_ERR("Incorrect m_press_action array index, %d", pa_idx);
		return;
	}

	LOG_DBG("membr_sw_intr_cb exit");
}

#if CONFIG_BOARD_C204_CORE
static void c204_membr_sw_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	membr_sw_intr_cb(dev, cb, NULL, pins);
}

static int membr_sw_configure_pin(char *label, int pin, int flags,
		struct gpio_callback *gpio_cb_data, gpio_callback_handler_t handler) {
	int ret = 0;
	const struct device *dev = device_get_binding(label);

	if (dev != NULL) {
		ret |= gpio_pin_configure(dev, pin, flags);

		if ((handler != NULL) && (gpio_cb_data != NULL)) {
			ret = gpio_pin_interrupt_configure(dev, pin, flags);
			if (ret != 0) {
				LOG_ERR("gpio_pin_interrupt_configure failed %s, %d", dev->name, pin);
				return ret;
			}
			gpio_init_callback(gpio_cb_data, handler, BIT(pin));
			ret = gpio_add_callback(dev, gpio_cb_data);
			if (ret) {
				LOG_ERR("gpio_add_callback failed");
				return ret;
			}
		}
	}
	return 0;
}

BSP_MEMBR_SWITCH_PRESS_STATUS bsp_membrane_switch_state_get(const char *label, int pin) {
	const struct device *dev = device_get_binding(label);
	int ret = 0;

	ret = gpio_pin_get_raw(dev, pin);
	if (ret == 0) {
		return SWITCH_ASSERTED;
	} else if (ret == 1) {
		return SWITCH_DEASSERTED;
	}
	return SWITCH_UNKNOWN;
}

#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
static int membr_sw_configure_pin(const char *port,
		struct gpio_callback *gpio_cb_data, gpio_callback_handler_t handler) {
	const struct device *dev = device_get_binding(port);
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	int ret = 0;

	data->port = device_get_binding(cfg->port_name);
	if (data->port != NULL) {
		ret |= gpio_pin_configure(data->port, cfg->pin, cfg->flags);
		ret |= gpio_pin_interrupt_configure(data->port, cfg->pin, cfg->flags);
		if (ret != 0) {
			LOG_ERR("gpio_pin_configure failed %s, %d\n", data->port->name,
					cfg->pin);
			return ret;
		}

		if (gpio_cb_data != NULL) {
			gpio_init_callback(gpio_cb_data, handler, BIT(cfg->pin));
			ret |= gpio_add_callback(data->port, gpio_cb_data);
			if (ret) {
				LOG_ERR("Failed to configure UI Interrupt");
				return ret;
			}
		}
	}
	return 0;
}

BSP_MEMBR_SWITCH_PRESS_STATUS bsp_membrane_switch_state_get(const char *switch_name, int pin) {
	const struct device *dev = device_get_binding(switch_name);
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	int ret = 0;

	data->port = device_get_binding(cfg->port_name);
	if (data->port == NULL) {
		LOG_ERR("device not found %s", cfg->port_name);
		return SWITCH_UNKNOWN;
	}

//	uint32_t value;
//	ret = gpio_port_get(data->port, value);

	ret = gpio_pin_get_raw(data->port, cfg->pin);
	if (ret == 0) {
		return SWITCH_ASSERTED;
	} else if (ret == 1) {
		return SWITCH_DEASSERTED;
	}
	return SWITCH_UNKNOWN;
}

#endif	/* CONFIG_BOARD_C204_CORE */

int bsp_membrane_callback_add(struct bsp_membr_callback *cb_data,
		bsp_membr_callback_handler_t cb_handler, const char *switch_label, uint32_t pin) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->switch_label = switch_label;
	cb_data->pin = pin;

	int ret = bsp_membr_manage_callback(&callbacks, cb_data, true);

	return ret;
}

int bsp_membrane_callback_remove(struct bsp_membr_callback *cb_data,
		bsp_membr_callback_handler_t cb_handler, const char *switch_label, uint32_t pin) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->switch_label = switch_label;
	cb_data->pin = pin;

	int ret = bsp_membr_manage_callback(&callbacks, cb_data, false);

	return ret;
}

void bsp_membrane_switch_poll_handler(const struct device *dev, const char *sw_label, uint32_t pins) {
#if (CONFIG_BOARD_C204_CORE)
	membr_sw_intr_cb(dev, NULL, sw_label, pins);
#endif
}

int bsp_membrane_switch_init() {
	int ret = 0;

	/* Configure the membrane switch pins */
#if CONFIG_BOARD_C204_CORE
	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_ENTER,
									BSP_MEMBR_SWITCH_PIN_ENTER,
									BSP_MEMBR_SWITCH_FLAGS_ENTER,
									&m_switch_cb_data[0],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_BACK,
									BSP_MEMBR_SWITCH_PIN_BACK,
									BSP_MEMBR_SWITCH_FLAGS_BACK,
									&m_switch_cb_data[1],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_LEFT,
									BSP_MEMBR_SWITCH_PIN_LEFT,
									BSP_MEMBR_SWITCH_FLAGS_LEFT,
									&m_switch_cb_data[2],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_POWER,
									BSP_MEMBR_SWITCH_PIN_POWER,
									BSP_MEMBR_SWITCH_FLAGS_POWER,
									&m_switch_cb_data[3],
									c204_membr_sw_intr_cb);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_RIGHT,
									BSP_MEMBR_SWITCH_PIN_RIGHT,
									BSP_MEMBR_SWITCH_FLAGS_RIGHT,
									&m_switch_cb_data[4],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_DOWN,
									BSP_MEMBR_SWITCH_PIN_DOWN,
									BSP_MEMBR_SWITCH_FLAGS_DOWN,
									&m_switch_cb_data[5],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_UP,
									BSP_MEMBR_SWITCH_PIN_UP,
									BSP_MEMBR_SWITCH_FLAGS_UP,
									&m_switch_cb_data[6],
									NULL);

	ret |= membr_sw_configure_pin(	BSP_MEMBR_SWITCH_LABEL_HOME,
									BSP_MEMBR_SWITCH_PIN_HOME,
									BSP_MEMBR_SWITCH_FLAGS_HOME,
									&m_switch_cb_data[7],
									NULL);

#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_CHECK,
			&m_switch_cb_data[0], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_BACK,
			&m_switch_cb_data[1], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_LEFT,
			&m_switch_cb_data[2], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_RIGHT,
			&m_switch_cb_data[3], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_UP,
			&m_switch_cb_data[4], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_DOWN,
			&m_switch_cb_data[5], membr_sw_intr_cb);
	ret |= membr_sw_configure_pin(BSP_MEMBR_SWITCH_LABEL_POWER,
			&m_switch_cb_data[6], membr_sw_intr_cb);

#endif	/* CONFIG_BOARD_C204_CORE */

	/* Initialize the fifo */
	k_fifo_init(&m_switch_fifo);

	/* Prepare interrupt worker */
	k_work_init(&m_work.interrupt_worker, edge_press_interrupt_worker);

	/* Start the membrane switch thread */
	m_press_tid = k_thread_create(&m_press_thread_data, m_press_thread_stack,
					K_THREAD_STACK_SIZEOF(m_press_thread_stack), membr_switch_handler_thread,
					NULL, NULL, NULL, APP_THREAD_PRIO_MEMBR_SWITCH, 0, K_NO_WAIT);
	ret = k_thread_name_set(m_press_tid, APP_THREAD_NAME_MEMBR_SWITCH);

	return ret;
}

