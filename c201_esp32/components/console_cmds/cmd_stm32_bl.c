/*
 * Copyright (c) 2021 Acme CPU
 *
 * cmd_stm32_bl.c
 * Created on: 23-Sep-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "sdkconfig.h"

#include "stm32_usart_bl_host.h"
#include "stm32g4xx_flash.h"

#include "stm32_test_fw/stm32g4_test_fw.h"

#define TAG "cmd_stm32_bl"

static void register_start();
static void register_get(void);
static void register_get_vrp(void);
//static void register_get_id(void);
static void register_read_memory(void);
static void register_go(void);
static void register_write_memory(void);
static void register_write_ob(void);
static void register_erase(void);
//static void register_ext_erase(void);
//static void register_write_protect(void);
//static void register_write_unprotect(void);
//static void register_readout_protect(void);
//static void register_readout_unprotect(void);
//static void register_get_checksum(void);
static void register_stm32_program(void);
static void register_stm32_verify(void);

void register_stm32_bl() {
	register_start();
	register_get();
	register_get_vrp();
	register_read_memory();
	register_go();
	register_write_memory();
	register_write_ob();
	register_erase();
	register_stm32_program();
	register_stm32_verify();
}

/* start */
static int start(int argc, char **argv) {
	int ret = 0;

	ret = stm32_ubl_usart_open();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "stm32_ubl_usart_open failed!");
	}

	ret = stm32_ubl_start_check();
	if (ret != 0) {
		ESP_LOGE(TAG, "stm32_ubl_start_check failed!");
	}

	return 0;
}

static void register_start(void) {
	const esp_console_cmd_t cmd = { .command = "stmbl_start", .help = "Start the STM32 bootloader communication",
			.hint = NULL, .func = &start, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/************************************* GET command 0x00 *************************************/
/********************************************************************************************/
static int get_cmd(int argc, char **argv) {
	int ret = 0;
	uint8_t resp[32] = {0x00};
	size_t resp_len = 0;

	ret = stm32_ubl_cmd_execute_get(resp, &resp_len);
	ESP_LOGI(TAG, "len = %d", resp_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, resp, resp_len, ESP_LOG_INFO);

	return ret;
}

static void register_get(void) {
	const esp_console_cmd_t cmd = { .command = "stmbl_get", .help = "Execute the STM32 bootloader GET command", .hint =
			NULL, .func = &get_cmd, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/******************** Get version and read protection status command 0x01 *******************/
/********************************************************************************************/
static int get_vrp(int argc, char **argv) {
	int ret = 0;
	uint8_t bl_ver = 0x00;
	uint8_t ob_1 = 0x00;
	uint8_t ob_2 = 0x00;

	ret = stm32_ubl_cmd_execute_get_version(&bl_ver, &ob_1, &ob_2);
	ESP_LOG_BUFFER_HEXDUMP(TAG, &bl_ver, 1, ESP_LOG_INFO);
	ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_1, 1, ESP_LOG_INFO);
	ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_2, 1, ESP_LOG_INFO);

	return ret;
}

static void register_get_vrp(void) {
	const esp_console_cmd_t cmd = { .command = "stmbl_get_vrp", .help = "Execute the STM32 bootloader GET VRP command", .hint =
			NULL, .func = &get_vrp, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/* GET ID command 0x02 */

/********************************************************************************************/
/************************************* Go command 0x21 **************************************/
/********************************************************************************************/
static int go_cmd(int argc, char **argv) {
	int ret = 0;

	ret = stm32_ubl_cmd_execute_go();
	ESP_LOGI(TAG, "GO ret = %d", ret);

	return 0;
}

static void register_go(void) {
	const esp_console_cmd_t cmd = { .command = "stmbl_go", .help = "Execute the STM32 bootloader GO command", .hint =
			NULL, .func = &go_cmd, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


/********************************************************************************************/
/********************************* Read memory command 0x11 *********************************/
/********************************************************************************************/
static struct {
	struct arg_str *addr_bank1;
	struct arg_int *size;
    struct arg_end *end;
} read_args;

static int read_mem_cmd(int argc, char **argv) {
	int ret = 0;

    int nerrors = arg_parse(argc, argv, (void **) &read_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, read_args.end, argv[0]);
        return 1;
    }
    const char *addr = read_args.addr_bank1->sval[0];

    ESP_LOGD(TAG, "addr[%c%c %c%c %c%c %c%c]", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

    uint8_t read_addr[4] = {0x00};
    char taddr[3] = {0x00};

	for (int i = 0; i < 4; i++) {
		memcpy(taddr, addr+(i*2), 2);
		read_addr[i] = strtoul(taddr, NULL, 16);
		memset(taddr, 0, 3);

		ESP_LOGD(TAG, "read_addr[%d] = 0x%x", i, read_addr[i]);
	}
	size_t data_size = read_args.size->ival[0];
	if (data_size > 256) {
		ESP_LOGE(TAG, "Incorrect size");
		return -1;
	}

//	uint8_t flash_optr_addr[4] = {0x1F, 0xFF, 0x78, 0x00};
	uint8_t read_data[256] = {0x00};
	ret = stm32_uble_cmd_execute_read_mem(read_addr, read_data, data_size);
	ESP_LOGI(TAG, "READ MEM ret = %d", ret);

	if (!ret)
		ESP_LOG_BUFFER_HEXDUMP(TAG, &read_data, data_size, ESP_LOG_WARN);

	return ret;
}
static void register_read_memory() {
	read_args.addr_bank1 = arg_str0(NULL, NULL, "<a>", "Address");
	read_args.size = arg_int0(NULL, NULL, "<s>", "Data size 0 to 255");
	read_args.end = arg_end(2);

	const esp_console_cmd_t cmd = {
			.command = "stmbl_read",
			.help = "Execute the STM32 bootloader READ MEMORY command",
			.hint =	NULL,
			.func = &read_mem_cmd,
			.argtable = &read_args};

	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/********************************* Write memory command 0x31 ********************************/
/********************************************************************************************/
static struct {
	struct arg_str *addr;
	struct arg_int *size;
    struct arg_end *end;
} write_args;

static int write_mem_cmd(int argc, char **argv) {
	int ret = 0;
    int nerrors = arg_parse(argc, argv, (void **) &write_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, write_args.end, argv[0]);
        return 1;
    }
    const char *addr = write_args.addr->sval[0];

    ESP_LOGD(TAG, "addr[%c%c %c%c %c%c %c%c]", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

    uint8_t write_addr[4] = {0x00};
    char taddr[3] = {0x00};

	for (int i = 0; i < 4; i++) {
		memcpy(taddr, addr+(i*2), 2);
		write_addr[i] = strtoul(taddr, NULL, 16);
		memset(taddr, 0, 3);

		ESP_LOGD(TAG, "read_addr[%d] = 0x%x", i, write_addr[i]);
	}
	size_t data_size = write_args.size->ival[0];
	if (data_size > 256) {
		ESP_LOGE(TAG, "Incorrect size");
		return -1;
	}

	uint8_t data[256] = {0x00};
	for (int i=0; i<data_size; i++) {
		data[i] = 0xab;
	}
	ESP_LOGI(TAG, "Writing :");
	ESP_LOG_BUFFER_HEXDUMP(TAG, &data, data_size, ESP_LOG_WARN);
	ret = stm32_uble_cmd_execute_write_mem(write_addr, data, data_size);
	ESP_LOGI(TAG, "WRITE MEM ret = %d", ret);

	return ret;
}

static void register_write_memory(void) {
	write_args.addr = arg_str0(NULL, NULL, "<a>", "Address");
	write_args.size = arg_int0(NULL, NULL, "<s>", "Data size 0 to 255");
	write_args.end = arg_end(2);

	const esp_console_cmd_t cmd = {
			.command = "stmbl_write",
			.help = "Execute the STM32 bootloader WRITE MEMORY command",
			.hint = NULL,
			.func = &write_mem_cmd,
			.argtable = &write_args};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/********************************* Write OB command 0x31 ************************************/
/********************************************************************************************/
static struct {
	struct arg_str *addr_bank1;
	struct arg_int *size_bank1;
	struct arg_str *addr_bank2;
	struct arg_int *size_bank2;
    struct arg_end *end;
} write_ob_args;

// stmbl_write_ob 1fff7800 48 1ffff800 48

static int write_ob_cmd(int argc, char **argv) {
	int ret = 0, status = 0;
	;

	int nerrors = arg_parse(argc, argv, (void**) &write_ob_args);
	if (nerrors != 0) {
		arg_print_errors(stderr, write_ob_args.end, argv[0]);
		return 1;
	}
	const char *addr_bank1 = write_ob_args.addr_bank1->sval[0];
	const char *addr_bank2 = write_ob_args.addr_bank2->sval[0];

	uint8_t ob_addr_bank1[4] = { 0x00 };
	char taddr[3] = { 0x00 };

	for (int i = 0; i < 4; i++) {
		memcpy(taddr, addr_bank1 + (i * 2), 2);
		ob_addr_bank1[i] = strtoul(taddr, NULL, 16);
		memset(taddr, 0, 3);

		ESP_LOGD(TAG, "read_addr[%d] = 0x%x", i, ob_addr_bank1[i]);
	}

	size_t ob_size_bank1 = write_ob_args.size_bank1->ival[0];

	/* Read the option bytes */
#define OB_SIZE_MAX	48
	uint8_t ob_bank1[OB_SIZE_MAX] = { 0x00 };
	uint8_t ob_bank2[OB_SIZE_MAX] = { 0x00 };

	FLASH_OB_REG ob_b1, ob_b2;

	/* bank 1*/
	ret = stm32_uble_cmd_execute_read_mem(ob_addr_bank1, ob_bank1, ob_size_bank1);
	ESP_LOGI(TAG, "READ MEM ret = %d", ret);
	if (!ret) {
		status++;
		memcpy(&ob_b1, ob_bank1, ob_size_bank1);
#if 1
		ESP_LOGI(TAG, "==============================================");
		ESP_LOGI(TAG, "OB Bank 1");
//		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_bank1, ob_size_bank1, ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_OPTR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_optr, sizeof(ob_b1.flash_optr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_optr_c, sizeof(ob_b1.flash_optr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_PCROP1SR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_pcrop_sr, sizeof(ob_b1.flash_pcrop_sr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_pcrop_sr_c, sizeof(ob_b1.flash_pcrop_sr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_PCROP1ER");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_pcrop_er, sizeof(ob_b1.flash_pcrop_er), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_pcrop_er_c, sizeof(ob_b1.flash_pcrop_er_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_WRP1AR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_wrpar, sizeof(ob_b1.flash_wrpar), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_wrpar_c, sizeof(ob_b1.flash_wrpar_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_WRP1BR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_wrpbr, sizeof(ob_b1.flash_wrpbr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_wrpbr_c, sizeof(ob_b1.flash_wrpbr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_SEC");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_sec, sizeof(ob_b1.flash_sec), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_sec_c, sizeof(ob_b1.flash_sec_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "==============================================");
#endif
	}

	/* bank 2 */
	uint8_t ob_addr_bank2[4] = { 0x00 };
//	char taddr[3] = { 0x00 };

	for (int i = 0; i < 4; i++) {
		memcpy(taddr, addr_bank2 + (i * 2), 2);
		ob_addr_bank2[i] = strtoul(taddr, NULL, 16);
		memset(taddr, 0, 3);

		ESP_LOGD(TAG, "read_addr[%d] = 0x%x", i, ob_addr_bank2[i]);
	}
	size_t ob_size_bank2 = write_ob_args.size_bank2->ival[0];

	ret = stm32_uble_cmd_execute_read_mem(ob_addr_bank2, ob_bank2, ob_size_bank2);
	ESP_LOGI(TAG, "READ MEM ret = %d", ret);
	if (!ret) {
		status++;
		memcpy(&ob_b2, ob_bank2, ob_size_bank2);
#if 1
		ESP_LOGI(TAG, "==============================================");
		ESP_LOGI(TAG, "OB Bank 2");
//			ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_bank2, ob_size_bank2, ESP_LOG_WARN);
		ESP_LOGI(TAG, "UNUSED");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_optr, sizeof(ob_b2.flash_optr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_optr_c, sizeof(ob_b2.flash_optr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_PCROP2SR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_pcrop_sr, sizeof(ob_b2.flash_pcrop_sr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_pcrop_sr_c, sizeof(ob_b2.flash_pcrop_sr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_PCROP2ER");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_pcrop_er, sizeof(ob_b2.flash_pcrop_er), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_pcrop_er_c, sizeof(ob_b2.flash_pcrop_er_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_WRP2AR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_wrpar, sizeof(ob_b2.flash_wrpar), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_wrpar_c, sizeof(ob_b2.flash_wrpar_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_WRP2BR");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_wrpbr, sizeof(ob_b2.flash_wrpbr), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_wrpbr_c, sizeof(ob_b2.flash_wrpbr_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "FLASH_SEC");
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_sec, sizeof(ob_b2.flash_sec), ESP_LOG_WARN);
		ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b2.flash_sec_c, sizeof(ob_b2.flash_sec_c), ESP_LOG_WARN);
		ESP_LOGI(TAG, "==============================================");
#endif
	}

	/* Change the option byte */
	if (status != 2) {
		ESP_LOGE(TAG, "read failed, not changing option byte");
		return -1;
	}

	ob_b1.flash_optr |= (OB_nBOOT0_SET | OB_BOOT0_FROM_PIN);
	ob_b1.flash_optr_c = ~ob_b1.flash_optr;
	ESP_LOGI(TAG, "FLASH_OPTR");
	ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_optr, sizeof(ob_b1.flash_optr), ESP_LOG_WARN);
	ESP_LOG_BUFFER_HEXDUMP(TAG, &ob_b1.flash_optr_c, sizeof(ob_b1.flash_optr_c), ESP_LOG_WARN);
	status++;

	/* Write the option bytes to flash */
	if (status != 3) {
		ESP_LOGE(TAG, "read failed, not changing option byte");
		return -1;
	}

	uint8_t ob_wr_bank1[OB_SIZE_MAX] = { 0x00 };
	uint8_t ob_wr_bank2[OB_SIZE_MAX] = { 0x00 };

	memcpy(ob_wr_bank1, &ob_b1, ob_size_bank1);
	memcpy(ob_wr_bank2, &ob_b2, ob_size_bank2);

	ESP_LOGI(TAG, "Printing write buffers:");
	ESP_LOG_BUFFER_HEXDUMP(TAG, ob_wr_bank1, ob_size_bank1, ESP_LOG_WARN);
	ESP_LOG_BUFFER_HEXDUMP(TAG, ob_wr_bank2, ob_size_bank2, ESP_LOG_WARN);

	ESP_LOGI(TAG, "Writing to Option bytes Bank 1:");
	ret = stm32_uble_cmd_execute_write_mem(ob_addr_bank1, ob_wr_bank1, ob_size_bank1);
	ESP_LOGI(TAG, "WRITE MEM ret = %d", ret);

//	ESP_LOGI(TAG, "Writing to Option bytes Bank 2:");
//	ret = stm32_uble_cmd_execute_write_mem(ob_addr_bank2, ob_wr_bank2, ob_size_bank2);
//	ESP_LOGI(TAG, "WRITE MEM ret = %d", ret);

	return ret;
}
static void register_write_ob(void) {
	write_ob_args.addr_bank1 = arg_str0(NULL, NULL, "<ab1>", "Address Bank 1");
	write_ob_args.size_bank1 = arg_int0(NULL, NULL, "<sb1>", "OB size bank 1");
	write_ob_args.addr_bank2 = arg_str1(NULL, NULL, "<ab2>", "Address Bank 2");
	write_ob_args.size_bank2 = arg_int1(NULL, NULL, "<sb2>", "OB size bank 2");
	write_ob_args.end = arg_end(2);

	const esp_console_cmd_t cmd = {
			.command = "stmbl_write_ob",
			.help = "Command used to write to Option Bytes area of STM32",
			.hint =	NULL,
			.func = &write_ob_cmd,
			.argtable = &write_ob_args};

	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/********************************* Erase memory command 0x43 ********************************/
/********************************************************************************************/
static int erase_mem_cmd(int argc, char **argv) {
	int ret = 0;

	uint16_t num_pages = 0;				// to erase n pages n-1 has to be sent
	uint8_t page_numbers[128] = {0};	// page numbers of the n pages

	uint8_t spl_erase[2] = {0xFF, 0xFF};
	ret = stm32_uble_cmd_execute_ext_erase_mem(num_pages, page_numbers, /*spl_erase*/NULL);
	ESP_LOGI(TAG, "ERASE MEM ret = %d", ret);

	return ret;
}

static void register_erase(void) {
	const esp_console_cmd_t cmd = { .command = "stmbl_erase", .help = "Execute the STM32 bootloader ERASE MEMORY command", .hint =
			NULL, .func = &erase_mem_cmd, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}


/********************************************************************************************/
/**************************** PROGRAM / FLASH STM32 MCU FIRMWARE ****************************/
/********************************************************************************************/
static int stm32_program(int argc, char **argv) {
	int ret = 0;

	const size_t fw_len = sizeof(_aczephyr);
	const size_t pg_len = 4096;
	const size_t max_wr_len = 256;
	size_t bytes_progd=0, wr_offset=0;
	int rem_fw_len=0, rem_pg_len=0;
	uint8_t pg_no[16] = {0x00};

	ESP_LOGI(TAG, "fw_len = %d, pg_len = %d", fw_len, pg_len);

	uint8_t write_addr[4];
	uint32_t address = 0x08000000;

	rem_fw_len=fw_len;
	do {
		rem_pg_len=pg_len;

		/* Erase next page - we erase one page and program it and then move to the next page */
		ESP_LOGI(TAG, "====================================================");
		ESP_LOGI(TAG, " ERASING: page %d", pg_no[0]);
		ESP_LOGI(TAG, "====================================================");

		ret = stm32_uble_cmd_execute_ext_erase_mem(0, pg_no, NULL);
		if (ret) return -1;

		pg_no[0]++;

		/* Loop to program a page */
		do {
			write_addr[0] = (uint8_t) ((address >> 24) & 0xFFU);
			write_addr[1] = (uint8_t) ((address >> 16) & 0xFFU);
			write_addr[2] = (uint8_t) ((address >> 8) & 0xFFU);
			write_addr[3] = (uint8_t) (address & 0xFFU);

			ESP_LOGI(TAG, "WRITING TO: [0x%x] :: [0x%x] [0x%x] [0x%x] [0x%x]", address, write_addr[0], write_addr[1],
					write_addr[2], write_addr[3]);

			if ((fw_len - bytes_progd) >= max_wr_len) {
				ESP_LOGI(TAG, "1. Programming offset %d with %d bytes", wr_offset, max_wr_len);

				ret = stm32_uble_cmd_execute_write_mem(write_addr, (uint8_t*)(_aczephyr + wr_offset), max_wr_len);
				if (ret) return -1;

				wr_offset += max_wr_len;
				bytes_progd += max_wr_len;
				address += max_wr_len;
				rem_pg_len -= max_wr_len;

				ESP_LOGI(TAG, "bytes_progd = %d, rem_pg_len = %d", bytes_progd, rem_pg_len);
			} else if ((fw_len - bytes_progd) == 0) {
				ESP_LOGI(TAG, "3. No more data");
				break;
			} else {
				size_t wr_len = (fw_len - bytes_progd);
				ESP_LOGI(TAG, "2. Programming offset %d with %d bytes", wr_offset, wr_len);

				ret = stm32_uble_cmd_execute_write_mem(write_addr, (uint8_t*)(_aczephyr + wr_offset), wr_len);
				if (ret) return -1;

				wr_offset += wr_len;
				bytes_progd += wr_len;
				address += wr_len;
				rem_pg_len -= wr_len;

				ESP_LOGI(TAG, "bytes_progd = %d, rem_pg_len = %d", bytes_progd, rem_pg_len);
			}
			ESP_LOGI(TAG, "");
		} while (rem_pg_len != 0);

		rem_fw_len -= pg_len;
		ESP_LOGI(TAG, "REM: rem_fw_len = %d", rem_fw_len);
		ESP_LOGI(TAG, "");
	} while (rem_fw_len > 0);

	return ret;
}

static void register_stm32_program(void) {
	const esp_console_cmd_t cmd = {
			.command = "stmbl_prog",
			.help = "Program/Flash a STM32 controller",
			.hint =	NULL,
			.func = &stm32_program, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

/********************************************************************************************/
/**************************** VERIFY STM32 FLASH AFTER PROGRAMMING **************************/
/********************************************************************************************/
static int stm32_verify(int argc, char **argv) {
	int ret = 0;

	uint8_t arr[10] = {0x00};
	printf("sizeof = %d\n", sizeof (arr));

	const size_t fw_len = sizeof(_aczephyr);
	const size_t max_rd_len = 256;
	size_t bytes_read=0, rd_offset=0;
	int rem_fw_len=0;

	uint8_t read_addr[4];
	uint32_t address = 0x08000000;
	uint8_t read_data[256] = {0x00};

	ESP_LOGI(TAG, "fw_len = %d", fw_len);

	rem_fw_len=fw_len;

	do {
		/* we read one chunk and compare it with the file and move to the next chunk */

		read_addr[0] = (uint8_t) ((address >> 24) & 0xFFU);
		read_addr[1] = (uint8_t) ((address >> 16) & 0xFFU);
		read_addr[2] = (uint8_t) ((address >> 8) & 0xFFU);
		read_addr[3] = (uint8_t) (address & 0xFFU);

		ESP_LOGI(TAG, "====================================================");
		ESP_LOGI(TAG, "READING FROM: [0x%x] :: [0x%x] [0x%x] [0x%x] [0x%x]", address, read_addr[0], read_addr[1], read_addr[2], read_addr[3]);
		ESP_LOGI(TAG, "====================================================");

		if ((fw_len - bytes_read) >= max_rd_len) {
			ESP_LOGI(TAG, "1. Reading offset %d with %d bytes", rd_offset, max_rd_len);

			/* Read from flash */
			ret = stm32_uble_cmd_execute_read_mem(read_addr, read_data, max_rd_len);
			if (ret) return -1;

			/* compare with file */
			ret = memcmp(read_data, (_aczephyr + rd_offset), max_rd_len);
			if (!ret)	ESP_LOGI(TAG, "Compare OK");
			else {
				ESP_LOGE(TAG, "Compare failed");
				break;
			}

			rd_offset += max_rd_len;
			bytes_read += max_rd_len;
			address += max_rd_len;
			rem_fw_len -= max_rd_len;

			ESP_LOGI(TAG, "bytes_read = %d, rem_fw_len = %d", bytes_read, rem_fw_len);
		} else if ((fw_len - bytes_read) == 0) {
			ESP_LOGI(TAG, "3. No more data");
			break;
		} else {
			size_t rd_len = (fw_len - bytes_read);
			ESP_LOGI(TAG, "2. Reading offset %d with %d bytes", rd_offset, rd_len);

			/* Read from flash */
			ret = stm32_uble_cmd_execute_read_mem(read_addr, read_data, rd_len);
			if (ret) return -1;

			/* compare with file */
			ret = memcmp(read_data, (_aczephyr + rd_offset), rd_len);
			if (!ret)	ESP_LOGI(TAG, "Compare OK");
			else {
				ESP_LOGE(TAG, "Compare failed");
				break;
			}

			rd_offset += rd_len;
			bytes_read += rd_len;
			address += rd_len;
			rem_fw_len -= rd_len;

			ESP_LOGI(TAG, "bytes_read = %d, rem_fw_len = %d", bytes_read, rem_fw_len);
		}
		ESP_LOGI(TAG, "");
	} while (rem_fw_len > 0);

	if (!ret)	ESP_LOGI(TAG, "Verify OK");
	else {
		ESP_LOGE(TAG, "Verify failed");
		ret = -1;
	}

	return ret;
}

static void register_stm32_verify(void) {
	const esp_console_cmd_t cmd = {
			.command = "stmbl_verify",
			.help = "Verify STM32 flash after programming",
			.hint =	NULL,
			.func = &stm32_verify, };
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
/* EOF */
