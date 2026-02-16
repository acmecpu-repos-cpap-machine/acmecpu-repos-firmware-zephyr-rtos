/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_sensor.c
 * Created on: 11-Aug-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include <sys/errno.h>

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"host_cmds"

int host_cmds_sensor_list_get() {
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);
	sprintf((char*) frame.payload, "%d%s%s%s", C20X_M2M_CMD_ID_SENSOR_LIST, M2M_CMD_PAYLOAD_DELIM,
			M2M_CMD_PAYLOAD_GET_CHAR, M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((const char *)frame.payload);
#if 0
	/* serialize the frame and send response */
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

int host_cmds_sensor_value_getone(uint8_t *sens_info /* channel no, sensor id (2 bytes fixed length) */) {
#define SENS_INFO_LEN	2

	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);
	sprintf((char*) frame.payload, "%d%s", C20X_M2M_CMD_ID_SENSOR_GET, M2M_CMD_PAYLOAD_DELIM);
	frame.payload_len = strlen((const char *)frame.payload);

	memcpy(frame.payload + frame.payload_len, sens_info, SENS_INFO_LEN);
	memcpy(frame.payload + frame.payload_len + SENS_INFO_LEN, M2M_CMD_PAYLOAD_TERM, 1);

	frame.payload_len = strlen((const char *)frame.payload);
#if 0
	/* serialize the frame and send response */
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

int host_cmds_sensor_value_getall() {
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);
	sprintf((char*) frame.payload, "%d%s%s%s", C20X_M2M_CMD_ID_SENSOR_GETALL, M2M_CMD_PAYLOAD_DELIM,
			M2M_CMD_PAYLOAD_GET_CHAR, M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((const char *)frame.payload);
#if 0
	/* serialize the frame and send response */
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


