/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Aug-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_bldcdrv);

#include <string.h>
#include <stdlib.h>

#include "acpu_c201_modules.h"
#include "app_thread_configs.h"
#include "app_uart_m2m_com/app_uart_m2m_com.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_bldc_drv/app_bldc_drv_cmds.h"

struct bldcdrv_rx_data {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	uint8_t *data;
	size_t len;
};

/* Wifi BT app thread variables */
K_THREAD_STACK_DEFINE(m_bldcdrv_th_stack, APP_THREAD_STACK_SIZE_BLDC_DRV);
static struct k_thread m_bldcdrv_th_data;
static k_tid_t m_bldcdrv_tid;
struct k_fifo m_bldcdrv_fifo;

static void app_bldc_drv_cb(void *rx_data, size_t len)
{
	LOG_DBG("bldc cmd = %lld", app_bldc_drv_exe_time_get());

	if ((rx_data == NULL) || (len <= 0)) {
		LOG_ERR("invalid params");
		return;
	}

	struct bldcdrv_rx_data *wbd = (struct bldcdrv_rx_data*) calloc(1,	sizeof(struct bldcdrv_rx_data));
	if (wbd == NULL) {
		LOG_ERR("%s calloc failed", __func__);
		return;
	}
#if NEW_BUF_TEST
	LOG_HEXDUMP_DBG(rx_data, len, "rxdata");
	wbd->data = rx_data;
#else
	LOG_HEXDUMP_DBG(rx_data, len, "RX_DATA");
	wbd->data = (uint8_t *) calloc(1, len);
	if (wbd->data == NULL) {
		LOG_ERR("%s calloc failed", __func__);
		return;
	}
	memcpy(wbd->data, rx_data, len);
#endif
	wbd->len = len;

//	LOG_DBG("wbd->data = %p", wbd->data);
	LOG_DBG("wbd->len = %d", wbd->len);

	/* Put data into the fifo */
	k_fifo_put(&m_bldcdrv_fifo, wbd);
}

static void app_bldc_drv_thread(void *p1, void *p2, void *p3)
{
	int ret = 0;
	struct bldcdrv_rx_data *wbd = NULL;
	while (1) {
		/* dequeue a rx data packet from the fifo */
		wbd = k_fifo_get(&m_bldcdrv_fifo, K_FOREVER);
		if (wbd == NULL) {
			LOG_ERR("k_fifo_get failed");
			continue;
		}

		struct m2m_frame_t in_frame, out_frame;
		memset(&in_frame, 0x00, sizeof(struct m2m_frame_t));
		memset(&out_frame, 0x00, sizeof(struct m2m_frame_t));

		if ((wbd->len <= 0) || (wbd->len > UART_M2M_FRAME_SIZE_MAX)) {
			LOG_ERR("invalid length wbd->len = %d", wbd->len);
			free(wbd->data);
			free(wbd);
			continue;
		}

		/* decode the buffer */
		LOG_DBG("th wbd->len = %d", wbd->len);
		ret = lib_m2m_frame_decode(wbd->data, wbd->len, &in_frame, CHECKSUM_IGNORE);

		LOG_DBG("in_frame.payload_len = %d", in_frame.payload_len);
		LOG_DBG("wbd = %p", wbd);

		free(wbd->data);
		free(wbd);

		if (ret == -EINVAL) {
			LOG_ERR("frame_decode: invalid input params");
		} else if (ret == -EPROTO) {
			LOG_ERR("frame_decode: checksum did not match");
		} else if (ret == -E2BIG) {
			LOG_ERR("frame_decode: payload size too big to handle");
		}
		if (ret < 0) {
			LOG_ERR("lib_m2m_frame_decode failed!, %d", ret);
//			goto end;
			continue;
		}

		/* decode successful, process the frame */
		ret = app_bldc_drv_cmd_process(&in_frame, &out_frame);
		if (ret < 0) {
			LOG_ERR("app_bldc_drv_cmd_process failed!");
		} else if (ret == UART_M2M_FRAME_SINGLE_RESP) {
			LOG_DBG("in frame type: RESPONSE");
		} else if (ret == UART_M2M_FRAME_STREAM_RESP) {
			LOG_DBG("in frame type: STREAM_RESP");
		} else if (ret == UART_M2M_FRAME_DATA_RESP) {
			LOG_DBG("in frame type: UART_M2M_FRAME_DATA_RESP");
		} else if (ret == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
			LOG_DBG("in frame type: UART_M2M_FRAME_DATA_RESP_ENDSTR");
		} else {
			LOG_INF("in frame type: REQUEST");

			/* compute checksum */
			ret = lib_m2m_frame_checksum_compute(&out_frame);

			/* send response */
			size_t sdata_len=0;
			char *ser_buf = lib_m2m_frame_alloc_serialize(&out_frame, &sdata_len);
			if (ser_buf == NULL) {
				goto end;
			}

//			LOG_HEXDUMP_DBG(ser_buf, sdata_len, "ser_buf");
			app_bldc_drv_cmd_send(ser_buf, sdata_len);

			/* free memory */
			free(ser_buf);
			ser_buf = NULL;
		}
end:
		;
		/* end of transaction */
	}
}

int app_bldc_drv_init()
{
	int ret = 0;

	/* Configure the UART interface */
	ret = app_uart_m2m_configure(UART_COM_MOTOR_DRV, app_bldc_drv_cb);

	app_bldc_drv_cmd_init();

	/* Initialize the Rx data fifo */
	k_fifo_init(&m_bldcdrv_fifo);

	/* Start wi-fi bt application thread */
	m_bldcdrv_tid = k_thread_create(&m_bldcdrv_th_data, m_bldcdrv_th_stack,
					K_THREAD_STACK_SIZEOF(m_bldcdrv_th_stack), app_bldc_drv_thread,
					NULL, NULL, NULL, APP_THREAD_PRIO_BLDC_DRV, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_bldcdrv_tid, APP_THREAD_NAME_BLDC_DRV);
#endif
	return ret;
}
