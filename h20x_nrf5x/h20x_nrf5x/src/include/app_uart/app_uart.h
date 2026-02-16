/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 7-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_UART_APP_UART_H_
#define SRC_INCLUDE_APP_UART_APP_UART_H_


void app_uart_get_and_print();

/**
 * @brief
 *      Initialize the uart interface for m2m communication
 * 		
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_uart_m2m_com_init();


#endif  /*SRC_INCLUDE_APP_UART_APP_UART_H_*/