/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jun-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_dfu);

#include <zephyr/fs/fs.h>

#include "app_thread_configs.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_uart_m2m_callback.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"
#include "app_net/app_net.h"

#include "lib_events/lib_events.h"
#if CONFIG_LIB_FILE_OPER
#include "lib_file_oper/lib_file_oper.h"
#endif

#include "app_dfu/app_dfu.h"

#define RESP_AVAILABLE_TIMEOUT		(10000)

static int m_file_handle = -1;
static int m_file_size = 0;
static int m_bytes_read = 0;


struct cmd_ctrl_data {
	struct app_uart_m2m_callback m2m_cb;
	struct k_sem lock;
	uint8_t resp_stat;
};

static struct cmd_ctrl_data * ccd_alloc_and_init()
{
	struct cmd_ctrl_data *ccd = (struct cmd_ctrl_data*) calloc(1, sizeof(struct cmd_ctrl_data));
	if (ccd == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return NULL;
	}
	k_sem_init(&ccd->lock, 0, 1);

	return ccd;
}

static int url_cmd_fire(int cmd, char *data, app_uart_m2m_callback_handler_t cb_handler)
{
	int ret = 0;

	/* allocate control data */
	struct cmd_ctrl_data *ccd = ccd_alloc_and_init();
	if (ccd == NULL)	return -ENOMEM;

	ccd->m2m_cb.user_data = ccd;
	ccd->m2m_cb.cmd = cmd;
	app_uart_m2m_callback_add(&ccd->m2m_cb, cb_handler, ccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s%d%s%s%s",	/* 200,50,url...\n*/
								ccd->m2m_cb.cmd,
								M2M_CMD_PAYLOAD_DELIM,
								strlen(data),
								M2M_CMD_PAYLOAD_DELIM,
								data,
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
	ret = k_sem_take(&ccd->lock, K_MSEC(RESP_AVAILABLE_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%d cmd timed out", cmd);
		goto err;
	}

	/* set the response */
	if (ccd->resp_stat == APP_NET_OK) 			ret = 0;
	else if (ccd->resp_stat == APP_NET_ERR) 	ret = -1;

err:
	app_uart_m2m_callback_remove(&ccd->m2m_cb, cb_handler, ccd->m2m_cb.cmd);
	free(ccd);
	return ret;
}

static void download_url_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct cmd_ctrl_data *ccd = (struct cmd_ctrl_data *) cb->user_data;

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
				k_sem_give(&ccd->lock);
			}
		}
	}
}

static void file_download_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	int ret = 0;
	struct cmd_ctrl_data *ccd = (struct cmd_ctrl_data *) cb->user_data;

	if (cmd == C20X_M2M_CMD_FILE_DOWNLOAD) {

		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}

		char *tok = strtok((char*)frame->payload, ",");		// cmd id
		int preamble_len = strlen(tok) + 1;					// length of cmd_id + comma, e.g. 202,...
		int copy_len = frame->payload_len - preamble_len;	// we need to exclude the preamble and copy the data only

		/* extract data and save to a file */
		if (frame->type == UART_M2M_FRAME_DATA_RESP) {
			m_bytes_read += frame->payload_len;
			LOG_DBG("payload_len = %d, sequence = %d, preamble_len = %d, copy_len = %d, bytes_read = %d",
					frame->payload_len, frame->sequence, preamble_len, copy_len, m_bytes_read);

#if CONFIG_LIB_FILE_OPER
			struct lib_file_oper_rw *wr = (struct lib_file_oper_rw*) calloc(1, sizeof(struct lib_file_oper_rw));
			if (wr == NULL) {
				LOG_ERR("calloc failed at %s", __func__);
				return;
			}

			wr->len = copy_len;
			wr->data = (char*) calloc(1, wr->len);
			if (wr->data == NULL) {
				LOG_ERR("%s calloc failed!", __func__);
				return;
			}

			memcpy(wr->data, (frame->payload+preamble_len), wr->len);

			/* write to file if we have a valid file handle */
			if (m_file_handle >= 0) {
				ret = lib_file_oper_write(m_file_handle, wr);
				m_file_size += wr->len;
				LOG_DBG("wrote %d bytes", wr->len);
			}
#endif
		} else if (frame->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
			tok = strtok(NULL, "\n");	// cmd_stat, we are expecting STREND (end of stream)
			if (tok != NULL) {
				if (strcmp(tok, M2M_CMD_RESP_STREND) == 0) {
#if CONFIG_LIB_FILE_OPER
					/* no more data to write, close the file */
					ret = lib_file_oper_close_file(m_file_handle);
					LOG_INF("total bytes wrote to file = %d", m_file_size);
					LOG_INF("total bytes received = %d", m_bytes_read);
#endif
				} else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
					LOG_ERR("cmd_stat received %s", M2M_CMD_RESP_ERR);
				}

				/* deallocate resources */
				app_uart_m2m_callback_remove(&ccd->m2m_cb, file_download_handler, ccd->m2m_cb.cmd);
				free(ccd);
				lib_events_report_event(LIB_EVENT_FILE_DOWNLOAD_COMPLETED);
			}
		}
	}
}

int app_file_download_url_set(char *url)
{
	int ret = 0;
	if (url == NULL)	return -EINVAL;

	ret = url_cmd_fire(C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET, url, download_url_handler);
	return ret;
}

int app_fw_file_download(uint32_t img_op)
{
	int ret = 0;

	/* allocate control data */
	struct cmd_ctrl_data *ccd = ccd_alloc_and_init();
	if (ccd == NULL)	return -ENOMEM;

	char *dir_path=NULL, *fname=NULL, *fpath=NULL;
	int max_bytes=0, fcount=1;

	dir_path = FW_DIR_PATH;
	fcount = FW_FILE_MAX_FILE_COUNT;

	switch (img_op) {
	case APP_DFU_FW_MAIN:
	{
		fname = FW_MAIN_FILE_NAME;
		fpath = FW_MAIN_FILE_PATH;
		max_bytes = FW_MAIN_FILE_MAX_SIZE_BYTES;
		break;
	}
	case APP_DFU_FW_NET:
	{
		fname = FW_NET_FILE_NAME;
		fpath = FW_NET_FILE_PATH;
		max_bytes = FW_NET_FILE_MAX_SIZE_BYTES;
		break;
	}
	case APP_DFU_FW_BLWDRV:
	{
		fname = FW_BLWDRV_FILE_NAME;
		fpath = FW_BLWDRV_FILE_PATH;
		max_bytes = FW_BLWDRV_FILE_MAX_SIZE_BYTES;
		break;
	}
	default:
		break;
	}

#if CONFIG_LIB_FILE_OPER
	/* delete if file exists */
	//lib_file_oper_delete_file(m_file_handle, fpath, false);
	ret = fs_unlink(fpath);

	/* create / open the download file */
	m_file_handle = lib_file_oper_create_open_file(dir_path, fname, fpath,
			max_bytes, fcount,
			(FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND));
	if (m_file_handle < 0) {
		LOG_ERR("could not create / open file!");
		free(ccd);
		return -1;
	}
	m_file_size = 0;
#endif
	m_bytes_read = 0;

	ccd->m2m_cb.user_data = ccd;
	ccd->m2m_cb.cmd = C20X_M2M_CMD_FILE_DOWNLOAD;
	app_uart_m2m_callback_add(&ccd->m2m_cb, file_download_handler, ccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_data_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s",	/* 202\n*/
								ccd->m2m_cb.cmd,
								M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((char*)frame.payload);

	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(&frame);

	/* serialize the frame and send */
	size_t sdata_len=0;
	char *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer != NULL) {
		LOG_HEXDUMP_DBG(serialized_buffer, sdata_len, "SETTINGS DOWNLOAD TX");
		ret = app_wifi_bt_cmd_send(serialized_buffer, sdata_len);
		free(serialized_buffer);

		lib_events_report_event(LIB_EVENT_FILE_DOWNLOAD_STARTED);

		/* we don't wait for the response so return here,
		 * resources will be deallocated inside the response callback */
		return ret;
	}

//err:
	app_uart_m2m_callback_remove(&ccd->m2m_cb, file_download_handler, ccd->m2m_cb.cmd);
	free(ccd);
	return -1;
}




