/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 28-Apr-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_wifibt);
#include <zephyr/drivers/uart.h>

#include <string.h>
#include <stdlib.h>

#include "acpu_c201_modules.h"
#include "app_thread_configs.h"
#include "app_uart_m2m_com/app_uart_m2m_com.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"

#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt.h"

#if RING_BUF_TEST
	#include <zephyr/sys/ring_buffer.h>
#include "../app_uart_m2m_com_priv.h"
#endif

struct wifibt_rx_data {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
#if RING_BUF_TEST
//	struct ring_buf *rb;
	struct bsp_uart_serial_config *cfg;
#else
	uint8_t *data;
#endif
	size_t len;
};

/* Wifi BT app thread variables */
K_THREAD_STACK_DEFINE(m_wifibt_th_stack, APP_THREAD_STACK_SIZE_WIFI_BT);
static struct k_thread m_wifibt_th_data;
static k_tid_t m_wifibt_tid;
struct k_fifo m_wifibt_fifo;

static uint8_t m_com_state = APP_WIFI_BT_COM_STATE_IDLE;

//static int m_test_fifo_size = 0;

static void app_wifi_bt_cb(void *rx_data, size_t len)
{
	if ((rx_data == NULL) || (len <= 0)) {
		LOG_ERR("invalid params");
		return;
	}

	struct wifibt_rx_data *wbd = (struct wifibt_rx_data*) calloc(1,	sizeof(struct wifibt_rx_data));
	if (wbd == NULL) {
		LOG_ERR("%s calloc failed", __func__);
		return;
	}

#if NEW_BUF_TEST
	LOG_HEXDUMP_DBG(rx_data, len, "rxdata");
	wbd->data = rx_data;
#elif RING_BUF_TEST
//	wbd->rb = (struct ring_buf *) rx_data;
	wbd->cfg = (struct bsp_uart_serial_config*) rx_data;
	wbd->len = len;
	LOG_DBG("cb wbd->len = %d", wbd->len);
#else
	LOG_HEXDUMP_DBG(rx_data, len, "rxdata");
	wbd->data = (uint8_t *) calloc(1, len);
	if (wbd->data == NULL) {
		LOG_ERR("%s calloc failed", __func__);
		return;
	}
	memcpy(wbd->data, rx_data, len);
#endif

	wbd->len = len;

//	LOG_DBG("wbd->data = %p", wbd->data);
	LOG_DBG("cb wbd->len = %d", wbd->len);
//	LOG_HEXDUMP_DBG(wbd->data, wbd->len, "RX");

	/* Put data into the fifo */
	k_fifo_put(&m_wifibt_fifo, wbd);
//	m_test_fifo_size++;
}

static void app_wifi_bt_thread(void *p1, void *p2, void *p3)
{
	int ret = 0;
	struct wifibt_rx_data *wbd = NULL;
	while (1) {
		m_com_state = APP_WIFI_BT_COM_STATE_IDLE;

		/* dequeue a rx data packet from the fifo */
		wbd = k_fifo_get(&m_wifibt_fifo, K_FOREVER);
		if (wbd == NULL) {
			LOG_ERR("k_fifo_get failed");
			continue;
		}
//		m_test_fifo_size--;

//		LOG_INF("fifo_size = %d", m_test_fifo_size);

		/* start of transaction */
		m_com_state = APP_WIFI_BT_COM_STATE_BUSY;

		struct m2m_frame_t in_frame, out_frame;
		memset(&in_frame, 0x00, sizeof(struct m2m_frame_t));
		memset(&out_frame, 0x00, sizeof(struct m2m_frame_t));

		if ((wbd->len <= 0) || (wbd->len > UART_M2M_FRAME_SIZE_MAX)) {
			LOG_ERR("invalid length wbd->len = %d", wbd->len);
#if RING_BUF_TEST
			free(wbd);
#else
			free(wbd->data);
			free(wbd);
#endif
			continue;
		}

#if RING_BUF_TEST
		/* get data from ring buffer and decode it */
		uint8_t *buf;
		uint32_t len;
		struct ring_buf *rb = &wbd->cfg->rb;
		len = ring_buf_get_claim(rb, &buf, wbd->len);
		if (len == 0) {
			LOG_ERR("ring buffer incorrect length, read = %d, got = %d", wbd->len, len);
			uart_irq_rx_enable(wbd->cfg->au_map->dev);
			free(wbd);
			continue;
		}

		LOG_DBG("th wbd->len = %d", wbd->len);
		int ret_decode = lib_m2m_frame_decode(buf, len, &in_frame, CHECKSUM_VERIFY);

		ret = ring_buf_get_finish(rb, len);
		if (ret < 0) {
			LOG_ERR("ring buffer finish incorrect length");
			uart_irq_rx_enable(wbd->cfg->au_map->dev);
			free(wbd);
			continue;
		}
		LOG_INF("ring_buf_get_finish = %d", len);

		uart_irq_rx_enable(wbd->cfg->au_map->dev);
		free(wbd);

		if (ret_decode < 0) {
			LOG_ERR("lib_m2m_frame_decode failed!, %d", ret_decode);
			// todo: handle error
			continue;
		}


#else
		/* decode the buffer */
		//	LOG_DBG("wbd->data = %p", wbd->data);
		LOG_DBG("th wbd->len = %d", wbd->len);
		ret = lib_m2m_frame_decode(wbd->data, wbd->len, &in_frame, CHECKSUM_VERIFY);

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
#endif	// RING_BUF_TEST

		/* decode successful, process the frame */
//		int64_t start, delta = 0;
//		start = k_uptime_get();
		ret = app_wifi_bt_cmd_process(&in_frame, &out_frame);

		switch (ret) {
		case UART_M2M_FRAME_SINGLE_REQ:
		{
			LOG_INF("in frame type: SINGLE_REQ");

			/* compute checksum */
			ret = lib_m2m_frame_checksum_compute(&out_frame);

			/* send response */
			size_t sdata_len=0;
			char *ser_buf = lib_m2m_frame_alloc_serialize(&out_frame, &sdata_len);
			if (ser_buf == NULL) {
				goto end;
			}

//			LOG_HEXDUMP_DBG(ser_buf, sdata_len, "ser_buf");
			app_wifi_bt_cmd_send(ser_buf, sdata_len);

			/* free memory */
			free(ser_buf);
			ser_buf = NULL;
			break;
		}
		case UART_M2M_FRAME_SINGLE_RESP:
		{
			LOG_INF("in frame type: SINGLE_RESP");
			break;
		}
		case UART_M2M_FRAME_STREAM_REQ:
		{
			LOG_DBG("in frame type: STREAM_REQ");
			break;
		}
		case UART_M2M_FRAME_STREAM_RESP:
		{
			LOG_DBG("in frame type: STREAM_RESP");
			break;
		}
		case UART_M2M_FRAME_DATA_REQ:
		{
			LOG_DBG("in frame type: DATA_REQ");
			break;
		}
		case UART_M2M_FRAME_DATA_RESP:
		{
			LOG_DBG("in frame type: DATA_RESP");
			break;
		}
		case UART_M2M_FRAME_DATA_ACK:
		{
			LOG_DBG("in frame type: DATA_ACK");
			break;
		}
		case UART_M2M_FRAME_DATA_RESP_ENDSTR:
		{
			LOG_DBG("in frame type: RESP_ENDSTR");
			break;
		}
		default:
			LOG_ERR("app_wifi_bt_cmd_process failed!");
			break;
		}
//		delta = k_uptime_delta(&start);
//		LOG_INF("wifi_bt_cmd_process time: %lld", delta);
end:
		;
		/* end of transaction */
	}
}

int app_wifi_bt_com_state_get()
{
	return m_com_state;
}

int app_wifi_bt_init()
{
	int ret = 0;

	/* Configure the UART interface */
	ret = app_uart_m2m_configure(UART_COM_WIFI_BT, app_wifi_bt_cb);

	app_wifi_bt_cmd_init();

	/* Initialize the Rx data fifo */
	k_fifo_init(&m_wifibt_fifo);

	/* Start wi-fi bt application thread */
	m_wifibt_tid = k_thread_create(&m_wifibt_th_data, m_wifibt_th_stack,
					K_THREAD_STACK_SIZEOF(m_wifibt_th_stack), app_wifi_bt_thread,
					NULL, NULL, NULL, APP_THREAD_PRIO_WIFI_BT, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_wifibt_tid, APP_THREAD_NAME_WIFI_BT);
#endif
	return ret;
}
