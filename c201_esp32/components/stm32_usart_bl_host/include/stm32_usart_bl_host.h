/*
 * Copyright (c) 2021 Acme CPU
 *
 * stm32_usart_bl_host.h
 * Created on: 22-Sep-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32_USART_BL_HOST_H_
#define COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32_USART_BL_HOST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

typedef int (*usart_open_t)(int baud_rate, int data_bits, int parity, int stop_bits, int flow_ctrl, int source_clk);
typedef int (*usart_send_t)(const char *data, size_t len);
typedef int (*usart_recv_t)();
typedef int (*usart_close_t)();
typedef void (*ms_delay_t)(uint32_t ms);

struct stm32_ubl_funcs {
	/**
	 * Function signature:
	 * int usart_open(int baud_rate, int data_bits, int parity, int stop_bits, int flow_ctrl, int source_clk)
	 *
	 * @brief: 	initializes the usart driver for stm32 bootloader communication
	 * @return:	0 for Success
	 * 			-ERRNO for failure
	 * */
	usart_open_t usart_open;

	/**
	 * Function signature:
	 * int usart_send(const char *data, size_t len)
	 *
	 * @brief: 	transmits one or more bytes of data to the usart driver
	 * @param:	data	pointer to the data to be transmitted through the usart
	 * 			len		length of the data being transmitted
	 * @return:	number of bytes transmitted successfully
	 * 			-ERRNO for failure
	 * */
	usart_send_t usart_send;

	/**
	 * Function signature:
	 * int usart_recv(void* buf, uint32_t length, uint32_t ms_to_wait)
	 *
	 * @brief: 	receive one or more bytes of data from the usart driver,
	 * 			optionally can wait for the data to become available
	 * @param:	buf		pointer to the buffer to receive the data from the usart driver
	 * 			length	length of the data being fetched
	 * 			ms_to_wait	number of ms to wait for the data to become available
	 * 						0xFFFFFFFF means wait forever
	 * @return:	number of bytes received
	 * */
	usart_recv_t usart_recv;

	usart_close_t usart_close;

	/**
	 * Function signature:
	 * void ms_delay(uint32_t ms_delay)
	 *
	 * @brief: 	blocking delay routine
	 *
	 * @param:	ms_delay	delay duration in milli seconds
	 * */
	ms_delay_t ms_delay;

};

/**
 * @brief: 	initializes the stm32 usart bootloader host library.
 * 			This must be called before calling any other functions of this library
 *
 * @param:	stm32_ubl	pointer to struct stm32_ubl_funcs having function pointers to the systems's usart driver functions
 * 						must be initialized by the caller
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_init(struct stm32_ubl_funcs *stm32_ubl);

/**
 * @brief: 	Opens the usart driver for communication
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_usart_open();

/**
 * @brief: This function starts the communication with an STM32 processor that is already in the bootloader mode
 * 			It sends the start byte 0x7F and waits for an ACK 0x79. The function retries for 10 times every 500ms
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_start_check();

/**
 * @brief: Sends STM32 bootloader's GET command to the device, receives the response and validates it
 * @param	p_resp[out]		pointer to array where the response bytes of the GET command will be copied. Must not be NULL
 * 			p_len[out]		number of bytes of the response
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_cmd_execute_get(uint8_t *p_resp, size_t *p_len);

/**
 * @brief: Sends STM32 bootloader's GET VERSION command to the device, receives the response
 * @param	bl_ver[out]		bootloader version
 * 			ob_1[out]		option byte 1
 * 			ob_2[out]		option byte 2
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_cmd_execute_get_version(uint8_t *bl_ver, uint8_t *ob_1, uint8_t *ob_2);

/**
 * @brief: Sends STM32 bootloader's GO command to the device and receives acknowledgment
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_ubl_cmd_execute_go();

/**
 * @brief: Sends STM32 bootloader's READ Memory command to the device and receives data
 * @param	start_addr[in]	start_addr from where the data has to be read (byte 1 is the address MSB and byte 4 is the LSB)
 * @param	buf[out]		buffer where the read data will be stored
 * @param	bytes_to_read[in]	number of bytes to be read (this value must <= 256)
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_uble_cmd_execute_read_mem(uint8_t *start_addr, uint8_t *buf, size_t bytes_to_read);

/**
 * @brief: Sends STM32 bootloader's WRITE command to the device, writes the data and receives acknowledgment
 * @param	start_addr[in]	start_addr of the write command (byte 1 is the address MSB and byte 4 is the LSB)
 * 			buf[in]		pointer to data to be written
 * 			bytes_to_write[in]			number of bytes to write (this value must <= 256)
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_uble_cmd_execute_write_mem(uint8_t *start_addr, uint8_t *buf, size_t bytes_to_write);

/**
 * @brief: Sends STM32 bootloader's ERASE command to the device, erases the flashs page wise / mass and receives acknowledgment
 * @param	num_pages[in]		the number of pages to erase (ignored if mass_erase = 1)
 * 			start_page[in]		page number start (ignored if mass_erase = 1)
 * 			end_page[in]		page number end (ignored if mass_erase = 1)
 * 			mass_erase[in]		if mass_erase = 1, the whole flash is erased
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_uble_cmd_execute_erase_mem(uint8_t num_pages, uint8_t start_page, uint8_t end_page, uint8_t mass_erase);

/**
 * @brief: Sends STM32 bootloader's EXTENDED ERASE command to the device
 * @param	nop[in]			2 bytes the number of pages to erase (ignored if spl_erase != 0)
 * 			pgnos[in]		page numbers to erase passed as an array of uint8_t (ignored if spl_erase != 0)
 * 			spl_erase[in]			special erase, value can be 0xFFFF, 0xFFFE, 0xFFFD or 0x0000
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int stm32_uble_cmd_execute_ext_erase_mem(uint16_t nop, uint8_t *pgnos, uint8_t *spl_erase);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32_USART_BL_HOST_H_ */
