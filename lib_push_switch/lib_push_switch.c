/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 29-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_push_switch);

#include "lib_push_switch.h"
#include "lib_push_switch_priv.h"

#define LIB_THREAD_STACK_SIZE_PUSH_SWITCH		(512) //1024
#define LIB_THREAD_PRIO_PUSH_SWITCH			1
#define LIB_THREAD_NAME_PUSH_SWITCH			"swch"

struct switch_press_prop {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	const struct device *dev;
//	const char* asserted_switch_label;
	uint32_t pin;
	LIB_PUSH_SWITCH_PRESSED_TYPE press_type;
};

/* Static variables */
static struct gpio_callback m_switch_cb_data[LIB_PUSH_SWITCH_MAX_NUM];
static uint8_t m_switch_cb_idx = 0;
static struct push_switch_data *m_ppsd;
static struct push_switch_data m_work;
static sys_slist_t callbacks;

/* Press detection thread variables */
K_THREAD_STACK_DEFINE(m_press_thread_stack, LIB_THREAD_STACK_SIZE_PUSH_SWITCH);
static struct k_thread m_press_thread_data;
static k_tid_t m_press_tid;
struct k_fifo m_switch_fifo;

/* Static functions */
static void membr_switch_handler_thread(void *p1, void *p2, void *p3)
{
	struct switch_press_prop *swpp = NULL;
	while (1) {
		/* dequeue a switch press request from the fifo */
		swpp = k_fifo_get(&m_switch_fifo, K_FOREVER);
		if (swpp == NULL) {
			continue;
		}

		/* fire callbacks to forward the request to the user interface module */
		bsp_membr_fire_callbacks(&callbacks, swpp->dev, swpp->pin, swpp->press_type);

		/* free memory */
		free(swpp);
	}
}

// static void membr_switch_insert_fifo(const char *asserted_switch_label, uint32_t pin, LIB_PUSH_SWITCH_PRESSED_TYPE press_type)
static void membr_switch_insert_fifo(struct push_switch_data *const switch_info)
{
	/* Allocate memory and set values */
	struct switch_press_prop *swpp = (struct switch_press_prop*) calloc(1,
			sizeof(struct switch_press_prop));
	if (swpp == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return;
	}

	// swpp->asserted_switch_label = asserted_switch_label;
	// swpp->pin = pin;
	// swpp->press_type = press_type;

	swpp->dev = switch_info->dev;
//	swpp->asserted_switch_label = switch_info->asserted_switch_label;
	swpp->pin = switch_info->asserted_switch_pin;
	swpp->press_type = switch_info->press_type;

	/* Put data into the fifo */
	k_fifo_put(&m_switch_fifo, swpp);
}

static void edge_press_interrupt_worker(struct k_work *work) {
	struct push_switch_data *const work_data = CONTAINER_OF(work, struct push_switch_data, interrupt_worker);

	int64_t start_time = k_uptime_get();
	do {
		int64_t temp = start_time;
		int64_t duration = k_uptime_delta(&temp);
		if (duration > LIB_PUSH_SWITCH_EDGE_NORMAL_PRESS_DURATION_MIN) {
			LOG_DBG("SWITCH_PRESSED_NORMAL_EDGE for %d switch detected successfully", work_data->asserted_switch_pin);

			// membr_switch_insert_fifo(work_data->asserted_switch_label, work_data->asserted_switch_pin, work_data->press_type);
			membr_switch_insert_fifo(work_data);

			break;
		}
		k_sleep(K_MSEC(10));
	} while (lib_push_switch_state_get(work_data->dev, work_data->asserted_switch_pin) == SWITCH_ASSERTED);
}

static void membr_sw_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("membr_sw_intr_cb: 0x%x\n", pins);
	LOG_DBG("membr_sw_intr_cb: %s\n", dev->name);

	struct push_switch_data *sw_pa = m_ppsd;//&m_press_action[0];
	// uint32_t asserted_switch = (pins & BSP_MEMBR_SWITCH_MASK_ALL_ASSERTED);

	/* Search for the index of the asserted switch */
	int pa_idx = -1;
	for (int i = 0; i < LIB_PUSH_SWITCH_MAX_NUM; i++) {
		if ((dev == sw_pa[i].dev) && (pins == sw_pa[i].asserted_switch_mask)) {
		// if (sw_pa[i].asserted_switch_mask == asserted_switch_mask) {
			pa_idx = i;
			break;
		}
	}

	if ((pa_idx >= 0) && (pa_idx < LIB_PUSH_SWITCH_MAX_NUM)) {
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

				if (duration > LIB_PUSH_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX) {
					/* if the duration is more than LIB_PUSH_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX
					 * we treat this as incorrect duration because it means that the
					 * last call ended up by not completing the press detection cycle
					 * so we reset the data here and restart the process */
					sw_pa[pa_idx].detection_continue = false;
					sw_pa[pa_idx].pressed_start = 0;
				} else if ((duration > LIB_PUSH_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MIN)) {
					/* press detection successful */
					LOG_DBG(
							"SWITCH_PRESSED_NORMAL for %d switch detected successfully",
							sw_pa[pa_idx].asserted_switch_pin);

					/* put the data into the switch fifo */
					// membr_switch_insert_fifo(
					// 		sw_pa[pa_idx].asserted_switch_label,
					// 		sw_pa[pa_idx].asserted_switch_pin,
					// 		sw_pa[pa_idx].press_type);
					membr_switch_insert_fifo(&sw_pa[pa_idx]);

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
			m_work.dev = sw_pa[pa_idx].dev;
			m_work.asserted_switch_mask = sw_pa[pa_idx].asserted_switch_mask;
//			m_work.asserted_switch_label = sw_pa[pa_idx].asserted_switch_label;
			m_work.asserted_switch_pin = sw_pa[pa_idx].asserted_switch_pin;
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

void lib_push_switch_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	membr_sw_intr_cb(dev, cb, pins);
}

void lib_push_switch_poll_handler(const struct device *dev, uint32_t switch_pin_mask, uint32_t switch_pin)
{
	membr_sw_intr_cb(dev, NULL, switch_pin_mask);
}

// int lib_push_switch_pin_configure(const struct device *dev, int pin, int flags,
// 		struct gpio_callback *gpio_cb_data, gpio_callback_handler_t handler)
int lib_push_switch_pin_configure(const struct device *dev, int pin, int flags, bool has_intr)
{
	int ret = 0;
	// const struct device *dev = device_get_binding(label);

	if (dev != NULL) {
		ret = gpio_pin_configure(dev, pin, flags);

		// if (!ret && (handler != NULL) && (gpio_cb_data != NULL)) {
		if (!ret && has_intr) {
			ret = gpio_pin_interrupt_configure(dev, pin, flags);
			if (ret != 0) {
				LOG_ERR("gpio_pin_interrupt_configure failed %s, %d", dev->name, pin);
				return ret;
			}
			gpio_init_callback(&m_switch_cb_data[m_switch_cb_idx], lib_push_switch_intr_cb, BIT(pin));
			ret = gpio_add_callback(dev, &m_switch_cb_data[m_switch_cb_idx]);
			if (ret) {
				LOG_ERR("gpio_add_callback failed");
				return ret;
			}
			m_switch_cb_idx++;
		}
	}
	return ret;
}

LIB_PUSH_SWITCH_PRESS_STATUS lib_push_switch_state_get(const struct device *dev, int pin)
{
	// const struct device *dev = device_get_binding(label);
	int ret = 0;

	ret = gpio_pin_get_raw(dev, pin);
	if (SWITCH_ASSERTED_PHY_LEVEL == 0) {
		if (ret == 0) {
			return SWITCH_ASSERTED;
		} else if (ret == 1) {
			return SWITCH_DEASSERTED;
		}
	} else if (SWITCH_ASSERTED_PHY_LEVEL == 1) {
		if (ret == 0) {
			return SWITCH_DEASSERTED;
		} else if (ret == 1) {
			return SWITCH_ASSERTED;
		}
	}

	return SWITCH_UNKNOWN;
}

int lib_push_switch_callback_add(struct lib_push_switch_callback *cb_data,
		lib_push_switch_callback_handler_t cb_handler, const struct device *dev, uint32_t pin)
{

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->dev = dev;
	cb_data->handler = cb_handler;
//	cb_data->switch_label = switch_label;
	cb_data->pin = pin;

	int ret = bsp_membr_manage_callback(&callbacks, cb_data, true);

	return ret;
}

int lib_push_switch_callback_remove(struct lib_push_switch_callback *cb_data,
		lib_push_switch_callback_handler_t cb_handler, const struct device *dev, uint32_t pin)
{

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");
	
	cb_data->dev = dev;
	cb_data->handler = cb_handler;
//	cb_data->switch_label = switch_label;
	cb_data->pin = pin;

	int ret = bsp_membr_manage_callback(&callbacks, cb_data, false);

	return ret;
}

int lib_push_switch_init(struct push_switch_data *ppsd)
{
	int ret = 0;

	/* copy the application switch data */
	if (ppsd != NULL)
		m_ppsd = ppsd;
	else {
		LOG_ERR("Switch data is NULL!");
		return -1;
	}

	/* Initialize the fifo */
	k_fifo_init(&m_switch_fifo);

	/* Prepare interrupt worker */
	k_work_init(&m_work.interrupt_worker, edge_press_interrupt_worker);

	/* Start the membrane switch thread */
	m_press_tid = k_thread_create(&m_press_thread_data, m_press_thread_stack,
					K_THREAD_STACK_SIZEOF(m_press_thread_stack), membr_switch_handler_thread,
					NULL, NULL, NULL, LIB_THREAD_PRIO_PUSH_SWITCH, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_press_tid, LIB_THREAD_NAME_PUSH_SWITCH);
#endif
	return ret;
}

