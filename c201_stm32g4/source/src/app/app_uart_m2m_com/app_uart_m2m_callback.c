/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Aug-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include "app_uart_m2m_com/app_uart_m2m_callback.h"

static sys_slist_t m_app_uart_m2m_cb;

int app_uart_m2m_callback_add(struct app_uart_m2m_callback *cb_data, app_uart_m2m_callback_handler_t cb_handler, uint32_t cmd) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->cmd = cmd;

	int ret = app_uart_m2m_manage_callback(&m_app_uart_m2m_cb, cb_data, true);

	return ret;
}

int app_uart_m2m_callback_remove(struct app_uart_m2m_callback *cb_data, app_uart_m2m_callback_handler_t cb_handler, uint32_t cmd) {

	__ASSERT(cb_data, "Callback pointer should not be NULL");
	__ASSERT(cb_handler, "Callback handler pointer should not be NULL");

	cb_data->handler = cb_handler;
	cb_data->cmd = cmd;

	int ret = app_uart_m2m_manage_callback(&m_app_uart_m2m_cb, cb_data, false);

	return ret;
}

sys_slist_t * app_uart_m2m_callback_get() {
	return &m_app_uart_m2m_cb;
}
