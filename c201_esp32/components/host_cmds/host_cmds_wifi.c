/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_wifi.c
 * Created on: 19-Apr-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <string.h>

#ifndef portTICK_PERIOD_MS
#define portTICK_PERIOD_MS portTICK_PERIOD_MS
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <sys/errno.h>

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#include "comm_wifi.h"

#define TAG	"host_cmds"

struct resp_cb_data {
	bool resp_available;
	uint8_t cmd_stat;		// 0 - OK, 1 - FAIL
	SemaphoreHandle_t lock;
};
static struct resp_cb_data m_rcd;
static struct host_cmd_callback cb_settings_set;

static void cb_wifi_ssid_set_handler(struct host_cmd_callback *cb, uint32_t cmd, void *frame)
{
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (cmd == C20X_M2M_CMD_NET_WIFI_SCANNED_LIST) {
		ESP_LOGW(TAG, "cb_settings_set_handler: cmd %d", C20X_M2M_CMD_NET_WIFI_SCANNED_LIST);

		if (fr->payload_len <= 0) {
			ESP_LOGE(TAG, "Invalid payload length");
			goto err;
		}

		char *tok = strtok((char*) fr->payload, ",");	// cmd id
		ESP_LOGW(TAG, "cmd id = %s", tok);
		tok = strtok(NULL, "\n");		// cmd_stat, we are expecting OK or ERR
		if (tok != NULL) {
			ESP_LOGW(TAG, "cmd stat = %s", tok);
			if (m_rcd.lock != NULL) {
				if (xSemaphoreTake(m_rcd.lock, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
					if (strcmp(tok, M2M_CMD_RESP_OK) == 0)
						m_rcd.cmd_stat = HOST_CMD_STAT_OK;
					else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
						m_rcd.cmd_stat = HOST_CMD_STAT_ERR;
				} else {
					ESP_LOGW(TAG, "could not take semaphore");
				}
				m_rcd.resp_available = true;
				xSemaphoreGive(m_rcd.lock);
			} else {
				ESP_LOGW(TAG, "m_rcd.lock == NULL");
			}
		} else {
			ESP_LOGW(TAG, "could not extract cmd stat");
		}
	}
err:
	free(fr);
}

static int update_data_availability(SemaphoreHandle_t mutex, bool *pvar, bool val) {
	if (mutex != NULL) {
		if (xSemaphoreTake(mutex, (TickType_t)(10 / portTICK_PERIOD_MS)) == pdTRUE) {

			*pvar = val;
			xSemaphoreGive(mutex);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}
#define RESP_AVAILABLE_LOOP_DELAY	(10)
#define RESP_AVAILABLE_TIMEOUT		(10*1000)	// 10 secs
static int wait_until_timeout(bool *pvar) {
	uint32_t delay = 0;
	while (!(*pvar)) {
		vTaskDelay(RESP_AVAILABLE_LOOP_DELAY / portTICK_PERIOD_MS);
		delay += RESP_AVAILABLE_LOOP_DELAY;
		if (delay > RESP_AVAILABLE_TIMEOUT) {
			return -1;
		}
	}
	return 0;
}

static int host_cmds_send_ssid(wifi_ap_record_t *ap_info, uint16_t ap_count) {
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);
	int idx = 0;
	idx += sprintf((char*)frame.payload, "%d%s",
			C20X_M2M_CMD_NET_WIFI_SCANNED_LIST, M2M_CMD_PAYLOAD_DELIM);

	for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
		int len = strlen((char*)ap_info[i].ssid);
		if (len < 10)
			idx += sprintf((char*) (frame.payload+idx), "0%d%s",
					len, ap_info[i].ssid);
		else
			idx += sprintf((char*) (frame.payload+idx), "%d%s",
					len, ap_info[i].ssid);
	}
	idx += sprintf((char*) (frame.payload+idx), "%s", M2M_CMD_PAYLOAD_TERM);

	frame.payload_len = strlen((char*)frame.payload);

	/* serialize the frame and send response */
#if 0
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		ESP_LOGE(TAG, "%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
#endif
	/* compute checksum */
	int ret = lib_m2m_frame_checksum_compute(&frame);

	size_t sdata_len=0;
	uint8_t *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ret = host_cmds_send_only((const char *)serialized_buffer, sdata_len);
err:
	free(serialized_buffer);
	return ret;
}

int host_cmds_send_send_ssid_to_host(wifi_ap_record_t *ap_info, uint16_t ap_count)
{
	int ret = 0;
	if (update_data_availability(m_rcd.lock, &m_rcd.resp_available, false) < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val: update_data_availability failed");
		return -1;
	}
	ret = host_cmds_send_ssid(ap_info, ap_count);
	if (ret < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val: : host_cmds_html_server_page_get failed");
		return -1;
	}
	ret = 0;
	if (wait_until_timeout(&m_rcd.resp_available) < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val: : wait_until_timeout failed");
		return -1;
	}

	if (m_rcd.cmd_stat == HOST_CMD_STAT_OK)			ret = HOST_CMD_STAT_OK;
	else											ret = -1;

	return ret;
}

int host_cmds_wifi_init()
{
	m_rcd.lock = xSemaphoreCreateMutex();
	m_rcd.resp_available = false;
	host_cmds_add_callback(&cb_settings_set, cb_wifi_ssid_set_handler, C20X_M2M_CMD_NET_WIFI_SCANNED_LIST);

	return 0;
}
