/*
 * Copyright (c) 2021 Acme CPU
 *
 * stm32_usart_bl_host.c
 * Created on: 22-Sep-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#define DEBUG_LOG			0
#define PLATFORM_ESP_IDF	1

#include <stdio.h>
#include <string.h>
#include "stm32_usart_bl_host.h"

#if PLATFORM_ESP_IDF
#include "driver/uart.h"
#include "esp_log.h"
#endif

#define TAG	"stm32_ubl"

#define MAX_DELAY		0xFFFFFFFF
#define READY			0x01
#define NOT_READY		0x00

#define START_BYTE		0x7F
#define ACK_BYTE		0x79
#define NACK_BYTE		0x1F

#define STM32_APP_START_ADDR	{0x08, 0x00, 0x00, 0x00}	//0x08000000 //CONFIG_STM32_APP_START_ADDR

/* Bootloader command codes */
#define CMD_GET				0x00
#define CMD_GET_VER			0x01
#define CMD_GO				0x21
#define CMD_READ_MEM		0x11
#define CMD_WRITE_MEM		0x31
#define CMD_ERASE_MEM		0x43
#define CMD_EXT_ERASE_MEM	0x44


static struct stm32_ubl_funcs m_ubl_comm = { NULL, NULL, NULL, NULL, NULL };
static int m_stm32_ubl_ready = NOT_READY;


/* Static functions */
static uint8_t create_xor_checksum(uint8_t *in_buf, size_t len) {
	if (in_buf == NULL)	return 0;

	uint8_t checksum = 0x00;
	for (size_t i=0; i<len; i++) {
		checksum = (checksum ^ in_buf[i]);
	}
	return checksum;
}

static int wait_for_ack() {
	uint8_t rxbyte = 0x00;
	int rx_num = m_ubl_comm.usart_recv(&rxbyte, sizeof(rxbyte), MAX_DELAY);
	if (rx_num != sizeof(rxbyte)) {
		printf("usart_recv failed, rx_num = %d\n", rx_num);
		return -1;
	}
	if (rxbyte == ACK_BYTE) {
		return 0;
	}
	else if (rxbyte == NACK_BYTE) {
		printf("NACK_BYTE received\n");
		return -2;
	}
	else 							return -1;
}

static int check_lib_status() {
	if ((m_ubl_comm.usart_send == NULL) || (m_ubl_comm.usart_recv == NULL) || (m_ubl_comm.ms_delay == NULL)) {
		return -1;
	}

	if (m_stm32_ubl_ready != READY) {
		return -1;
	}

	return 0;
}


/* Global functions */

/********************************************************************************************/
/*********************************** INIT BOOTLOADER HOST ***********************************/
/********************************************************************************************/
int stm32_ubl_init(struct stm32_ubl_funcs *stm32_ubl) {
	int ret = 0;
	m_stm32_ubl_ready = NOT_READY;

	m_ubl_comm.usart_open = stm32_ubl->usart_open;
	m_ubl_comm.usart_send = stm32_ubl->usart_send;
	m_ubl_comm.usart_recv = stm32_ubl->usart_recv;
	m_ubl_comm.usart_close = stm32_ubl->usart_close;
	m_ubl_comm.ms_delay = stm32_ubl->ms_delay;

	return ret;
}

int stm32_ubl_usart_open() {
	int ret = -1;

	if (m_ubl_comm.usart_open != NULL) {
#if PLATFORM_ESP_IDF
		ret = m_ubl_comm.usart_open(115200, UART_DATA_8_BITS, UART_PARITY_EVEN, UART_STOP_BITS_1,
				UART_HW_FLOWCTRL_DISABLE, UART_SCLK_APB);
#endif
	}

	return ret;
}


/********************************************************************************************/
/************************************* START BOOTLOADER *************************************/
/********************************************************************************************/
int stm32_ubl_start_check() {
	if ((m_ubl_comm.usart_send == NULL) || (m_ubl_comm.usart_recv == NULL) || (m_ubl_comm.ms_delay == NULL)) {
		return -1;
	}

	uint8_t txdata = START_BYTE;
	const uint8_t rxdata = ACK_BYTE;
	size_t len = sizeof(txdata);

	int ret = 0, rx_bytes = 0, tx_bytes = 0;
	uint8_t buf = 0x00;

	int retry = 0;
	while (1) {
		if (retry > 10) {
			printf("DFU communication failed\n");
			break;
		}
		tx_bytes = m_ubl_comm.usart_send((const char*) &txdata, len);
		if (tx_bytes == len) {
			rx_bytes = m_ubl_comm.usart_recv(&buf, sizeof(buf), 500);
			if (rx_bytes != sizeof(buf)) {
				++retry;
				printf("usart_recv failed, rx_bytes = %d\n", rx_bytes);
				ret = -1;
				m_ubl_comm.ms_delay(500);
			} else {
				if (buf == rxdata) {
					m_stm32_ubl_ready = READY;
					printf("Connectivity to STM32 Bootloader is successful\n");
#if PLATFORM_ESP_IDF
					ESP_LOG_BUFFER_HEXDUMP(TAG, &buf, sizeof(buf), ESP_LOG_WARN);
#endif
					break;
				} else {
					++retry;
					printf("Connectivity to STM32 Bootloader failed\n");
#if PLATFORM_ESP_IDF
					ESP_LOG_BUFFER_HEXDUMP(TAG, &buf, sizeof(buf), ESP_LOG_WARN);
#endif
					ret = -1;
				}
			}
		} else {
			printf("usart_send failed, tx_bytes = %d\n", tx_bytes);
			ret = -1;
			break;
		}
	}

	return ret;
}


/* ********************************** BOOTLOADER COMMANDS ***********************************/

/********************************************************************************************/
/**************************************** GET COMMAND ***************************************/
/********************************************************************************************/
int stm32_ubl_cmd_execute_get(uint8_t *p_resp, size_t *p_len) {
	if (check_lib_status() != 0)
		return -1;

	if (p_resp == NULL)
		return -1;
	*p_len = 0;

	uint8_t txdata[2] = { 0x00 };
	size_t len = sizeof(txdata);

	int ret = 0, tx_num = 0;
	uint8_t rxbyte = 0x00;

	uint8_t bytes_to_follow = 0;
	uint8_t rxbuf[128] = { 0x00 };

	txdata[0] = CMD_GET;
	txdata[1] = ~CMD_GET;
	printf("transmitting [0x%x][0x%x]\n", txdata[0], txdata[1]);

	tx_num = m_ubl_comm.usart_send((const char*) &txdata, len);
	if (tx_num == len) {
		if (wait_for_ack() != 0)
			return -1; /* BYTE ACK */

		memcpy(p_resp + *p_len, &rxbyte, sizeof(rxbyte));
		*p_len += sizeof(rxbyte);
#if DEBUG_LOG
			printf("B1 ACK = 0x%x\n", rxbyte);
#endif
		/* BYTE 2: N (number of bytes to follow - 1 except current and ACKs */
		m_ubl_comm.usart_recv(&rxbyte, sizeof(rxbyte), MAX_DELAY);
		memcpy(p_resp + *p_len, &rxbyte, sizeof(rxbyte));
		*p_len += sizeof(rxbyte);
#if DEBUG_LOG
			printf("B2 N = 0x%x\n", rxbyte);
#endif
		/* BYTE 3: to n */
		bytes_to_follow = rxbyte + 1;
		m_ubl_comm.usart_recv(rxbuf, bytes_to_follow, MAX_DELAY);
		memcpy(p_resp + *p_len, rxbuf, bytes_to_follow);
		*p_len += bytes_to_follow;
		for (uint8_t i = 0; i < bytes_to_follow; i++) {
#if DEBUG_LOG
				printf("B%d = 0x%x\n", i + 3, rxbuf[i]);
#endif
		}

		if (wait_for_ack() != 0)
			return -1; /* BYTE ACK */

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}


/********************************************************************************************/
/************************************ GET VERSION COMMAND ***********************************/
/********************************************************************************************/
int stm32_ubl_cmd_execute_get_version(uint8_t *bl_ver, uint8_t *ob_1, uint8_t *ob_2) {
	if ( check_lib_status() != 0 )	return -1;

	int ret = 0, tx_num = 0;

	/* Command to be transmitted and its complement */
	uint8_t cmd[2] = { 0x00 };
	cmd[0] = CMD_GET_VER;
	cmd[1] = ~CMD_GET_VER;
	uint8_t len = sizeof(cmd);
	printf("transmitting [0x%x][0x%x]\n", cmd[0], cmd[1]);

	/* BYTE 1 and 2: 0x11 and 0xEE */
	tx_num = m_ubl_comm.usart_send((const char*) &cmd, len);
	if (tx_num == len) {
		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* Receive data */
		m_ubl_comm.usart_recv(bl_ver, 1, MAX_DELAY);
		m_ubl_comm.usart_recv(ob_1, 1, MAX_DELAY);
		m_ubl_comm.usart_recv(ob_2, 1, MAX_DELAY);

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */
	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}
	return ret;
}


/********************************************************************************************/
/**************************************** GO COMMAND ****************************************/
/********************************************************************************************/
int stm32_ubl_cmd_execute_go() {
	if (check_lib_status() != 0)
		return -1;

	uint8_t txdata[2] = { 0x00 };
	uint8_t start_addr[4] = STM32_APP_START_ADDR;
	uint8_t checksum = create_xor_checksum(start_addr, sizeof(start_addr));

	size_t len = 0;

	int ret = 0,
	tx_num = 0;
	uint8_t rxbyte = 0x00;

	/* Command to be transmitted and its complement */
	txdata[0] = CMD_GO;
	txdata[1] = ~CMD_GO;
	len = sizeof(txdata);
	printf("transmitting [0x%x][0x%x]\n", txdata[0], txdata[1]);

	/* BYTE 1 and 2: 0x21 and 0xDE */
	tx_num = m_ubl_comm.usart_send((const char*) &txdata, len);
	if (tx_num == len) {
		if (wait_for_ack() != 0)
			return -1; /* BYTE ACK */

		/* if we receive some data, it should be ACK or NACK */
		printf("B1 ACK = 0x%x\n", rxbyte);

		/* BYTE 3 to 6: start address */
		tx_num = m_ubl_comm.usart_send((const char*) start_addr, sizeof(start_addr));
		if (tx_num != sizeof(start_addr)) {
			printf("usart_send start_addr failed, tx_num = %d\n", tx_num);
			return -1;
		}

		/* BYTE 7: checksum */
		tx_num = m_ubl_comm.usart_send((const char*) &checksum, sizeof(checksum));
		if (tx_num != sizeof(checksum)) {
			printf("usart_send start_addr checksum, tx_num = %d\n", tx_num);
			return -1;
		}

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}


/********************************************************************************************/
/********************************** READ MEMORY COMMAND *************************************/
/********************************************************************************************/
int stm32_uble_cmd_execute_read_mem(uint8_t *start_addr, uint8_t *buf, size_t bytes_to_read) {
	if ( check_lib_status() != 0 )	return -1;

	int ret = 0, rx_num = 0, tx_num = 0;

	/* Command to be transmitted and its complement */
	uint8_t cmd[2] = { 0x00 };
	cmd[0] = CMD_READ_MEM;
	cmd[1] = ~CMD_READ_MEM;
	uint8_t len = sizeof(cmd);
	printf("transmitting [0x%x][0x%x]\n", cmd[0], cmd[1]);

	/* BYTE 1 and 2: 0x11 and 0xEE */
	tx_num = m_ubl_comm.usart_send((const char*) &cmd, len);
	if (tx_num == len) {
		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* BYTE 3 to 6: start address */
		tx_num = m_ubl_comm.usart_send((const char*) start_addr, 4);
		if (tx_num != 4) {
			printf("usart_send start_addr failed, tx_num = %d\n", tx_num);
			return -1;
		}

		/* BYTE 7: checksum */
		uint8_t checksum = create_xor_checksum(start_addr, 4);
		tx_num = m_ubl_comm.usart_send((const char*) &checksum, sizeof(checksum));
		if (tx_num != sizeof(checksum)) {
			printf("usart_send start_addr checksum, tx_num = %d\n", tx_num);
			return -1;
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* BYTE 8 and 9: number of bytes to be read and its checksum */
		uint8_t nob[2];
		nob[0] = bytes_to_read - 1U;
		nob[1] = nob[0] ^ 0xFFU;

		tx_num = m_ubl_comm.usart_send((const char*) &nob, sizeof(nob));
		if (tx_num != sizeof(nob)) {
			printf("usart_send start_addr checksum, tx_num = %d\n", tx_num);
			return -1;
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* Receive data */
		rx_num = m_ubl_comm.usart_recv(buf, bytes_to_read, MAX_DELAY);
		if (rx_num != bytes_to_read) {
			printf("usart_recv failed, rx_num = %d\n", rx_num);
			return -1;
		}

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}


/********************************************************************************************/
/********************************** WRITE MEMORY COMMAND ************************************/
/********************************************************************************************/
int stm32_uble_cmd_execute_write_mem(uint8_t *start_addr, uint8_t *buf, size_t bytes_to_write) {
	if ( check_lib_status() != 0 )	return -1;

	int ret = 0, tx_num = 0;

	/* Command to be transmitted and its complement */
	uint8_t cmd[2] = { 0x00 };
	cmd[0] = CMD_WRITE_MEM;
	cmd[1] = ~CMD_WRITE_MEM;
	uint8_t len = sizeof(cmd);
	printf("transmitting [0x%x][0x%x]\n", cmd[0], cmd[1]);

	/* BYTE 1 and 2: 0x31 and 0xCE */
	tx_num = m_ubl_comm.usart_send((const char*) &cmd, len);
	if (tx_num == len) {
		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* BYTE 3 to 6: start address */
		tx_num = m_ubl_comm.usart_send((const char*) start_addr, 4);
		if (tx_num != 4) {
			printf("usart_send start_addr failed, tx_num = %d\n", tx_num);
			return -1;
		}

		/* BYTE 7: checksum */
		uint8_t checksum = create_xor_checksum(start_addr, 4);
		tx_num = m_ubl_comm.usart_send((const char*) &checksum, sizeof(checksum));
		if (tx_num != sizeof(checksum)) {
			printf("usart_send start_addr checksum, tx_num = %d\n", tx_num);
			return -1;
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		/* BYTE 8 and 9: number of bytes to be read and its checksum */
		uint8_t nbyte_and_data[257] = {0x00};		// contains number of bytes plus the data bytes (n+1 = max 256 bytes)
		nbyte_and_data[0] = bytes_to_write - 1U;
		memcpy(&nbyte_and_data[1], buf, bytes_to_write);
		checksum = create_xor_checksum(nbyte_and_data, bytes_to_write+1);
#if 0
		ESP_LOGI(TAG, "USART Send: ");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &nbyte_and_data, 257, ESP_LOG_WARN);
#endif
		tx_num = m_ubl_comm.usart_send((const char*) &nbyte_and_data[0], 1);
		tx_num = m_ubl_comm.usart_send((const char*) &nbyte_and_data[1], bytes_to_write);
		tx_num = m_ubl_comm.usart_send((const char*) &checksum, sizeof(checksum));
		if (tx_num != sizeof(checksum)) {
			printf("usart_send start_addr checksum, tx_num = %d\n", tx_num);
			return -1;
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}

/********************************************************************************************/
/********************************** ERASE MEMORY COMMAND ************************************/
/********************************************************************************************/
int stm32_uble_cmd_execute_erase_mem(uint8_t num_pages, uint8_t start_page, uint8_t end_page, uint8_t mass_erase) {
	if ( check_lib_status() != 0 )	return -1;

	int ret = 0, tx_num = 0;

	/* Command to be transmitted and its complement */
	uint8_t cmd[2] = { 0x00 };
	cmd[0] = CMD_ERASE_MEM;
	cmd[1] = ~CMD_ERASE_MEM;
	uint8_t len = sizeof(cmd);
	printf("transmitting [0x%x][0x%x]\n", cmd[0], cmd[1]);

	/* BYTE 1 and 2: 0x43 and 0xBC */
	tx_num = m_ubl_comm.usart_send((const char*) &cmd, len);
	if (tx_num == len) {
		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		if (mass_erase) {
			uint8_t data = 0xFF;
			m_ubl_comm.usart_send((const char*) &data, 1);
			data = 0x00;
			m_ubl_comm.usart_send((const char*) &data, 1);
		} else {
			// TODO
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}


/********************************************************************************************/
/******************************* EXTENDED ERASE MEMORY COMMAND ******************************/
/********************************************************************************************/
int stm32_uble_cmd_execute_ext_erase_mem(uint16_t nop, uint8_t *pgnos, uint8_t *spl_erase) {
	if ( check_lib_status() != 0 )	return -1;

	int ret = 0, tx_num = 0;

	/* Command to be transmitted and its complement */
	uint8_t cmd[2] = { 0x00 };
	cmd[0] = CMD_EXT_ERASE_MEM;
	cmd[1] = ~CMD_EXT_ERASE_MEM;
	uint8_t len = sizeof(cmd);
	printf("transmitting [0x%x][0x%x]\n", cmd[0], cmd[1]);

	/* BYTE 1 and 2: 0x43 and 0xBC */
	tx_num = m_ubl_comm.usart_send((const char*) &cmd, len);
	if (tx_num == len) {
		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

		if (spl_erase != 0) {
			uint16_t spl_data = 0x00;
			memcpy(&spl_data, spl_erase, 2);

#if DEBUG_LOG
			printf("spl_data = 0x%x\n", spl_data);
#endif
			switch (spl_data) {
			case 0xFFFF:	 /* mass erase */
			case 0xFFFE:	 /* bank 1 erase */
			case 0xFFFD:	 /* bank 2 erase */
				ret = 0;	 /* ok */
				break;
			default:
				printf("incorrect special erase value!\n");
				ret = -1;	 /*incorrect spl_erase value*/
			}

			if (ret != 0)	return -1;
			m_ubl_comm.usart_send((const char*) &spl_data, 2);
			uint8_t checksum = create_xor_checksum((uint8_t*) &spl_data, 2);
#if DEBUG_LOG
			printf("checksum = 0x%x\n", checksum);
#endif
			m_ubl_comm.usart_send((const char*) &checksum, 1);

		} else {
			/* Bytes 3, 4: number of pages */
			uint8_t data_nop[2] = {0x00};

			data_nop[0] = (uint8_t) (nop >> 8) & 0xFFU;
			data_nop[1] = (uint8_t) nop & 0xFFU;
#if DEBUG_LOG
			printf("bytes 3 to 4 = 0x%x 0x%x\n", data_nop[0], data_nop[1]);
#endif
			m_ubl_comm.usart_send((const char*) data_nop, 2);

			/* Bytes 2x(N+1): page numbers */
			uint8_t *data_pgnos = (uint8_t *)calloc((nop+1), 2);
			size_t len_pgnos = (nop+1) * 2;
#if DEBUG_LOG
			printf("len_pgnos = %d\n", len_pgnos);
			printf("bytes 2x(N+1): ");
#endif
			for (int i=0,j=0; i<(nop+1); i++,j+=2) {
				data_pgnos[j] = (uint8_t) (pgnos[i] >> 8) & 0xFFU;
				data_pgnos[j+1] = (uint8_t) pgnos[i] & 0xFFU;
#if DEBUG_LOG
			printf("[0x%x 0x%x] ", data_pgnos[j], data_pgnos[j+1]);
#endif
			}
			m_ubl_comm.usart_send((const char*) data_pgnos, len_pgnos);

			/* Checksum byte */
			uint8_t *data_ckh = (uint8_t *)calloc((len_pgnos+2), 1);
			size_t len_chk = len_pgnos+2;
			memcpy(data_ckh, data_nop, 2);
			memcpy(data_ckh+2, data_pgnos, len_pgnos);

			uint8_t checksum = create_xor_checksum((uint8_t*) data_ckh, len_chk);
#if DEBUG_LOG
			printf("\n");
			printf("data_ckh: ");
			for (int i=0; i<len_chk; i++) {
				printf("0x%x ", data_ckh[i]);
			}
			printf("\n");
			printf("len_chk = %d\n", len_chk);
			printf("checksum = 0x%x\n", checksum);
#endif
			m_ubl_comm.usart_send((const char*) &checksum, 1);

			free(data_pgnos);
			free(data_ckh);
		}

		if ( wait_for_ack() !=0 ) return -1;		/* BYTE ACK */

	} else {
		printf("usart_send failed, tx_num = %d\n", tx_num);
		return -1;
	}

	return ret;
}

/* EOF */
