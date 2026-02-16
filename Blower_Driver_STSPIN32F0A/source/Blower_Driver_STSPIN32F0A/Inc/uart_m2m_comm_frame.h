/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef __UART_M2M_COMM_FRAME_H
#define __UART_M2M_COMM_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "uart_m2m_comm_config.h"

#define UART_M2M_FRAME_SIZE_MAX		CONFIG_UART_M2M_BUFFER_SIZE
#define UART_M2M_HEADER_SIZE_MAX		10
#define UART_M2M_PAYLOAD_SIZE_MAX		(UART_M2M_FRAME_SIZE_MAX - UART_M2M_HEADER_SIZE_MAX)
#define UART_M2M_START_OF_FRAME			0xAC	/* start of frame */

typedef enum {
	UART_M2M_FRAME_SINGLE_REQ=0,
	UART_M2M_FRAME_SINGLE_RESP,
	UART_M2M_FRAME_STREAM
} UART_M2M_FRAME_TYPE;

struct m2m_frame_t {
	uint8_t sof;
	uint8_t type;
	uint32_t sequence;
	uint32_t payload_len;
	uint8_t payload[UART_M2M_PAYLOAD_SIZE_MAX];
};


int m2m_comm_frame_serialize(uint8_t *sbuf, uint32_t sbuf_len, struct m2m_frame_t *frame, uint32_t *sdata_len);

int m2m_com_frame_decode(uint8_t *sbuf, uint32_t len, struct m2m_frame_t *frame);



#ifdef __cplusplus
}
#endif

#endif /* __UART_M2M_COMM_FRAME_H */
