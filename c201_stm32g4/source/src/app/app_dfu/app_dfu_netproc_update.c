/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 22-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_dfu);

#include "app_thread_configs.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"
#include "lib_m2m_frame/lib_m2m_frame.h"
#include "app_uart_m2m_com/app_uart_m2m_callback.h"
#include "app_net/app_net.h"
#include "app_dfu/app_dfu.h"
#include "app_net/app_net_data_util.h"
#include "lib_events/lib_events.h"

#define RESP_AVAILABLE_TIMEOUT		(10000)

/* thread static variables */
K_THREAD_STACK_DEFINE(m_dfu_netproc_stack, APP_THREAD_STACK_SIZE_DFU_NETPROC);
static struct k_thread m_dfu_netproc_data;
static k_tid_t m_dfu_netproc_tid;

struct ack_cb_data {
	struct k_sem lock;
	uint32_t last_sequence;
};
struct dfu_netproc_cmd_ctrl_data {
	struct app_uart_m2m_callback m2m_cb;
	struct ack_cb_data rcb_data;
	int tot_payload_sent;
	uint8_t resp_stat;
};

static struct dfu_netproc_cmd_ctrl_data * nccd_alloc_and_init()
{
	struct dfu_netproc_cmd_ctrl_data *nccd = (struct dfu_netproc_cmd_ctrl_data*) calloc(1, sizeof(struct dfu_netproc_cmd_ctrl_data));
	if (nccd == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return NULL;
	}
	k_sem_init(&nccd->rcb_data.lock, 0, 1);

	return nccd;
}

static void fwapp_avail_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct dfu_netproc_cmd_ctrl_data *ccd = (struct dfu_netproc_cmd_ctrl_data *) cb->user_data;

	if (cmd == ccd->m2m_cb.cmd) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}

		char *tok = strtok((char*)frame->payload, ",");	// cmd id
		if (tok != NULL) {
			tok = strtok(NULL, "\n");					// status OK / ERR
			if (tok != NULL) {
				if (strcmp(tok, M2M_CMD_RESP_OK) == 0)
					ccd->resp_stat = APP_NET_OK;
				else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
					ccd->resp_stat = APP_NET_ERR;
				k_sem_give(&ccd->rcb_data.lock);
			}
		}
	}
}

int app_dfu_netproc_fw_available_send()
{
	int ret = 0;

	/* allocate control data */
	struct dfu_netproc_cmd_ctrl_data *nccd = nccd_alloc_and_init();
	if (nccd == NULL)	return -ENOMEM;

	nccd->m2m_cb.user_data = nccd;
	nccd->m2m_cb.cmd = C20X_M2M_CMD_NET_FWAPP_AVAIL;
	app_uart_m2m_callback_add(&nccd->m2m_cb, fwapp_avail_handler, nccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s",	/* 203\n*/
								nccd->m2m_cb.cmd,
								M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((char*)frame.payload);

	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -1;
		goto err;
	}

	LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "TX");
	ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	ret = k_sem_take(&nccd->rcb_data.lock, K_MSEC(RESP_AVAILABLE_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%d cmd timed out", C20X_M2M_CMD_NET_FWAPP_AVAIL);
		goto err;
	}

	/* set the response */
	if (nccd->resp_stat == APP_NET_OK) 			ret = 0;
	else if (nccd->resp_stat == APP_NET_ERR) 	ret = -1;

err:
	app_uart_m2m_callback_remove(&nccd->m2m_cb, fwapp_avail_handler, nccd->m2m_cb.cmd);
	free(nccd);
	return ret;
}

int app_dfu_netproc_fw_upgrade()
{
	int ret = 0;
	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s",	/* 205\n*/
								C20X_M2M_CMD_NET_FWAPP_UPDATE,
								M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((char*)frame.payload);

	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -1;
		return ret;
	}

	LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "TX");
	ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);
	return ret;
}

static void data_ack_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct dfu_netproc_cmd_ctrl_data *nccd = (struct dfu_netproc_cmd_ctrl_data *) cb->user_data;

	if (cmd == nccd->m2m_cb.cmd) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;
		uint32_t last_sequence = nccd->rcb_data.last_sequence;
		uint32_t ack = last_sequence + 1;
		if (ack == frame->ack) {
			LOG_DBG("ACK %d matched with SEQ %d", frame->ack, last_sequence);
			k_sem_give(&nccd->rcb_data.lock);
			return;
		}
		LOG_ERR("ACK %d not matched with SEQ %d", frame->ack, last_sequence);
	}
}

static void fw_bin_send_thread(void *p1, void *p2, void *p3)
{
	int ret = 0;
	const char *img_path = (const char *) p1;
	struct dfu_netproc_cmd_ctrl_data *nccd = (struct dfu_netproc_cmd_ctrl_data *) p2;

	/* open the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, img_path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("file %s open failed!", img_path);
		goto err;
	}

	char data_buf[(UART_M2M_FRAME_SIZE_MAX-4)*2] = {0x00};	// can contain 2 packets of m2m_frame_t.payload
	ssize_t rd_len=0;
	while (1) {
		rd_len = fs_read(&zfp, data_buf, sizeof(data_buf));
		if (rd_len > 0) {
			ret = app_net_send_data_resp(UART_M2M_FRAME_DATA_RESP, nccd->m2m_cb.cmd, data_buf,
					rd_len, &nccd->tot_payload_sent, &nccd->rcb_data.last_sequence,
					&nccd->rcb_data.lock);
		} else if (rd_len == 0) {
			ret = app_net_send_data_resp(UART_M2M_FRAME_DATA_RESP_ENDSTR, nccd->m2m_cb.cmd, data_buf,
					rd_len, &nccd->tot_payload_sent, &nccd->rcb_data.last_sequence,
					&nccd->rcb_data.lock);
			LOG_INF("file sending done");
			break;
		} else if (rd_len < 0) {
			LOG_ERR("fs_read failed, abort!");
			goto err;
		}
		if (ret != 0) {
			LOG_ERR("app_net_send_data_resp failed, abort!");
			goto err;
		}
	}

err:
	/* deallocate resources */
	fs_close(&zfp);
	app_uart_m2m_callback_remove(&nccd->m2m_cb, data_ack_handler, nccd->m2m_cb.cmd);
	free(nccd);
	lib_events_report_event(LIB_EVENT_FW_PROGRAM_COMPLETED);
}

int app_dfu_netproc_fw_send(uint32_t cmd)
{
	int ret = 0;

	/* allocate control data */
	struct dfu_netproc_cmd_ctrl_data *nccd = nccd_alloc_and_init();
	if (nccd == NULL)	return -ENOMEM;

	nccd->m2m_cb.user_data = nccd;
	nccd->m2m_cb.cmd = cmd;
	app_uart_m2m_callback_add(&nccd->m2m_cb, data_ack_handler, nccd->m2m_cb.cmd);

	/* start data sending thread */
	m_dfu_netproc_tid = k_thread_create(&m_dfu_netproc_data,
			m_dfu_netproc_stack,
			K_THREAD_STACK_SIZEOF(m_dfu_netproc_stack),
			fw_bin_send_thread, FW_NET_FILE_PATH, nccd, NULL, APP_THREAD_PRIO_DFU_NETPROC,
			0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_dfu_netproc_tid, APP_THREAD_NAME_DFU_NETPROC);
#endif
	return ret;
}

