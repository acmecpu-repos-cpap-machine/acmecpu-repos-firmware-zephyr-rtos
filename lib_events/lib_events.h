/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_LIB_LIB_EVENTS_LIB_EVENTS_H_
#define SRC_LIB_LIB_EVENTS_LIB_EVENTS_H_

#include <zephyr/sys/slist.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/* System level events */
typedef enum {
	LIB_EVENT_SUSPEND = 0,
	LIB_EVENT_RESUME,
	LIB_EVENT_POWER_ON,
	LIB_EVENT_POWER_OFF,
	LIB_EVENT_REBOOT,

	LIB_EVENT_SYSTEM_BOOTING,
	LIB_EVENT_SYSTEM_BOOTING_COMPLETE,

	LIB_EVENT_CHARGER_ATTACHED,
	LIB_EVENT_CHARGER_REMOVED,
	LIB_EVENT_CHARGE_COMPLETE,

	LIB_EVENT_TIME_NOT_SET,

	LIB_EVENT_SETTINGS_CHANGED,

	LIB_EVENT_BLOWER_FAULT,

	LIB_EVENT_NET_WIFI_STARTED,
	LIB_EVENT_NET_WIFI_STOPPED,
	LIB_EVENT_NET_WIFI_CONNECTED,
	LIB_EVENT_NET_WIFI_DISCONNECTED,
	LIB_EVENT_NET_HOTSPOT_STARTED,
	LIB_EVENT_NET_HOTSPOT_STOPPED,
	LIB_EVENT_NET_BT_STARTED,
	LIB_EVENT_NET_BT_STOPPED,

	LIB_EVENT_UCPD_SNK_NEGO_STARTED,
	LIB_EVENT_UCPD_SNK_NEGO_DONE,
	LIB_EVENT_UCPD_SNK_NEGO_FAILED,

	LIB_EVENT_FILE_DOWNLOAD_STARTED,
	LIB_EVENT_FILE_DOWNLOAD_COMPLETED,
	LIB_EVENT_FILE_DOWNLOAD_FAILED,

	LIB_EVENT_FW_PROGRAM_STARTED,
	LIB_EVENT_FW_PROGRAM_COMPLETED,
	LIB_EVENT_FW_PROGRAM_FAILED,

	LIB_EVENT_MAX
} LIB_EVENT_TYPE;

struct lib_events_callback;

typedef void (*lib_events_callback_handler_t)(struct lib_events_callback *cb, LIB_EVENT_TYPE event);

struct lib_events_callback {
	/* This is meant to be used in the lib_events library and the user should not mess with it */
	sys_snode_t node;

	/* Actual callback function being called when relevant. */
	lib_events_callback_handler_t handler;

	/* Type of event */
	LIB_EVENT_TYPE event;
};

static inline int lib_events_manage_callback(sys_slist_t *callbacks,
					struct lib_events_callback *callback,
					bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

static inline void lib_events_fire_callbacks(sys_slist_t *list, LIB_EVENT_TYPE event)
{
	struct lib_events_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->event == event) {
			if (cb->handler != NULL) {
//				__ASSERT(cb->handler, "No callback handler!");
				cb->handler(cb, event);
			}
		}
	}
}

/* Function declarations */
int lib_events_report_event(LIB_EVENT_TYPE reported_event);

int lib_events_callback_add(struct lib_events_callback *cb_data,
		lib_events_callback_handler_t cb_handler, LIB_EVENT_TYPE event);

int lib_events_callback_remove(struct lib_events_callback *cb_data,
		lib_events_callback_handler_t cb_handler, LIB_EVENT_TYPE event);

int lib_events_init();

#endif /* SRC_LIB_LIB_EVENTS_LIB_EVENTS_H_ */
