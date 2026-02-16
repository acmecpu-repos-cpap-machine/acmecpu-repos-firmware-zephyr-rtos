/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_settings.c
 * Created on: 03-Aug-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>



#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"host_cmds"

/* The settings set command 'C20X_M2M_CMD_SETTINGS_VAL_SET' is common for multiple
 * user interfaces like http server, bluetooth etc. Hence, the callback and related data
 * are common. The below structure and variables are used for handling the settings set
 * commands' callback
 * */
struct resp_cb_data {
	bool resp_available;
	uint8_t cmd_stat;		// 0 - OK, 1 - FAIL
	SemaphoreHandle_t lock;
};
static struct resp_cb_data m_rcd;
static struct host_cmd_callback cb_settings_set;


int host_cmds_settings_write(void *data, uint32_t len) {
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	char cmd_id[10] = {0x00};
	uint8_t cmd_len = sprintf(cmd_id, "%d%s", C20X_M2M_CMD_ID_SETTINGS, M2M_CMD_PAYLOAD_DELIM);

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = cmd_len+len;
	memcpy(frame.payload, cmd_id, cmd_len);
	memcpy(frame.payload+cmd_len, data, len);

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

int host_cmds_settings_val_set(const char *path, const char *val) {
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	char cmd_id[10] = {0x00};
	uint8_t cmd_len = sprintf(cmd_id, "%d%s", C20X_M2M_CMD_SETTINGS_VAL_SET, M2M_CMD_PAYLOAD_DELIM);

	lib_m2m_frame_header_single_req_make(&frame);

	strcpy((char*)frame.payload, cmd_id);
	strcat((char*)frame.payload, path);
	strcat((char*)frame.payload, M2M_CMD_PAYLOAD_DELIM);
	strcat((char*)frame.payload, val);
	strcat((char*)frame.payload, M2M_CMD_PAYLOAD_TERM);

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


static int update_data_availability(SemaphoreHandle_t mutex, bool *pvar, bool val) {
	if (mutex != NULL) {
	 if (xSemaphoreTake(mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
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

static void cb_settings_set_handler(struct host_cmd_callback *cb, uint32_t cmd, void *frame)
{
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (cmd == C20X_M2M_CMD_SETTINGS_VAL_SET) {
		ESP_LOGW(TAG, "cb_settings_set_handler: cmd %d", C20X_M2M_CMD_SETTINGS_VAL_SET);

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
				if (xSemaphoreTake(m_rcd.lock, (TickType_t)(10 / portTICK_PERIOD_MS)) == pdTRUE) {
					if (strcmp(tok, M2M_CMD_RESP_OK) == 0)
						m_rcd.cmd_stat = HOST_CMD_STAT_OK;
					else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0)
						m_rcd.cmd_stat = HOST_CMD_STAT_ERR;
					else if (strcmp(tok, M2M_CMD_RESP_CONT) == 0)
						m_rcd.cmd_stat = HOST_CMD_STAT_CONT;
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

int host_cmds_send_settings_val_set_to_host(char *settings_path, char *settings_val)
{
	int ret = 0;
	if (update_data_availability(m_rcd.lock, &m_rcd.resp_available, false) < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val_set_to_host: update_data_availability failed");
		return -1;
	}
	ret = host_cmds_settings_val_set(settings_path, settings_val);
	if (ret < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val_set_to_host: : host_cmds_settings_val_set failed");
		return -1;
	}
	ret = 0;
	if (wait_until_timeout(&m_rcd.resp_available) < 0) {
		ESP_LOGE(TAG, "host_cmds_send_settings_val_set_to_host: : wait_until_timeout failed");
		return -1;
	}

	if (m_rcd.cmd_stat == HOST_CMD_STAT_OK)			ret = HOST_CMD_STAT_OK;
	else if (m_rcd.cmd_stat == HOST_CMD_STAT_CONT)	ret = HOST_CMD_STAT_CONT;
	else											ret = -1;

	return ret;
}

int host_cmds_settings_init()
{
	m_rcd.lock = xSemaphoreCreateMutex();
	m_rcd.resp_available = false;
	host_cmds_add_callback(&cb_settings_set, cb_settings_set_handler, C20X_M2M_CMD_SETTINGS_VAL_SET);

	return 0;
}
