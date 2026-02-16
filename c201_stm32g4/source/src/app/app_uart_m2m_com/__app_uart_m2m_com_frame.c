/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-May-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_uart_m2m);

#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"

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

int m2m_comm_frame_decode(uint8_t *sbuf, uint32_t len, struct m2m_frame_t *frame)
{
	if (sbuf == NULL)  {
		LOG_ERR("sbuf = NULL");
		return -EINVAL;
	}

	if (frame == NULL)  {
		LOG_ERR("frame == NULL");
		return -EINVAL;
	}

	if (len == 0)  {
		LOG_ERR("len == 0");
		return -EINVAL;
	}

	int ret = 0;
	int buf_ptr = 0;

	memcpy(&frame->sof, sbuf, sizeof(frame->sof));
	buf_ptr += sizeof(frame->sof);

	memcpy(&frame->type, sbuf + buf_ptr, sizeof (frame->type));
	buf_ptr += sizeof(frame->type);

	memcpy(&frame->sequence, sbuf + buf_ptr, sizeof(frame->sequence));
	buf_ptr += sizeof(frame->sequence);

	memcpy(&frame->ack, sbuf + buf_ptr, sizeof(frame->ack));
	buf_ptr += sizeof(frame->ack);

	memcpy(&frame->checksum, sbuf + buf_ptr, sizeof(frame->checksum));
	buf_ptr += sizeof(frame->checksum);

	memcpy(&frame->payload_len, sbuf + buf_ptr, sizeof(frame->payload_len));
	buf_ptr += sizeof(frame->payload_len);

	if ((frame->payload_len > 0) && (frame->payload_len <= UART_M2M_PAYLOAD_SIZE_MAX)) {
		memcpy(frame->payload, sbuf + buf_ptr, frame->payload_len);
		buf_ptr += frame->payload_len;
	} else {
		ret = -E2BIG;
	}

	if (ret == 0) {
		// verify checksum
		int ck = m2m_comm_frame_checksum_verify(frame);
		if (ck != 0) {
			LOG_ERR("checksum did not match!");
			ret = -EPROTO;
		}
	}

	return ret;
}

static void frame_common_attr_set(struct m2m_frame_t *frame)
{
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->sequence = 0;
	frame->ack = 0;
	frame->checksum = 0;
}

void m2m_comm_frame_header_single_req_make(struct m2m_frame_t *frame)
{
	frame->type = UART_M2M_FRAME_SINGLE_REQ;
	frame_common_attr_set(frame);
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

void m2m_comm_frame_header_data_req_make(struct m2m_frame_t *frame)
{
	frame->type = UART_M2M_FRAME_DATA_REQ;
	frame_common_attr_set(frame);
}

void m2m_comm_frame_header_data_resp_make(struct m2m_frame_t *frame)
{
	frame->type = UART_M2M_FRAME_DATA_RESP;
	frame_common_attr_set(frame);
}

uint8_t* m2m_comm_frame_alloc_serialize(struct m2m_frame_t *frame, size_t *buf_len)
{
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame->payload_len + 1;
	uint32_t sdata_len = 0;
	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("calloc failed at %s", __func__);
//		return -ENOMEM;
		return NULL;
	}
	m2m_comm_frame_serialize(serialized_buffer, sbuf_len, frame, &sdata_len);
	*buf_len = sbuf_len;

//	host_cmds_send_only((const char*)serialized_buffer, sdata_len);
//	ESP_LOG_BUFFER_HEXDUMP(TAG, serialized_buffer, sdata_len, ESP_LOG_WARN);
//	free(serialized_buffer);
	return serialized_buffer;
}

static uint8_t create_xor_checksum(uint8_t *in_buf, size_t len)
{
	if (in_buf == NULL)	return 0;

	uint8_t checksum = 0x00;
	for (size_t i=0; i<len; i++) {
		checksum = (checksum ^ in_buf[i]);
	}
	return checksum;
}

int m2m_comm_frame_checksum_verify(struct m2m_frame_t *frame)
{
	if (frame == NULL)  {
		LOG_ERR("frame == NULL");
		return -EINVAL;
	}

	int ret = 0;
	uint8_t in_checksum = frame->checksum;
	uint8_t checksum = 0x00;

	frame->checksum = 0x00;	// while computing checksum, the checksum field itself should be 0

	size_t len = 0;
	uint8_t *ser = m2m_comm_frame_alloc_serialize(frame, &len);
	if (ser == NULL) {
		LOG_ERR("checksum serialization failed!");
		return -1;
	}

	checksum = create_xor_checksum(ser, len);
	if (checksum == in_checksum) {
		LOG_INF("checksum matched");
		ret = 0;
	} else {
		LOG_INF("checksum did not match");
		ret = -1;
	}

	free(ser);
	return ret;
}

int m2m_comm_frame_checksum_compute(struct m2m_frame_t *frame)
{
	if (frame == NULL)  {
		LOG_ERR("frame == NULL");
		return -EINVAL;
	}

	frame->checksum = 0x00;	// while computing checksum, the checksum field itself should be 0

	size_t len = 0;
	uint8_t *ser = m2m_comm_frame_alloc_serialize(frame, &len);
	if (ser == NULL) {
		LOG_ERR("checksum serialization failed!");
		return -1;
	}

	frame->checksum = create_xor_checksum(ser, len);
	free(ser);
	return 0;
}

uint32_t m2m_comm_frame_ack_get(struct m2m_frame_t *frame)
{
	return 0;
}




