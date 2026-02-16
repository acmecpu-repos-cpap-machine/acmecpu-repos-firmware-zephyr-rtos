/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 03-May-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <inttypes.h>
#include <string.h>
#include <sys/errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"host_cmds"

static int m_tot_payload_sent = 0;
static uint32_t m_last_sequence = 0;
static SemaphoreHandle_t m_sem_ack;

static int buffer_send_wait_retry(uint8_t *ser_buf, size_t buf_len)
{
	// try to send the buffer
	int timeout = UART_M2M_ACK_TIMEOUT_BASE_MS;
	int retry_factor = 1, retry_count = 0;
	int semret = 0;
	do {
		int sent_len = host_cmds_send_only((const char*) ser_buf, buf_len);
		if (sent_len != buf_len) {
			ESP_LOGW(TAG, "host_cmds_send_only could not send all requested bytes");
			// TODO handle error
		}
		ESP_LOG_BUFFER_HEXDUMP(TAG, ser_buf, buf_len, ESP_LOG_DEBUG);

		/* wait for ack */
		semret = xSemaphoreTake(m_sem_ack, (TickType_t)((timeout * retry_factor) / portTICK_PERIOD_MS));
		if (semret != pdTRUE) {
			/* did not receive acknowledgment re-send packet */
			retry_factor *= 2;
			retry_count++;
			ESP_LOGW(TAG, "timeout, retrying with %d ms wait period, retry number %d",
					timeout * retry_factor, retry_count);
			if (retry_count >= UART_M2M_RETRY_COUNT_MAX) {
				ESP_LOGE(TAG, "Maximum retries reached, abroting!");
				return -1;
			}
		}
	} while (semret != pdTRUE);
	return 0;
}

void host_cmds_net_total_payload_sent_reset()
{
	m_tot_payload_sent = 0;
}

int host_cmds_net_total_payload_sent_get()
{
	return m_tot_payload_sent;
}

void host_cmds_net_data_ack_handler(void *frame)
{
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;
	uint32_t ack = m_last_sequence+1;
	if (ack == fr->ack) {
		ESP_LOGD(TAG, "ACK %ld matched with SEQ %ld", fr->ack, m_last_sequence);
		xSemaphoreGive(m_sem_ack);
		return;
	}
	ESP_LOGE(TAG, "ACK %ld not matched with SEQ %ld", fr->ack, m_last_sequence);
}

int host_cmds_net_send_stream_resp(int frame_type, int cmd, char *data_buf, int data_len)
{
	if (data_buf == NULL) {
		ESP_LOGE(TAG, "data_buf is NULL");
		return -EINVAL;
	}

	ESP_LOGI(TAG, "stream_resp: %d, %d, %d", frame_type, cmd, data_len);

	int ret = 0;
	static uint32_t seq = 1;

//	uint8_t *ser_buf = NULL;
	size_t buf_len = 0;

	/* make a data response frame */
	struct m2m_frame_t resp_frame;

	if (frame_type == UART_M2M_FRAME_DATA_RESP) {
		int bytes_sent = 0, to_copy = 0;
		lib_m2m_frame_header_data_resp_make(&resp_frame);
		while (bytes_sent < data_len) {

			int payload_idx = sprintf((char*) resp_frame.payload, "%d%s",
											cmd, M2M_CMD_PAYLOAD_DELIM);

			int max_data_size = UART_M2M_PAYLOAD_SIZE_MAX -
								(payload_idx /*+ strlen(M2M_CMD_PAYLOAD_TERM)*/);
			ESP_LOGD(TAG, "max_data_size = %d", max_data_size);

			/* the data buffer can be larger that the payload buffer, so
			 * check and send the appropriate number of bytes */
			if ((data_len - bytes_sent) > max_data_size) {
				to_copy = max_data_size;
			} else {
				to_copy = (data_len - bytes_sent);
			}
			ESP_LOGD(TAG, "data_len = %d, to_copy = %d, bytes_sent = %d", data_len, to_copy, bytes_sent);

			memcpy(resp_frame.payload + payload_idx, data_buf + bytes_sent, to_copy);
			payload_idx += to_copy;
//			payload_idx += sprintf((char*)(resp_frame.payload + payload_idx), "%s", M2M_CMD_PAYLOAD_TERM);
//			strcat((char*)(resp_frame.payload + payload_idx), M2M_CMD_PAYLOAD_TERM);
			bytes_sent += to_copy;

			/* add sequence number */
			resp_frame.sequence = seq++;
			resp_frame.payload_len = payload_idx;
			m_last_sequence = resp_frame.sequence;	// this will be verified with the acknowledgment number

			ESP_LOGD(TAG, "payload_len = %ld, sequence = %ld", resp_frame.payload_len, resp_frame.sequence);

			/* compute checksum */
			ret = lib_m2m_frame_checksum_compute(&resp_frame);

			/* serialize and send */
			uint8_t *ser_buf = lib_m2m_frame_alloc_serialize(&resp_frame, &buf_len);
			if (ser_buf == NULL) {
				ESP_LOGE(TAG, "lib_m2m_frame_alloc_serialize failed %d", ret);
				return ret;
			}

			ret = buffer_send_wait_retry(ser_buf, buf_len);
			free(ser_buf);
			if (ret < 0) goto err;

			m_tot_payload_sent += resp_frame.payload_len;
		}
	} else if (frame_type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
		lib_m2m_frame_header_data_resp_end_make(&resp_frame);
		seq = 1;	// reset sequence number

		sprintf((char*)resp_frame.payload, "%d%s%s%s", cmd,
		M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_STREND,
		M2M_CMD_PAYLOAD_TERM);

		resp_frame.payload_len = strlen((char*)resp_frame.payload);

		/* add sequence number */
		resp_frame.sequence = ++m_last_sequence;

		ESP_LOGD(TAG, "payload_len = %ld, sequence = %ld", resp_frame.payload_len, resp_frame.sequence);

		/* compute checksum */
		ret = lib_m2m_frame_checksum_compute(&resp_frame);

		/* serialize and send */
		uint8_t *ser_buf = lib_m2m_frame_alloc_serialize(&resp_frame, &buf_len);
		if (ser_buf == NULL) {
			ESP_LOGE(TAG, "lib_m2m_frame_alloc_serialize failed %d", ret);
			return ret;
		}

		ret = buffer_send_wait_retry(ser_buf, buf_len);
		free(ser_buf);
		if (ret < 0) goto err;

		ESP_LOGI(TAG, "Sent STREND");

		/* reset sequence number */
		m_last_sequence = 0;
	} else {
		ESP_LOGE(TAG, "incorrect frame type %d", frame_type);
		return -EINVAL;
	}

err:
//	free(ser_buf);
	return ret;
}

int host_cmds_net_init()
{
	m_sem_ack = xSemaphoreCreateBinary();
	if (m_sem_ack == NULL) {
		ESP_LOGE(TAG, "could not allocate resources!");
		return -1;
	}

	return 0;
}

