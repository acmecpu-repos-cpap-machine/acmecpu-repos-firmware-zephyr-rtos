/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdlib.h>
#include <string.h>
#include "uart_m2m_comm.h"

/* variable to indicate that an rx frame has been received and it should be processed */
__IO uint8_t g_uart_m2m_comm_data_available = 0;

static struct uart_m2m_data m_udata = {.is_first_char = 1, .tx_type = TRANSACT_SINGLE_BYTE, .uart_buf_ptr = 0};
static uint8_t *m_pbuf = NULL;
static uint32_t *m_plen = 0;

void uart_m2m_comm_rx_handler(void)
{
	uint8_t c;

	if (m_udata.tx_type == TRANSACT_SINGLE_BYTE) {
		c = LL_USART_ReceiveData8(USART1);

		if (m_udata.is_first_char) {
			if (c == UART_M2M_START_OF_FRAME) {
				/* Restart a new frame */
				m_udata.is_first_char = 0;
				m_udata.uart_buf_ptr = &m_udata.uart_buf[0];
				m_udata.uart_buf_ctr = 0;
			} else {
				/* if the first char is not a SOF, ignore the entire packet */
				return;
			}
		}

		if (m_udata.uart_buf_ctr < CONFIG_UART_M2M_BUFFER_SIZE) {
			*m_udata.uart_buf_ptr++ = c;
			m_udata.uart_buf_ctr++;
		}

		if (m_udata.uart_buf_ctr == UART_M2M_HEADER_SIZE_MAX) {
			/* header received, now get the payload */
			m_udata.tx_type = TRANSACT_BULK;
			memcpy(&m_udata.bulk_sz, (m_udata.uart_buf_ptr - sizeof(m_udata.bulk_sz)), sizeof(m_udata.bulk_sz));
		}

	} else if (m_udata.tx_type == TRANSACT_BULK) {
		/* we can read only one byte at a time here
		 * drivers that support reading multiple bytes at a time can replace the LL_USART_ReceiveData8 line
		 * and modify the logic
		 * */
		uint8_t n = 1;

		/* read 1 byte from the USART rx fifo and copy into the library's buffer */
		*m_udata.uart_buf_ptr = LL_USART_ReceiveData8(USART1);

		/* increment buffer pointers */
		m_udata.uart_buf_ptr += n;
		m_udata.uart_buf_ctr += n;

		/* calculate balance data to be read */
		m_udata.bulk_sz = m_udata.bulk_sz - n;

		if (m_udata.bulk_sz == 0) {
			LL_USART_DisableIT_RXNE(USART1);

			/* end of transaction */
			m_udata.tx_type = TRANSACT_SINGLE_BYTE;

			/* copy the data into application's buffer */
			memset(m_pbuf, 0x00, CONFIG_UART_M2M_BUFFER_SIZE);
			memcpy(m_pbuf, m_udata.uart_buf, m_udata.uart_buf_ctr);
			*m_plen = m_udata.uart_buf_ctr;

			/* the next char we receive should be start of a new packet */
			m_udata.is_first_char = 1;

			/* inform application to process the data */
			g_uart_m2m_comm_data_available = 1;
		}
	}
}

void uart_m2m_comm_set_app_buf(uint8_t *pbuf, uint32_t *plen)
{
	m_pbuf = pbuf;
	m_plen = plen;
}
