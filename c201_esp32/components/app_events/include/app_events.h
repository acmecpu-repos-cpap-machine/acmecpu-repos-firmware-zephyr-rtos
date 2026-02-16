/*
 * Copyright (c) 2021 Acme CPU
 *
 * app_events.h
 * Created on: 24-Jun-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_APP_EVENTS_INCLUDE_APP_EVENTS_H_
#define COMPONENTS_APP_EVENTS_INCLUDE_APP_EVENTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t m_event_q;


#include "gll.h"

#define APP_EVENTS_TASK_STACK		(1024*2)
#define APP_EVENTS_TASK_PRIO		(6)

/* System level events */
typedef enum {
	APP_EVENT_SUSPEND = 0,
	APP_EVENT_RESUME,
	APP_EVENT_POWER_OFF,

	APP_EVENT_TIME_NOT_SET,

	APP_EVENT_FW_TO_UPDATE_SELF,
	APP_EVENT_FW_UPDATING_SELF,
	APP_EVENT_FW_UPDATED_SELF,
	APP_EVENT_FW_TO_UPDATE_COPROC,
	APP_EVENT_FW_UPDATING_COPROC,
	APP_EVENT_FW_UPDATED_COPROC,

	APP_EVENT_MAX
} APP_EVENT_TYPE;

struct app_events_callback;

typedef void (*app_events_callback_handler_t)(struct app_events_callback *cb, APP_EVENT_TYPE event);

struct app_events_callback {
	/* This is meant to be used in the library and the user should not mess with it */
	int pos;

	/* Actual callback function being called when relevant. */
	app_events_callback_handler_t handler;

	/* Name of the command */
	APP_EVENT_TYPE event;
};

static inline int app_events_manage_callback(gll_t *callbacks,
					struct app_events_callback *callback,
					bool set)
{
	assert(callback);
	assert(callback->handler);

	/* check if list is empty, if not remove the item */
	if (gll_get(callbacks, 0) != NULL) {
		if (gll_remove(callbacks, callback->pos) != NULL) {
			if (!set) {
				return -1;
			}
		}
	}

	if (set) {
		gll_add(callbacks, callback, callback->pos);
	}

	return 0;
}

static inline void app_events_fire_callback(gll_t *list, APP_EVENT_TYPE event) {
	struct app_events_callback *cb;
	int i = 0;

	while (1) {
		cb = gll_get(list, i++);
		if (cb != NULL) {
			if (cb->event == event) {
				if (cb->handler != NULL) {
					assert(cb->handler);
					cb->handler(cb, event);
				}
			}
		} else {
			break;
		}
	}
}

int app_events_add_callback(struct app_events_callback *cb_data,
		app_events_callback_handler_t handler, APP_EVENT_TYPE event);

int app_events_remove_callback(struct app_events_callback *cb_data,
		app_events_callback_handler_t handler, APP_EVENT_TYPE event);

int app_events_report_event(APP_EVENT_TYPE reported_event);

int app_events_init();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_APP_EVENTS_INCLUDE_APP_EVENTS_H_ */
