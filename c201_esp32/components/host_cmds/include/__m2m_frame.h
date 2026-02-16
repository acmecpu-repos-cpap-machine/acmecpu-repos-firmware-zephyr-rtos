/*
 * Copyright (c) 2021 Acme CPU
 *
 * m2m_frame.h
 * Created on: 19-May-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_INCLUDE_M2M_FRAME_H_
#define COMPONENTS_HOST_CMDS_INCLUDE_M2M_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sdkconfig.h"

#define UART_M2M_PACKET_SIZE_MAX		CONFIG_UART_M2M_BUFFER_SIZE
#define UART_M2M_HEADER_SIZE_MAX		10
#define UART_M2M_PAYLOAD_SIZE_MAX	(UART_M2M_PACKET_SIZE_MAX - UART_M2M_HEADER_SIZE_MAX)
#define UART_M2M_START_OF_FRAME		0xAC	/* start of frame */

typedef enum {
	UART_M2M_FRAME_SINGLE_REQ=0,
	UART_M2M_FRAME_SINGLE_RESP,
	UART_M2M_FRAME_STREAM_REQ,
	UART_M2M_FRAME_STREAM_RESP,
	UART_M2M_FRAME_STREAM_RESP_ENDSTR,

	UART_M2M_FRAME_MAX
} UART_M2M_FRAME_TYPE;

struct m2m_frame_t {
	uint8_t sof;
	uint8_t type;
	uint32_t sequence;
	uint32_t payload_len;
	uint8_t payload[UART_M2M_PAYLOAD_SIZE_MAX];
};

void m2m_comm_frame_header_single_req_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_single_resp_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_stream_req_make(struct m2m_frame_t *frame);

void m2m_comm_frame_header_stream_resp_make(struct m2m_frame_t *frame);

int m2m_comm_frame_serialize(uint8_t *sbuf, uint32_t sbuf_len, struct m2m_frame_t *frame, uint32_t *sdata_len);

/**
 * @brief	Seializes and sends data to host processor
 * @param out_frame		frame to serialize and send
 * @return
 * 	0 		success
 * 	-ENOMEM	out of memory
 */
int m2m_comm_frame_serialize_and_send(struct m2m_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_INCLUDE_M2M_FRAME_H_ */
