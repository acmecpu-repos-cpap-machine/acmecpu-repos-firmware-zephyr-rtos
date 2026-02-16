/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef __UART_M2M_COMM_H
#define __UART_M2M_COMM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_uart.h"
#include "stm32f0xx_ll_usart.h"

#include "uart_m2m_comm_config.h"
#include "uart_m2m_comm_frame.h"

#define NO_UART	1

#define TRANSACT_SINGLE_BYTE	0
#define TRANSACT_BULK			1

struct uart_m2m_data {
	/* application and uart mapping data */
//	struct app_uart_map *au_map;

	/* timeout (maximum request-response delay) */
//	uint32_t timeout;

	/* UART m2m work item */
//	struct k_work m2m_work;

	/* data copy semaphore */
//	struct k_sem copy_sem;

	/* Pointer to current position in buffer */
	uint8_t is_first_char;

	/* Pointer to current position in buffer */
	uint8_t *uart_buf_ptr;

	/* transaction type single byte of bulk */
	uint8_t tx_type;

	/* bulk size */
	uint32_t bulk_sz;

	/* Number of bytes received or to send */
	uint16_t uart_buf_ctr;

	/* Storage of received characters or characters to send */
	uint8_t uart_buf[CONFIG_UART_M2M_BUFFER_SIZE];
};

void uart_m2m_comm_rx_handler(void);
void uart_m2m_comm_set_app_buf(uint8_t *pbuf, uint32_t *plen);

#ifdef __cplusplus
}
#endif

#endif /* __UART_M2M_COMM_H */
