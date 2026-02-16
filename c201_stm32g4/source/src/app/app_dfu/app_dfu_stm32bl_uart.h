/*
 * Copyright (c) 2024 Acme CPU
 *
 * Created on: 4-Jul-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DFU_APP_DFU_STM32BL_UART_H_
#define SRC_APP_APP_DFU_APP_DFU_STM32BL_UART_H_

#include <stdint.h>

/**
 * @brief	delay function
 * @param ms_delay	delay in ms to sleep
 */
void app_dfu_stm32bl_uart_ms_delay(uint32_t ms_delay);

/**
 * @brief	get the current size of the uart rx fifo
 * @return
 * 		size of the the rx fifo
 */
uint32_t app_dfu_stm32bl_uart_rx_fifo_size_get();

/**
 * @brief	writes byte to the uart interface by polling.
 * 			This is a blocking function
 * @param data	the buffer containing Tx data
 * @param len	the number of bytes to write
 * @return
 * 			0
 */
int app_dfu_stm32bl_uart_write_bytes(const void *data, int len);

/**
 * @brief reads data from the uart receive fifo
 * @param buf			buffer to stored the read data
 * @param length		number of bytes to read
 * @param ms_to_wait	ms to wait if data is not available
 * 						0xFFFFFFFF means wait forever
 * @return
 * 		the number of bytes read if success
 * 		-ve number if failed
 */
int app_dfu_stm32bl_uart_read_bytes(void* buf, int length, int ms_to_wait);

int app_dfu_stm32bl_uart_open(int baud_rate, int data_bits, int parity, int stop_bits,
		int flow_ctrl, int source_clk);
int app_dfu_stm32bl_uart_close();

/**
 * @brief	initialize the uart interface connected to the STM32 device
 * @return
 * 		0 if success
 * 		-ve if failed
 */
int app_dfu_stm32bl_uart_configure();

#endif /* SRC_APP_APP_DFU_APP_DFU_STM32BL_UART_H_ */

