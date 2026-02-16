/*
 * Copyright (c) 2021 Acme CPU
 *
 * m2m_frame.c
 * Created on: 19-May-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "esp_log.h"
#include "m2m_frame.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"

#define TAG	"m2m_frame"

void m2m_comm_frame_header_single_req_make(struct m2m_frame_t *frame) {
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_SINGLE_REQ;
	frame->sequence = 0;
}

void m2m_comm_frame_header_single_resp_make(struct m2m_frame_t *frame)
{
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_SINGLE_RESP;
	frame->sequence = 0;
}

void m2m_comm_frame_header_stream_req_make(struct m2m_frame_t *frame)
{
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_STREAM_REQ;
	frame->sequence = 0;
}

void m2m_comm_frame_header_stream_resp_make(struct m2m_frame_t *frame)
{
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_STREAM_RESP;
	frame->sequence = 0;
}

int m2m_comm_frame_serialize(uint8_t *sbuf, uint32_t sbuf_len, struct m2m_frame_t *frame, uint32_t *sdata_len)
{
	int ret = 0;

	if ((sbuf == NULL) || (frame == NULL) || (sdata_len == NULL))
		return -EINVAL;

	int buf_ptr = 0;
	memset(sbuf, 0, sbuf_len);

	memcpy(sbuf, &frame->sof, sizeof(frame->sof));
	buf_ptr += sizeof(frame->sof);

	memcpy(sbuf + buf_ptr, &frame->type, sizeof(frame->type));
	buf_ptr += sizeof(frame->type);

	memcpy(sbuf + buf_ptr, &frame->sequence, sizeof(frame->sequence));
	buf_ptr += sizeof(frame->sequence);

	memcpy(sbuf + buf_ptr, &frame->payload_len, sizeof(frame->payload_len));
	buf_ptr += sizeof(frame->payload_len);

	memcpy(sbuf + buf_ptr, frame->payload, frame->payload_len);
	buf_ptr += frame->payload_len;

	/* copy the serialized buffer length */
	*sdata_len = buf_ptr;

	return ret;
}

int m2m_comm_frame_serialize_and_send(struct m2m_frame_t *frame)
{
	if (frame->payload_len > UART_M2M_PACKET_SIZE_MAX) {
		ESP_LOGE(TAG, "incorrect frame->payload_len = %ld", frame->payload_len);
		return -EINVAL;
	}
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame->payload_len + 1;
	uint32_t sdata_len = 0;

	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		ESP_LOGE(TAG, "calloc failed at %s", __func__);
		return -ENOMEM;
	}

	ESP_LOGD(TAG, "sbuf_len = %ld", sbuf_len);

	m2m_comm_frame_serialize(serialized_buffer, sbuf_len, frame, &sdata_len);

	ESP_LOGD(TAG,
			"sending stream response: payload_len = %ld, sequence = %ld, sdata_len = %ld",
			frame->payload_len, frame->sequence, sdata_len);

	host_cmds_send_only((const char*)serialized_buffer, sdata_len);

//	ESP_LOG_BUFFER_HEXDUMP(TAG, serialized_buffer, sdata_len, ESP_LOG_DEBUG);
	ESP_LOG_BUFFER_HEXDUMP(TAG, serialized_buffer, UART_M2M_HEADER_SIZE_MAX, ESP_LOG_WARN);

	free(serialized_buffer);
	return 0;
}
