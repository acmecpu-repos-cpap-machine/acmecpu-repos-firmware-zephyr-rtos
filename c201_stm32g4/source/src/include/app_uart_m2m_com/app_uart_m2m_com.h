/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 28-Apr-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_H_

#define NEW_BUF_TEST	1
#define RING_BUF_TEST	0

#if RING_BUF_TEST
	#define RING_BUF_BYTES	(CONFIG_UART_M2M_BUFFER_SIZE * 64)
#endif

typedef enum {
	UART_M2M_APP_ID_WIFI_BT = 0,
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	UART_M2M_APP_ID_MOTOR_DRV,
#endif
	/*UART_M2M_APP_ID_LTE_MODEM,*/

	UART_M2M_APP_ID_MAX
} UART_M2M_APP_ID;

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
#define UART_M2M_APP_NUM	1
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
#define UART_M2M_APP_NUM	2
#endif

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
#define APP_TO_UART_MAP_INITIALIZER		{ \
	{UART_M2M_APP_ID_WIFI_BT, NULL}, \
}
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
#define APP_TO_UART_MAP_INITIALIZER		{ \
	{UART_M2M_APP_ID_WIFI_BT, NULL}, \
	{UART_M2M_APP_ID_MOTOR_DRV, NULL}, \
}
#endif

typedef void (*app_uart_m2m_cb_t)(void *rx_data, size_t len);

/**
 * @brief: 	Transmit data via uart.
 * @note:	First app_uart_m2m_init() then app_uart_m2m_configure() must be called
 *
 * @param:	uart_app_id	application id of the caller. The UART device will be mapped according to this ID
 * 			data the data to be transmitted
 * 			len length of the data in bytes
 *
 * @return:	0 for Success
 * 			-ENXIO no UART device configures, call app_uart_m2m_configure() first
 * */
int app_uart_m2m_send(UART_M2M_APP_ID uart_app_id, const void *data, int len);

/**
 * @brief: 	Configure the uart device, setup ISR callback and work queue to send received data back to the application
 * @note:	app_uart_m2m_init() must be called first
 *
 * @param:	uart_app_id	application id of the caller. The UART device will be mapped according to this ID
 * 			app_cb application's callback function where the received data needs to be sent
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int app_uart_m2m_configure(UART_M2M_APP_ID uart_app_id, app_uart_m2m_cb_t app_cb);

/**
 * @brief: 	Initialize the uart m2m library
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int app_uart_m2m_com_init();

#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_UART_M2M_COM_H_ */
