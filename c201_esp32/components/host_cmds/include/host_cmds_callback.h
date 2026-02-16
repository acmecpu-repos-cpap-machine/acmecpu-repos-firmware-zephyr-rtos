/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_callback.h
 * Created on: 28-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_CALLBACK_H_
#define COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_CALLBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "gll.h"

struct host_cmd_callback;

typedef void (*host_cmd_callback_handler_t)(struct host_cmd_callback *cb, uint32_t cmd, void *packet);

struct host_cmd_callback {
	/* This is meant to be used in the library and the user should not mess with it */
	int pos;

	/* Actual callback function being called when relevant. */
	host_cmd_callback_handler_t handler;

	/* Name of the command */
//	const char *cmd;

	/* ID of the command */
	uint32_t cmd;
};

static inline int host_cmds_manage_callback(gll_t *callbacks,
					struct host_cmd_callback *callback,
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

static inline void host_cmds_fire_callback(gll_t *list, uint32_t cmd, void *packet) {
	struct host_cmd_callback *cb;
	int i = 0;

	while (1) {
		cb = gll_get(list, i++);
		if (cb != NULL) {
//			if (!strcmp(cb->cmd, cmd)) {
			if (cb->cmd == cmd) {
				if (cb->handler != NULL) {
					assert(cb->handler);
					cb->handler(cb, cb->cmd, packet);
					break;
				}
			}
		} else {
			break;
		}
	}
}

int host_cmds_add_callback(struct host_cmd_callback *cb_data,
		host_cmd_callback_handler_t handler, uint32_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_CALLBACK_H_ */
