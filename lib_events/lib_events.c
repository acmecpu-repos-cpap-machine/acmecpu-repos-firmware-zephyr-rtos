/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_events);

#include "lib_events.h"
#include "lib_events_config.h"

/* Event thread variables */
#define LIB_THREAD_NAME_EVENT_CAPTURE					"evnt"

K_THREAD_STACK_DEFINE(m_thread_stack, LIB_THREAD_STACK_SIZE_EVENT_CAPTURE);
static struct k_thread m_thread_data;
static k_tid_t m_tid;
static struct k_fifo m_event_fifo;

static sys_slist_t m_callbacks;

struct lib_event {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	LIB_EVENT_TYPE event;
};

/* Static functions */
static void lib_event_thread(void *p1, void *p2, void *p3) {
//	LIB_EVENT_TYPE *pevnt = NULL;
	struct lib_event *pevnt = NULL;
	while (1) {
		/* dequeue an event request from the fifo */
		pevnt = k_fifo_get(&m_event_fifo, K_FOREVER);
		if (pevnt == NULL) {
			continue;
		}

		/* fire callbacks to registered modules */
//		lib_events_fire_callbacks(&m_callbacks, *pevnt);
		lib_events_fire_callbacks(&m_callbacks, pevnt->event);

		/* free memory */
		free(pevnt);
	}
}

int lib_events_report_event(LIB_EVENT_TYPE reported_event) {
//	LIB_EVENT_TYPE *pevnt = (LIB_EVENT_TYPE *) calloc(1, sizeof(LIB_EVENT_TYPE));
	struct lib_event *pevnt = (struct lib_event*)calloc(1, sizeof(struct lib_event));
	if (pevnt == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return -1;
	}

//	*pevnt = reported_event;
//	pevnt->temp = reported_event+1;
	pevnt->event = reported_event;

	/* Put data into the fifo */
	k_fifo_put(&m_event_fifo, pevnt);

	return 0;
}

int lib_events_callback_add(struct lib_events_callback *cb_data,
		lib_events_callback_handler_t cb_handler, LIB_EVENT_TYPE event) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->event = event;

	int ret = lib_events_manage_callback(&m_callbacks, cb_data, true);

	return ret;
}

int lib_events_callback_remove(struct lib_events_callback *cb_data,
		lib_events_callback_handler_t cb_handler, LIB_EVENT_TYPE event) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->event = event;

	int ret = lib_events_manage_callback(&m_callbacks, cb_data, false);

	return ret;
}

int lib_events_init() {
	int ret = 0;

	/* Initialize the fifo */
	k_fifo_init(&m_event_fifo);

	/* Start the event thread */
	m_tid = k_thread_create(&m_thread_data, m_thread_stack,
					K_THREAD_STACK_SIZEOF(m_thread_stack), lib_event_thread,
					NULL, NULL, NULL, LIB_THREAD_PRIO_EVENT_CAPTURE, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_tid, LIB_THREAD_NAME_EVENT_CAPTURE);
#endif
	return ret;
}
