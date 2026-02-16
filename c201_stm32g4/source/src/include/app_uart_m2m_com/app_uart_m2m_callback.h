/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Aug-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_CALLBACK_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_CALLBACK_H_

#include <zephyr/sys/slist.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
//#include <assert.h>
#include <zephyr/sys/__assert.h>


struct app_uart_m2m_callback;

typedef void (*app_uart_m2m_callback_handler_t)(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data);

struct app_uart_m2m_callback {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	/* Actual callback function being called when relevant. */
	app_uart_m2m_callback_handler_t handler;

	/* Command id */
	uint32_t cmd;

	/* user data */
	void *user_data;
};

static inline int app_uart_m2m_manage_callback(sys_slist_t *callbacks, struct app_uart_m2m_callback *callback, bool set)
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

static inline void app_uart_m2m_fire_callbacks(sys_slist_t *list, uint32_t cmd, void *data)
{
	struct app_uart_m2m_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if (cb->cmd == cmd) {
			if (cb->handler != NULL) {
//				__ASSERT(cb->handler, "No callback handler!");
				cb->handler(cb, cmd, data);
			}
		}
	}
}

/**
 * @brief	Register a callback function with the app_uart_m2m_com module which gets called when a packet is received
 * 			matching the command id parameter. This calls adds a node to the list of callbacks registered with app_uart_m2m_com module
 * @param cb_data		app_uart_m2m_callback object. This variable should not get deallocated, should be a static variable
 * @param cb_handler 	the handler function of type app_uart_m2m_callback_handler_t
 * @param cmd			the command ID to be matched with
 * @return
 * 		0		Success
 * 		-EINVAL	Invalid parameter
 */
int app_uart_m2m_callback_add(struct app_uart_m2m_callback *cb_data, app_uart_m2m_callback_handler_t cb_handler, uint32_t cmd);

/**
 * @brief	Unregister a callback function which was registered with the app_uart_m2m_com module
 * @param cb_data		The object with which the callback was registered earlier
 * @param cb_handler	The handler function
 * @param cmd			the command ID to be matched with
 * @return
 * 		0		Success
 * 		-EINVAL	Invalid parameter
 */
int app_uart_m2m_callback_remove(struct app_uart_m2m_callback *cb_data, app_uart_m2m_callback_handler_t cb_handler, uint32_t cmd);

/**
 * @brief	Returns the list head pointer which contains the list of callbacks
 * @return	The list head pointer
 */
sys_slist_t * app_uart_m2m_callback_get();


#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_CALLBACK_H_ */
