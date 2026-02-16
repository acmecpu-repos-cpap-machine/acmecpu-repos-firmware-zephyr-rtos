/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 24-Apr-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_settings);

#include <zephyr/fs/fs.h>

#include "app_thread_configs.h"
#include "app_settings/app_settings.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
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


#define RESP_AVAILABLE_TIMEOUT		(10000)

typedef enum {
	STREAM_FRAME_START=0,
	STREAM_FRAME_IN_PROCESS,
	STREAM_FRAME_END
} STREAM_FRAME_STAT;

struct resp_cb_data {
//	bool resp_available;
	struct k_sem lock;
	int stream_stat;
};

struct settings_cmd_ctrl_data {
	struct app_uart_m2m_callback m2m_cb;
	struct resp_cb_data rcb_data;
	uint8_t resp_stat;
};

//static int m_stream_stat = STREAM_FRAME_START;
static int m_dnl_file_handle = -1;

static int m_file_size = 0;
static int m_bytes_read = 0;

static struct settings_cmd_ctrl_data * sccd_alloc_and_init()
{
	struct settings_cmd_ctrl_data *sccd = (struct settings_cmd_ctrl_data*) calloc(1, sizeof(struct settings_cmd_ctrl_data));
	if (sccd == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return NULL;
	}
	k_sem_init(&sccd->rcb_data.lock, 0, 1);

//	sccd->rcb_data.resp_available = false;
	sccd->rcb_data.stream_stat = STREAM_FRAME_START;
	return sccd;
}



static void settings_download_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	int ret = 0;
	struct settings_cmd_ctrl_data *sccd = (struct settings_cmd_ctrl_data *) cb->user_data;

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
			if (m_dnl_file_handle >= 0) {
				ret = lib_file_oper_write(m_dnl_file_handle, wr);
				m_file_size += wr->len;
				LOG_DBG("wrote %d bytes to %s", wr->len, SETTINGS_DNL_CURR_FILE_PATH);
			}
#endif
		} else if (frame->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
			tok = strtok(NULL, "\n");	// cmd_stat, we are expecting STREND (end of stream)
			if (tok != NULL) {
				if (strcmp(tok, M2M_CMD_RESP_STREND) == 0) {
#if CONFIG_LIB_FILE_OPER
					/* no more data to write, close the file */
					ret = lib_file_oper_close_file(m_dnl_file_handle);
					LOG_INF("total bytes wrote to file = %d", m_file_size);
					LOG_INF("total bytes received = %d", m_bytes_read);
#endif
				} else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) {
					LOG_ERR("cmd_stat received %s", M2M_CMD_RESP_ERR);
				}

				/* deallocate resources */
				app_uart_m2m_callback_remove(&sccd->m2m_cb, settings_download_handler, sccd->m2m_cb.cmd);
				free(sccd);
			}
		}
	}
}

static void settings_download_url_cert_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct settings_cmd_ctrl_data *sccd = (struct settings_cmd_ctrl_data *) cb->user_data;

	if (cmd == sccd->m2m_cb.cmd) {
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
					sccd->resp_stat = APP_NET_OK;
				else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
					sccd->resp_stat = APP_NET_ERR;
				k_sem_give(&sccd->rcb_data.lock);
			}
		}
	}
}

static int url_cert_cmd_fire(int cmd, char *data,
		app_uart_m2m_callback_handler_t cb_handler)
{
	int ret = 0;

	/* allocate control data */
	struct settings_cmd_ctrl_data *sccd = sccd_alloc_and_init();
	if (sccd == NULL)	return -ENOMEM;

	sccd->m2m_cb.user_data = sccd;
	sccd->m2m_cb.cmd = cmd;
	app_uart_m2m_callback_add(&sccd->m2m_cb, cb_handler, sccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s%d%s%s%s",	/* 200,50,url...\n*/
								sccd->m2m_cb.cmd,
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
	ret = k_sem_take(&sccd->rcb_data.lock, K_MSEC(RESP_AVAILABLE_TIMEOUT));
	if (ret == -EAGAIN) {
		LOG_ERR("%d cmd timed out", cmd);
		goto err;
	}

	/* set the response */
	if (sccd->resp_stat == APP_NET_OK) 			ret = 0;
	else if (sccd->resp_stat == APP_NET_ERR) 	ret = -1;

err:
	app_uart_m2m_callback_remove(&sccd->m2m_cb, cb_handler, sccd->m2m_cb.cmd);
	free(sccd);
	return ret;
}

int app_settings_file_download_url_set()
{
	int ret = 0;

	/* TODO: read url from config file */

	/* if no config file found use the URL from Kconfig */
	ret = url_cert_cmd_fire(C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET,
			CONFIG_APP_SETTINGS_DNL_FILE_URL, settings_download_url_cert_handler);

	return ret;
}

int app_settings_file_download_url_set_dynamic(char *url)
{
	int ret = 0;
	if (url == NULL)	return -EINVAL;

	ret = url_cert_cmd_fire(C20X_M2M_CMD_FILE_DOWNLOAD_URL_SET,
			url, settings_download_url_cert_handler);

	return ret;
}

int app_settings_file_download_cert_set()
{
	int ret = 0;

	/* TODO: read cert from config file */
	ret = url_cert_cmd_fire(C20X_M2M_CMD_FILE_DOWNLOAD_CERT_SET,
			"", settings_download_url_cert_handler);

	return ret;
}

int app_settings_file_download()
{
	int ret = 0;

	/* allocate control data */
	struct settings_cmd_ctrl_data *sccd = sccd_alloc_and_init();
	if (sccd == NULL)	return -ENOMEM;

#if CONFIG_LIB_FILE_OPER
	ret = fs_unlink(SETTINGS_DNL_CURR_FILE_PATH);

	/* create / open the settings download file */
	m_dnl_file_handle = lib_file_oper_create_open_file(
			SETTINGS_DNL_DIRECTORY_PATH,
			SETTINGS_DNL_CURR_FILE_NAME,
			SETTINGS_DNL_CURR_FILE_PATH,
			SETTINGS_DNL_CURR_FILE_MAX_SIZE_BYTES,
			SETTINGS_DNL_MAX_FILE_COUNT,
			(FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND));
	if (m_dnl_file_handle < 0)	{
		LOG_ERR("could not create / open file!");
		free(sccd);
		return -1;
	}
	m_file_size = 0;
#endif
	m_bytes_read = 0;

	sccd->m2m_cb.user_data = sccd;
	sccd->m2m_cb.cmd = C20X_M2M_CMD_FILE_DOWNLOAD;
	app_uart_m2m_callback_add(&sccd->m2m_cb, settings_download_handler, sccd->m2m_cb.cmd);

	/* make and send command to network processor */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_data_req_make(&frame);

	sprintf((char*)frame.payload, "%d%s",	/* 202\n*/
								sccd->m2m_cb.cmd,
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

		/* we don't wait for the response so return here,
		 * resources will be deallocated inside the response callback */
		return ret;
	}

//err:
	app_uart_m2m_callback_remove(&sccd->m2m_cb, settings_download_handler, sccd->m2m_cb.cmd);
	free(sccd);
	return -1;
}

int app_settings_file_delete()
{
	int ret = 0;

#if CONFIG_LIB_FILE_OPER
	ret = lib_file_oper_delete_file(m_dnl_file_handle, SETTINGS_DNL_CURR_FILE_PATH, false);
#endif

	return ret;
}

