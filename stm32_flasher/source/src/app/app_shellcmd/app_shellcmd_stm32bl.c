/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 19-Oct-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#include <zephyr/fs/fs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#include "lib_stm32bl_host/lib_stm32bl_host.h"
#include "app_storage/app_storage.h"

#define HIGH	1
#define LOW		0

static bool m_device_in_bl_mode = false;
static uint32_t m_rw_chunk = 256;

static int bootloader_mode_put()
{
	int ret = 0;
#if 1//(CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
	const struct gpio_dt_spec rst = GPIO_DT_SPEC_GET(DT_NODELABEL(bldc_rst), gpios);
	const struct gpio_dt_spec boot = GPIO_DT_SPEC_GET(DT_NODELABEL(bldc_boot), gpios);

	if (!gpio_is_ready_dt(&rst)) {
		LOG_ERR("gpio %d not ready", rst.pin);
		return -1;
	}
	if (!gpio_is_ready_dt(&boot)) {
		LOG_ERR("gpio %d not ready", boot.pin);
		return -1;
	}

	ret = gpio_pin_configure_dt(&rst, (GPIO_OUTPUT | rst.dt_flags));
	if (ret < 0) {
		LOG_ERR("gpio %d configure failed", rst.pin);
		return -1;
	}
	ret = gpio_pin_configure_dt(&boot, (GPIO_OUTPUT | boot.dt_flags));
	if (ret < 0) {
		LOG_ERR("gpio %d configure failed", boot.pin);
		return -1;
	}

	ret = gpio_pin_set_dt(&boot, HIGH);
	ret = gpio_pin_set_dt(&rst, LOW);
	k_sleep(K_MSEC(1000));
	ret = gpio_pin_set_dt(&rst, HIGH);
#endif
	return ret;
}

static int bootloader_mode_exit()
{
	int ret = 0;
#if 1//(CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
	const struct gpio_dt_spec boot = GPIO_DT_SPEC_GET(DT_NODELABEL(bldc_boot), gpios);

	if (!gpio_is_ready_dt(&boot)) {
		LOG_ERR("gpio %d not ready", boot.pin);
		return -1;
	}

	ret = gpio_pin_configure_dt(&boot, (GPIO_OUTPUT | boot.dt_flags));
	if (ret < 0) {
		LOG_ERR("gpio %d configure failed", boot.pin);
		return -1;
	}
	ret = gpio_pin_set_dt(&boot, LOW);
#endif
	return ret;
}

static int stm32bl_start(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret = 0;

	/* put device into bootloader mode */
	ret = bootloader_mode_put();
	if (ret < 0) {
		LOG_ERR("bootloader_mode_put failed, %d", ret);
		return ret;
	}

	/* send start byte and version response */
	ret = lib_stm32bl_host_start_check();
	if (!ret) {
		m_device_in_bl_mode = true;
		shell_print(shell, MSG_PASS);
	} else {
		shell_print(shell, MSG_FAIL", %d", ret);
	}

	bootloader_mode_exit();
	return ret;
}

static int stm32bl_rw_chunk(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint32_t chunk = strtol(argv[1], NULL, 10);
	if ((chunk > 0) && (chunk < 512)) {
		m_rw_chunk = chunk;
		shell_print(shell, MSG_PASS);
	} else {
		shell_print(shell, MSG_FAIL);
	}
	return 0;
}

static int stm32bl_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint8_t buf[100];
	size_t len;

	int ret = lib_stm32bl_host_cmd_execute_get(buf, &len);
	if (ret == 0) {
		shell_print(shell, MSG_PASS);
//		shell_hexdump(shell, buf, len);

		int num_bytes = buf[1];
		shell_print(shell, "bootloader version: 0x%x", buf[2]);
		shell_print(shell, "supported commands:");
		shell_hexdump(shell, &buf[3], num_bytes);
//		LOG_HEXDUMP_INF(buf, len, "GET");
	} else {
		shell_print(shell, MSG_FAIL", %d", ret);
	}

	return 0;
}

static int stm32bl_get_vrp(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint8_t bl_ver, ob1, ob2;

	int ret = lib_stm32bl_host_cmd_execute_get_version(&bl_ver, &ob1, &ob2);
	if (ret == 0) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "bootloader version: 0x%x", bl_ver);
		shell_print(shell, "option byte 1: 0x%x", ob1);
		shell_print(shell, "option byte 2: 0x%x", ob2);
	} else {
		shell_print(shell, MSG_FAIL", %d", ret);
	}

	return 0;
}

static int stm32bl_get_id(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	int ret = 0;//lib_stm32bl_host_cmd_execute_get_version(&bl_ver, &ob1, &ob2);
	if (ret == 0) {
		shell_print(shell, "not implemented !!!");
	} else {
		shell_print(shell, MSG_FAIL", %d", ret);
	}

	return 0;
}

static uint32_t addr_byte_swap(uint32_t x)
{
	uint32_t tmp1, tmp2, tmp3, tmp4;
	tmp4 = (x & 0x000000FF ) << 24;
	tmp3 = (x & 0x0000FF00 ) << 8;
	tmp2 = (x & 0x00FF0000 ) >> 8;
	tmp1 = (x & 0xFF000000 ) >> 24;

	return (tmp4 | tmp3 | tmp2 | tmp1);
}

static int stm32bl_read_mem(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint32_t addr = strtol(argv[1], NULL, 16);
	uint32_t len = strtol(argv[2], NULL, 10);

//	uint8_t start_addr[4] = {0x08, 0x00, 0x00, 0x00};
	uint32_t start_addr = addr;//0x08000000;
	size_t size = len;//(1*1024);
	size_t chunk_sz = m_rw_chunk;

	if (len < chunk_sz) {
		shell_print(shell, MSG_FAIL", len (%d) is less than chunk size (%d), increase len or decrease chunk size", len, chunk_sz);
		return -1;
	}

	for (int i = 0; i < size; i += chunk_sz) {
		uint8_t *read_buf = (uint8_t*) calloc(1, chunk_sz);
		if (read_buf == NULL) {
			shell_print(shell, MSG_FAIL", no memory!");
			return -1;
		}
		uint32_t addr_swapped = addr_byte_swap(start_addr);
		int ret = lib_stm32bl_host_cmd_execute_read_mem((uint8_t*)&addr_swapped, read_buf, chunk_sz);
		if (ret == 0) {
			shell_print(shell, "%d, %d, [0x%x] to [0x%x]", i, (i+chunk_sz), start_addr, start_addr+chunk_sz);
//			shell_hexdump(shell, read_buf, chunk_sz);
		} else {
			shell_print(shell, MSG_FAIL", %d", ret);
			free(read_buf);
			break;
		}
		start_addr = start_addr + chunk_sz;
		free(read_buf);
	}

	return 0;
}

static int stm32bl_read_mem_save(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}
	int ret = 0;
	uint32_t addr = strtol(argv[1], NULL, 16);	// start addr
	uint32_t len = strtol(argv[2], NULL, 10);	// bytes to read

	char fname[64] = {0x00};
	char fname_with_path[256] = {0x00};
	strcpy(fname, argv[3]);						// filename to save
	strcpy(fname_with_path, app_storage_mount_point_get());
	strcat(fname_with_path, "/");
	strcat(fname_with_path, fname);

	/* delete file is it exists */
	fs_unlink(fname_with_path);

	/* open or create the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, fname_with_path, (FS_O_CREATE | FS_O_READ | FS_O_WRITE /*| FS_O_APPEND*/));
	if (ret != 0) {
		LOG_ERR("file %s open/create failed!", fname_with_path);
		shell_print(shell, MSG_FAIL": file %s open/create failed!", fname_with_path);
		return ret;
	}

	uint32_t start_addr = addr;
	size_t size = len;
	size_t chunk_sz = m_rw_chunk;
	int wr_total = 0;

	if (len < chunk_sz) {
		shell_print(shell, MSG_FAIL", len (%d) is less than chunk size (%d), increase len or decrease chunk size", len, chunk_sz);
		return -1;
	}

	for (int i = 0; i < size; i += chunk_sz) {
		uint8_t *read_buf = (uint8_t*) calloc(1, chunk_sz);
		if (read_buf == NULL) {
			shell_print(shell, MSG_FAIL", no memory!");
			goto err;
		}
		uint32_t addr_swapped = addr_byte_swap(start_addr);
		ret = lib_stm32bl_host_cmd_execute_read_mem((uint8_t*)&addr_swapped, read_buf, chunk_sz);
		if (ret == 0) {
//			LOG_INF("[0x%x] to [0x%x]", start_addr, start_addr+chunk_sz);
//			LOG_HEXDUMP_INF(read_buf, chunk_sz, "");
//			shell_hexdump(shell, read_buf, chunk_sz);

			/* write to file */
			int wrbytes = fs_write(&zfp, read_buf, chunk_sz);
			if (wrbytes != chunk_sz) {
				LOG_ERR("fs_write failed");
				shell_print(shell, MSG_FAIL": fs_write failed");
			} else {
				wr_total += wrbytes;
				LOG_INF("wrote %d bytes", wr_total);
			}
		} else {
			shell_print(shell, MSG_FAIL", %d", ret);
			free(read_buf);
			goto err;
		}
		start_addr = start_addr + chunk_sz;
		free(read_buf);
	}

err:
	/* close the file */
	fs_close(&zfp);

	return 0;
}

static int stm32bl_write_mem(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}
	int ret = 0;
	uint32_t addr = strtol(argv[1], NULL, 16);	// start addr

	char fname[256] = {0x00};
	char fname_with_path[256] = {0x00};
	strcpy(fname, argv[2]);						// filename to save
	strcpy(fname_with_path, app_storage_mount_point_get());
	strcat(fname_with_path, "/");
	strcat(fname_with_path, fname);

	/* open or create the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, fname_with_path, (FS_O_CREATE | FS_O_READ /*| FS_O_WRITE | FS_O_APPEND*/));
	if (ret != 0) {
		LOG_ERR("file %s open/create failed!", fname_with_path);
		shell_print(shell, MSG_FAIL": file %s open/create failed!", fname_with_path);
		return ret;
	}

#define WR_SIZE_MAX	256
	uint32_t start_addr = addr;
	uint8_t write_buf[WR_SIZE_MAX] = {0x00};
	int wr_total = 0;
	char rd;
	int rd_bytes, i;

	while (1) {
		for (i = 0; i < WR_SIZE_MAX; i++) {
			rd_bytes = fs_read(&zfp, &rd, 1);
			if (rd_bytes < 0) {
				LOG_ERR("fs_read failed");
				shell_print(shell, MSG_FAIL": fs_read failed");
				ret = -1;
				goto err;
			} else if (rd_bytes == 0) {	// end of file
				LOG_INF("eof");
				break;
			} else {
				write_buf[i] = rd;
			}
		}
		if ((i == 0) && rd_bytes == 0)	{
			LOG_INF("write complete");
			break;
		}

		uint32_t addr_swapped = addr_byte_swap(start_addr);
		ret = lib_stm32bl_host_cmd_execute_write_mem((uint8_t*)&addr_swapped, write_buf, i);
		if (ret == 0) {
			start_addr = start_addr + i;
			memset(write_buf, 0x00, sizeof(write_buf));
			wr_total += i;
			LOG_INF("wrote %d bytes", wr_total);
//			shell_print(shell, "wrote %d bytes", wr_total);
		} else {
			shell_print(shell, MSG_FAIL", %d", ret);
			goto err;
		}
	}

err:
	/* close the file */
	fs_close(&zfp);

	return ret;
}

static int stm32bl_erase_ext(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		shell_print(shell, "device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint32_t erase_spl = strtol(argv[1], NULL, 10);
	if (erase_spl != 1) {
		shell_print(shell, "not implemented, only mass erase is supported. Usage: erase_ext 1");
		return 0;
	}

	uint8_t spl_erase[2] = {0xFF, 0xFF};
	int ret = lib_stm32bl_host_cmd_execute_ext_erase_mem(0, NULL, spl_erase);
	if (!ret) {
		shell_print(shell, MSG_PASS);
	} else {
		shell_print(shell, MSG_FAIL", %d", ret);
	}

	return ret;
}

static int verify_program(char *bin_fname_with_path, uint32_t addr)
{
	int ret = 0;

#define BUF_SIZE_MAX	256
	uint8_t fread_buf[BUF_SIZE_MAX] = {0x00};
	uint8_t binread_buf[BUF_SIZE_MAX] = {0x00};
	char rd;
	int rd_bytes, i;
	uint32_t start_addr = addr;

	/* open the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, bin_fname_with_path, (FS_O_CREATE | FS_O_READ /*| FS_O_WRITE | FS_O_APPEND*/));
	if (ret != 0) {
		LOG_ERR("file %s open/create failed!", bin_fname_with_path);
		return ret;
	}

	while (1) {
		for (i = 0; i < BUF_SIZE_MAX; i++) {
			rd_bytes = fs_read(&zfp, &rd, 1);
			if (rd_bytes < 0) {
				LOG_ERR("fs_read failed");
				ret = -1;
				goto err;
			} else if (rd_bytes == 0) {	// end of file
				LOG_INF("eof");
				break;
			} else {
				fread_buf[i] = rd;
			}
		}
		if ((i == 0) && rd_bytes == 0)	break;

		uint32_t chunk_sz = i;
		uint32_t addr_swapped = addr_byte_swap(start_addr);
		ret = lib_stm32bl_host_cmd_execute_read_mem((uint8_t*)&addr_swapped, binread_buf, chunk_sz);
		if (ret == 0) {
			if(memcmp(fread_buf, binread_buf, chunk_sz) != 0) {
				LOG_ERR("verify failed");
				ret = -EFAULT;
				goto err;
			}
			start_addr = start_addr + chunk_sz;
		} else {
			LOG_ERR("failed to read from target");
			goto err;
		}
	}
err:
	if (ret == 0) {
		LOG_INF("verify successful");
	}
	fs_close(&zfp);
	return ret;
}

static int stm32bl_flashbin(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}
#define ARGC_MAX		5
#define ARGV_LEN_MAX	20
	int ret = 0;
	char *targv[ARGC_MAX + 1] = {0}; /* +1 reserved for NULL */

	/* start */
	ret = stm32bl_start(shell, 0, NULL);
	if (ret) goto err;

	/* get_id */

	/* mass erase */
	shell_print(shell, "#### Erasing ...");
	targv[0] = "erase_ext ";
	targv[1] = "1";
	ret = stm32bl_erase_ext(shell, 2, &targv[0]);
	if (ret) goto err;
	else	shell_print(shell, "Mass erase successful");

	/* program */
	shell_print(shell, "#### Writing ...");
	targv[0] = "write_mem ";
	targv[1] = "08000000 ";
	targv[2] = argv[1];
	ret = stm32bl_write_mem(shell, 3, &targv[0]);
	if (ret) goto err;
	else	shell_print(shell, "Writing successful");

	k_sleep(K_MSEC(100));

	/* verify */
	shell_print(shell, "#### Verifying ...");
	char fname[256] = {0x00};
	char fname_with_path[256] = {0x00};
	strcpy(fname, argv[1]);						// filename to save
	strcpy(fname_with_path, app_storage_mount_point_get());
	strcat(fname_with_path, "/");
	strcat(fname_with_path, fname);
	ret = verify_program(fname_with_path, 0x08000000);
	if (ret) goto err;
	else	shell_print(shell, "Verify successful");

	/* go */

err:
	if (ret !=0)
		shell_print(shell, MSG_FAIL": Flashing failed, %d", ret);
	else
		shell_print(shell, "Flashing successful");
	return ret;
}

/* stm32bl */
SHELL_STATIC_SUBCMD_SET_CREATE(stm32bl_subcmds,
		SHELL_CMD(start, NULL, "Puts the subject device into bootloader mode and verifies the same", stm32bl_start),
		SHELL_CMD_ARG(rw_chunk, NULL, "Set the read / write chunk size", stm32bl_rw_chunk, 2, 0),
		SHELL_CMD(get, NULL, "Execute STM32 BL GET cmd", stm32bl_get),
		SHELL_CMD(get_vrp, NULL, "Execute STM32 BL Get Version and Read Protection Status cmd", stm32bl_get_vrp),
		SHELL_CMD(get_id, NULL, "Execute STM32 BL Get ID cmd", stm32bl_get_id),
		SHELL_CMD_ARG(read_mem, NULL, "Execute STM32 BL Read Memory cmd. Params start_addr, size", stm32bl_read_mem, 3, 0),
		SHELL_CMD_ARG(read_mem_save, NULL, "Execute STM32 BL Read Memory cmd and save the contents to a file. Usage: read_mem_save 08000000 4096 fw.bin", stm32bl_read_mem_save, 4, 0),
//		SHELL_CMD(go, NULL, "Execute STM32 BL Go cmd", stm32bl_go),
//		SHELL_CMD(write_mem, NULL, "Execute STM32 BL Write Memory cmd", stm32bl_write_mem),
		SHELL_CMD_ARG(write_mem, NULL, "Execute STM32 BL Write Memory cmd read from file and write to target. Usage: write_mem 08000000 fw.bin", stm32bl_write_mem, 3, 0),
//		SHELL_CMD(erase, NULL, "Execute STM32 BL Erase cmd", stm32bl_erase),
		SHELL_CMD_ARG(erase_ext, NULL, "Execute STM32 BL Extended Erase cmd. Usage: erase_ext 1", stm32bl_erase_ext, 2, 0),
//		SHELL_CMD(write_protect, NULL, "Execute STM32 BL Write Protect cmd", stm32bl_write_protect),
//		SHELL_CMD(write_unprotect, NULL, "Execute STM32 BL Write Unprotect cmd", stm32bl_write_unprotect),
//		SHELL_CMD(readout_protect, NULL, "Execute STM32 BL Readout Protect cmd", stm32bl_readout_protect),
//		SHELL_CMD(readout_unprotect, NULL, "Execute STM32 BL Readout Unprotect cmd", stm32bl_readout_unprotect),
//		SHELL_CMD(checksum_get, NULL, "Execute STM32 BL Checksum Get cmd", stm32bl_checksum_get),
		SHELL_CMD_ARG(flashbin, NULL, "Erase and flash a BIN file to the target device. Usage: stm32bl flashbin fw.bin", stm32bl_flashbin, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(stm32bl, &stm32bl_subcmds, "STM32 Bootloader Host Commands", NULL);
