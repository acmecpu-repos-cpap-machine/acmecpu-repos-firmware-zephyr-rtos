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
#include <stdint.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_net);

#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_uart_m2m_callback.h"

static int buffer_send_wait_retry(uint8_t *ser_buf, size_t buf_len, struct k_sem *lock)
{
	// try to send the buffer
	int timeout = UART_M2M_ACK_TIMEOUT_BASE_MS;
	int retry_factor = 1, retry_count = 0;
	int semret = 0;
	do {
		app_wifi_bt_cmd_send(ser_buf, buf_len);
//		if (sent_len != buf_len) {
//			LOG_WRN("host_cmds_send_only could not send all requested bytes");
//			// TODO handle error
//		}
		LOG_HEXDUMP_DBG(ser_buf, buf_len, "ACK");

		/* wait for ack */
		semret = k_sem_take(lock, K_MSEC(timeout * retry_factor));
		if (semret == -EAGAIN) {
			/* did not receive acknowledgment re-send packet */
			retry_factor *= 2;
			retry_count++;
			LOG_WRN("timeout, retrying with %d ms wait period, retry number %d",
					timeout * retry_factor, retry_count);
			if (retry_count >= UART_M2M_RETRY_COUNT_MAX) {
				LOG_ERR("Maximum retries reached, abroting!");
				return -1;
			}
		}
	} while (semret != 0);
	return 0;
}

int app_net_send_data_resp(int frame_type, int cmd, char *data_buf, int data_len,
		int *tot_payload_sent, uint32_t *last_sequence, struct k_sem *lock)
{
	if (data_buf == NULL) {
		LOG_ERR("data_buf is NULL");
		return -EINVAL;
	}

	LOG_INF("data_resp: %d, %d, %d", frame_type, cmd, data_len);

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
			LOG_DBG("max_data_size = %d", max_data_size);

			/* the data buffer can be larger that the payload buffer, so
			 * check and send the appropriate number of bytes */
			if ((data_len - bytes_sent) > max_data_size) {
				to_copy = max_data_size;
			} else {
				to_copy = (data_len - bytes_sent);
			}
			LOG_DBG("data_len = %d, to_copy = %d, bytes_sent = %d", data_len, to_copy, bytes_sent);

			memcpy(resp_frame.payload + payload_idx, data_buf + bytes_sent, to_copy);
			payload_idx += to_copy;
//			payload_idx += sprintf((char*)(resp_frame.payload + payload_idx), "%s", M2M_CMD_PAYLOAD_TERM);
//			strcat((char*)(resp_frame.payload + payload_idx), M2M_CMD_PAYLOAD_TERM);
			bytes_sent += to_copy;

			/* add sequence number */
			resp_frame.sequence = seq++;
			resp_frame.payload_len = payload_idx;
			*last_sequence = resp_frame.sequence;	// this will be verified with the acknowledgment number

			LOG_DBG("payload_len = %d, sequence = %d", resp_frame.payload_len, resp_frame.sequence);

			/* compute checksum */
			ret = lib_m2m_frame_checksum_compute(&resp_frame);

			/* serialize and send */
			uint8_t *ser_buf = lib_m2m_frame_alloc_serialize(&resp_frame, &buf_len);
			if (ser_buf == NULL) {
				LOG_ERR("lib_m2m_frame_alloc_serialize failed %d", ret);
				return ret;
			}

			ret = buffer_send_wait_retry(ser_buf, buf_len, lock);
			free(ser_buf);
			if (ret < 0) goto err;

			*tot_payload_sent += resp_frame.payload_len;
		}
	} else if (frame_type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
		lib_m2m_frame_header_data_resp_end_make(&resp_frame);
		seq = 1;	// reset sequence number

		sprintf((char*)resp_frame.payload, "%d%s%s%s", cmd,
		M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_STREND,
		M2M_CMD_PAYLOAD_TERM);

		resp_frame.payload_len = strlen((char*)resp_frame.payload);

		/* add sequence number */
		resp_frame.sequence = ++(*last_sequence);

		LOG_DBG("payload_len = %d, sequence = %d", resp_frame.payload_len, resp_frame.sequence);

		/* compute checksum */
		ret = lib_m2m_frame_checksum_compute(&resp_frame);

		/* serialize and send */
		uint8_t *ser_buf = lib_m2m_frame_alloc_serialize(&resp_frame, &buf_len);
		if (ser_buf == NULL) {
			LOG_ERR("lib_m2m_frame_alloc_serialize failed %d", ret);
			return ret;
		}

		ret = buffer_send_wait_retry(ser_buf, buf_len, lock);
		free(ser_buf);
		if (ret < 0) goto err;

		LOG_INF("Sent STREND");

		/* reset sequence number */
		*last_sequence = 0;
	} else {
		LOG_ERR("incorrect frame type %d", frame_type);
		return -EINVAL;
	}

err:
//	free(ser_buf);
	return ret;
}
