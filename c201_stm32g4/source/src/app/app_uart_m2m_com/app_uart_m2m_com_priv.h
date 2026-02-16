/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 28-Apr-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_PRIV_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_PRIV_H_

#include "acpu_c201_modules.h"
#include "app_uart_m2m_com/app_uart_m2m_com.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif

#include <zephyr/sys/ring_buffer.h>

#define TRANSACT_SOF_DETECT		0
#define TRANSACT_SINGLE_BYTE	1
#define TRANSACT_BULK			2

struct app_uart_map {
	/* UART application ID */
	UART_M2M_APP_ID app_id;

	/* UART device name */
//	const char *dev_name;

	/* UART device */
	const struct device *dev;

	/* application callback to send rx data */
	app_uart_m2m_cb_t app_cb;
};

#if NEW_BUF_TEST
struct app_copy_buf {
	/* Buffer pointer to be used to allocate and copy the received data, this will be used by the application */
	char *buf;

	/* Buffer length to be used by the application */
	uint32_t len;
};
#endif

struct bsp_uart_serial_config {
	/* application and uart mapping data */
	struct app_uart_map *au_map;

	/* timeout (maximum request-response delay) */
	uint32_t timeout;

	/* UART m2m work item */
	struct k_work m2m_work;

	/* data copy semaphore */
	struct k_sem copy_sem;

	/* Pointer to current position in buffer */
	uint8_t *uart_buf_ptr;

	/* transaction type single byte of bulk */
	uint8_t tx_type;

	/* bulk size */
	uint32_t bulk_sz;

	/* Number of bytes received or to send */
	uint16_t uart_buf_ctr;

	/* Storage of received characters or characters to send */
	uint8_t uart_buf[UART_M2M_FRAME_SIZE_MAX];
#if NEW_BUF_TEST
	/* Data structure to store the received uart data and pass to the application */
	struct app_copy_buf app_buf;
#endif
#if RING_BUF_TEST
	struct ring_buf rb;
	uint8_t rbuffer[RING_BUF_BYTES];
#endif
};

#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_PRIV_H_ */
