/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 31-Aug-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_mat_key);

#include "lib_matrix_keypad.h"
#include "lib_matrix_keypad_config.h"

#define MAX_ROWS	CONFIG_MATRIX_KEYPAD_MAX_ROWS
#define MAX_COLS	CONFIG_MATRIX_KEYPAD_MAX_COLS
#define KEY_PRESS_DURATION_MS_MIN	CONFIG_KEY_PRESS_DURATION_MS_MIN
#define KEY_PRESS_DURATION_MS_MAX	CONFIG_KEY_PRESS_DURATION_MS_MAX

#define HIGH	(1)
#define LOW		(0)

static struct libmk_key_data (*m_pkd)[MAX_COLS];
static libmk_callback_handler_t m_cb;

#if (CONFIG_MATRIX_KEYPAD_COL_OUTPUT)
static struct gpio_callback m_cb_data[MAX_ROWS];
#elif (CONFIG_MATRIX_KEYPAD_ROW_OUTPUT)
static struct gpio_callback m_cb_data[MAX_COLS];
#endif

/* Key scan thread variables */
K_THREAD_STACK_DEFINE(m_scan_thread_stack, LIB_MK_SCAN_THREAD_STACK_SIZE);
static struct k_thread m_scan_thread_data;
static k_tid_t m_scan_tid;

static void key_press_detect(struct libmk_key_data *kd, libmk_callback_handler_t cb)
{
	/* check if this is the 1st call */
	if ((!kd->det.cont_detection) && (kd->det.start_time == 0)) {
		kd->det.cont_detection = true;
		kd->det.start_time = k_uptime_get();
	} else {
		int64_t start = kd->det.start_time;
		int64_t duration = k_uptime_delta(&start);

		if (duration > KEY_PRESS_DURATION_MS_MAX) {
			/* if the duration is more than KEY_PRESS_DURATION_MS_MAX
			 * we treat this as incorrect duration because it means that the
			 * last call ended up by not completing the press detection cycle
			 * so we reset the data here and restart the process */
			kd->det.cont_detection = false;
			kd->det.start_time = 0;
		} else if ((duration > KEY_PRESS_DURATION_MS_MIN)) {
			/* press detection successful */
			LOG_DBG("%d key detected", kd->det.cont_detection);

			/* fire callback */
			if (cb != NULL)
				cb(kd->key_id);

			/* reset the data */
			kd->det.cont_detection = false;
			kd->det.start_time = 0;
		}
	}
}

static int key_state_get(struct gpio_dt_spec *key)
{
	int state = gpio_pin_get_raw(key->port, key->pin);
#if (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_HIGH)
	if (state == 1)			return KEY_ASSERTED;
	else if (state == 0)	return KEY_DEASSERTED;
	else					return KEY_DEASSERTED;
#elif (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_LOW)
	if (state == 0)			return KEY_ASSERTED;
	else if (state == 1)	return KEY_DEASSERTED;
	else					return KEY_DEASSERTED;
#endif
}

static void keypad_scan_thread(void *p1, void *p2, void *p3)
{
	struct libmk_key_data (*pkd)[MAX_COLS] = p1;
	libmk_callback_handler_t cb = p2;
	while (1) {
#if (CONFIG_MATRIX_KEYPAD_COL_OUTPUT)
		/* Drive one column and read each row then drive next column. Repeat this process */
		for (int col = 0; col < MAX_COLS; col++) {
			for (int row=0; row < MAX_ROWS; row++) {
				struct libmk_key_data *kd = &pkd[row][col];

				/* drive the output */
				if (kd->dev_col.port != NULL) {
#if (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_HIGH)
					gpio_pin_set_dt(&kd->dev_col, HIGH);
#elif (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_LOW)
					gpio_pin_set_dt(&kd->dev_col, LOW);
#endif
				}
				/* scan the input */
				if (kd->has_intr) {
					// TODO
				} else {
					if (kd->dev_row.port != NULL) {
						if (key_state_get(&kd->dev_row) == KEY_ASSERTED) {
							/* detect a valid key press and notify the application */
							key_press_detect(kd, cb);
						}
					}
				}
				/* delay */
				k_sleep(K_MSEC(LIB_MK_SCAN_DELAY_MS));
			}

			/* reset the output */
			struct libmk_key_data *kd_col = &pkd[0][col];
			if (kd_col->dev_col.port != NULL) {
#if (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_HIGH)
				gpio_pin_set_dt(&kd_col->dev_col, LOW);
#elif (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_LOW)
				gpio_pin_set_dt(&kd_col->dev_col, HIGH);
#endif
			}
		}

#elif (CONFIG_MATRIX_KEYPAD_ROW_OUTPUT)
#endif
	}
}

static void lib_mk_intr_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
//	membr_sw_intr_cb(dev, cb, pins);
}

static int lib_mk_gpio_configure(struct libmk_key_data pkd[MAX_ROWS][MAX_COLS])
{
	int ret = 0;
#if (CONFIG_MATRIX_KEYPAD_COL_OUTPUT)
	for (int row = 0; row < MAX_ROWS; row++) {
		bool row_init = false;
		for (int col = 0; col < MAX_COLS; col++) {
			struct libmk_key_data *kd = &pkd[row][col];
			/* reset detection variables */
			kd->det.cont_detection = false;
			kd->det.start_time = 0;

			/* configure column gpio */
			if (kd->dev_col.port != NULL) {
				ret = gpio_pin_configure_dt(&kd->dev_col, (GPIO_OUTPUT | GPIO_PUSH_PULL));
				if (ret != 0) {
					LOG_ERR("gpio_pin_configure_dt failed %s, %d", kd->dev_col.port->name, kd->dev_col.pin);
					return ret;
				}
#if (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_LOW)
				gpio_pin_set_dt(&kd->dev_col, HIGH);
#elif (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_HIGH)
				gpio_pin_set_dt(&kd->dev_col, LOW);
#endif
			}

			/* configure row gpio */
			if ((kd->dev_row.port != NULL) && (!row_init)) {
				ret = gpio_pin_configure_dt(&kd->dev_row, GPIO_INPUT);
				if (ret != 0) {
					LOG_ERR("gpio_pin_configure_dt failed %s, %d", kd->dev_row.port->name, kd->dev_row.pin);
					return ret;
				}
				if (kd->has_intr) {
#if (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_LOW)
					ret = gpio_pin_interrupt_configure_dt(&kd->dev_row, GPIO_INT_LEVEL_LOW);
#elif (CONFIG_MATRIX_KEYPAD_OUTPUT_DRIVE_HIGH)
					ret = gpio_pin_interrupt_configure_dt(&kd->dev_row, GPIO_INT_LEVEL_HIGH);
#endif
					if (ret != 0) {
						LOG_ERR("gpio_pin_interrupt_configure_dt failed %s, %d", kd->dev_row.port->name, kd->dev_row.pin);
						return ret;
					}

					gpio_init_callback(&m_cb_data[row], lib_mk_intr_cb, BIT(kd->dev_row.pin));
					ret = gpio_add_callback(kd->dev_row.port, &m_cb_data[row]);
					if (ret) {
						LOG_ERR("gpio_add_callback failed");
						return ret;
					}
				}

				row_init = true;	// row pin should be initialized once
			}
		}
	}
#elif (CONFIG_MATRIX_KEYPAD_ROW_OUTPUT)
	for (int col = 0; col < MAX_COLS; col++) {
		for (int row = 0; row < MAX_ROWS; row++) {

		}
	}
#endif
	return ret;
}

int lib_mk_callback_register(libmk_callback_handler_t cb)
{
	m_cb = cb;
	return 0;
}

int lib_mk_init(struct libmk_key_data pkd[MAX_ROWS][MAX_COLS])
{
	int ret = 0;

	/* copy the application switch data */
	if (pkd != NULL)
		m_pkd = pkd;
	else {
		LOG_ERR("Key data is NULL!");
		return -1;
	}

	/* configure the gpio pins */
	ret = lib_mk_gpio_configure(pkd);

	/* start keypad scan thread */
	m_scan_tid = k_thread_create(&m_scan_thread_data, m_scan_thread_stack,
					K_THREAD_STACK_SIZEOF(m_scan_thread_stack), keypad_scan_thread,
					m_pkd, m_cb, NULL, LIB_MK_SCAN_THREAD_PRIO, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_scan_tid, LIB_MK_SCAN_THREAD_NAME);
#endif

	return ret;
}
