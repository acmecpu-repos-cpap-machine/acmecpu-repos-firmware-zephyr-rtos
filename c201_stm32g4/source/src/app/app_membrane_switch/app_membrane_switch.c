/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_membrane_switch);

#include "bsp_gpios.h"
#include "app_membrane_switch_priv.h"
#include "app_membrane_switch/app_membrane_switch.h"
#include "bsp_membrane_switch/bsp_membrane_switch.h"
#include "app_thread_configs.h"
#include "bsp_buzzer/bsp_buzzer.h"

struct switch_press_prop {
	uint32_t asserted_switch;
	BSP_MEMBR_SWITCH_PRESSED_TYPE press_type;
};

/* Static variables */
K_THREAD_STACK_DEFINE(m_switch_thread_stack, APP_THREAD_STACK_SIZE_MEMBR_SWITCH);
static struct k_thread m_switch_thread_data;
static k_tid_t m_switch_tid;
struct k_fifo m_switch_fifo;

static void membr_switch_handler_thread(void *p1, void *p2, void *p3) {
	struct switch_press_prop *swpp = NULL;
	while (1) {
		/* dequeue a switch press request from the fifo */
		swpp = k_fifo_get(&m_switch_fifo, K_FOREVER);
		if (swpp == NULL) {
			continue;
		}

		/* forward the request to the user interface module */
		bsp_buzzer_play_switch_pressed();

		/* free memory */
		free(swpp);
	}
}

static void membr_switch_cb_handler(uint32_t asserted_switch, BSP_MEMBR_SWITCH_PRESSED_TYPE press_type) {
	/* Allocate memory and set values */
	struct switch_press_prop *swpp = (struct switch_press_prop*) calloc(1, sizeof(struct switch_press_prop));
	if (swpp == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return;
	}

	swpp->asserted_switch = asserted_switch;
	swpp->press_type = press_type;

	/* Put data into the fifo */
	k_fifo_put(&m_switch_fifo, swpp);
}

/* Global functions */
int app_membrane_switch_init() {
	int ret = 0;

	ret = bsp_membrane_switch_init();
	if (ret != 0) {
		LOG_ERR("failed to initialize membrane switch");
		return ret;
	}

	/* Initialize the switch fifo */
	k_fifo_init(&m_switch_fifo);

	/* Start the membrane switch thread */
	m_switch_tid = k_thread_create(&m_switch_thread_data, m_switch_thread_stack,
			K_THREAD_STACK_SIZEOF(m_switch_thread_stack), membr_switch_handler_thread,
			NULL, NULL, NULL, APP_THREAD_PRIO_MEMBR_SWITCH, 0, K_NO_WAIT);

	/* Register a switch pressed callback with the bsp */
	bsp_membrane_switch_register_app_cb(membr_switch_cb_handler);

	return ret;
}
