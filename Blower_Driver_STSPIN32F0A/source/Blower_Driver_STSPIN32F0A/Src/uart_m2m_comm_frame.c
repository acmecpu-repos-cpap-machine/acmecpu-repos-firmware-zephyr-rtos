/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart_m2m_comm_frame.h"

int m2m_comm_frame_serialize(uint8_t *sbuf, uint32_t sbuf_len, struct m2m_frame_t *frame, uint32_t *sdata_len)
{
	int ret = 0;

//	if ((sbuf == NULL) || (frame == NULL) || (sdata_len == NULL))
//		return -1;

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

int m2m_com_frame_decode(uint8_t *sbuf, uint32_t len, struct m2m_frame_t *frame)
{
//	if (sbuf == NULL || frame == NULL || len == 0)
//		return -1;

	int ret = 0;
	int buf_ptr = 0;

	memcpy(&frame->sof, sbuf, sizeof(frame->sof));
	buf_ptr += sizeof(frame->sof);

	memcpy(&frame->type, sbuf + buf_ptr, sizeof (frame->type));
	buf_ptr += sizeof(frame->type);

	memcpy(&frame->sequence, sbuf + buf_ptr, sizeof(frame->sequence));
	buf_ptr += sizeof(frame->sequence);

	memcpy(&frame->payload_len, sbuf + buf_ptr, sizeof(frame->payload_len));
	buf_ptr += sizeof(frame->payload_len);

	if ((frame->payload_len > 0) && (frame->payload_len < UART_M2M_PAYLOAD_SIZE_MAX)) {
		memcpy(frame->payload, sbuf + buf_ptr, frame->payload_len);
		buf_ptr += frame->payload_len;
	} else {
		ret = -1;
	}

	return ret;
}


