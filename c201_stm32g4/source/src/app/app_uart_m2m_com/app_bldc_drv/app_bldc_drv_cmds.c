/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Aug-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <version.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_bldcdrv);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "app_uart_m2m_com/app_uart_m2m_com.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_bldc_drv/app_bldc_drv_cmds.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#include "app_uart_m2m_com/app_uart_m2m_callback.h"

static struct k_mutex m_tx_mutex;

int app_bldc_drv_cmd_process(struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame)
{
	int ret = 0;

	if ((in_frame == NULL) || (out_frame == NULL)) {
		return -EINVAL;
	}

	if (in_frame->sof != UART_M2M_START_OF_FRAME) {
		LOG_ERR("Invalid SOF");
		return -EPROTO;
	}

	/* extract the command id */
	uint8_t *buf = (uint8_t *) calloc(1, in_frame->payload_len);
	if (buf == NULL) {
		return -ENOMEM;
	}
	memcpy(buf, in_frame->payload, in_frame->payload_len);

	char *tok = strtok(buf, ",\n");
	uint16_t cmd_id = atoi(tok);

	free(buf);

	if (in_frame->type == UART_M2M_FRAME_SINGLE_REQ) {
		ret = 0;
	} else if (in_frame->type == UART_M2M_FRAME_SINGLE_RESP) {
		/* fire callback */
		app_uart_m2m_fire_callbacks(app_uart_m2m_callback_get(), cmd_id, in_frame);
		ret = UART_M2M_FRAME_SINGLE_RESP;
	} else if (in_frame->type == UART_M2M_FRAME_STREAM_REQ) {
		ret = UART_M2M_FRAME_STREAM_REQ;
	} else if (in_frame->type == UART_M2M_FRAME_STREAM_RESP) {
		ret = UART_M2M_FRAME_STREAM_RESP;
	} else {
		ret = -1;
	}

	return ret;
}

static int64_t time_start = 0;
int64_t app_bldc_drv_exe_time_get()
{
	return k_uptime_delta(&time_start);
}

int app_bldc_drv_cmd_send(void *data, size_t len)
{
	int ret = 0;
	LOG_HEXDUMP_DBG(data, len, "MOTOR_DRV_TX");
	time_start = k_uptime_get();
	k_mutex_lock(&m_tx_mutex, K_FOREVER);
	ret = app_uart_m2m_send(UART_M2M_APP_ID_MOTOR_DRV, data, len);
	k_mutex_unlock(&m_tx_mutex);

	return ret;
}

void app_bldc_drv_cmd_init()
{
	/* initialize the Tx mutex */
	k_mutex_init(&m_tx_mutex);
}
