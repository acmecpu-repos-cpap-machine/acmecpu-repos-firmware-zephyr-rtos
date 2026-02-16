/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 4-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_dfu);

#include "lib_stm32bl_host/lib_stm32bl_host.h"
#include "app_storage/app_storage.h"

#define HIGH	1
#define LOW		0

#define MSG_PASS	"OK"
#define MSG_FAIL	"ERR"

static bool m_device_in_bl_mode = false;
//static uint32_t m_rw_chunk = 256;

static uint32_t addr_byte_swap(uint32_t x)
{
	uint32_t tmp1, tmp2, tmp3, tmp4;
	tmp4 = (x & 0x000000FF ) << 24;
	tmp3 = (x & 0x0000FF00 ) << 8;
	tmp2 = (x & 0x00FF0000 ) >> 8;
	tmp1 = (x & 0xFF000000 ) >> 24;

	return (tmp4 | tmp3 | tmp2 | tmp1);
}

static int bootloader_mode_put()
{
	int ret = 0;
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
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
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
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

static int stm32bl_start()
{
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
	} else {
		LOG_ERR("lib_stm32bl_host_start_check failed, %d", ret);
	}

	bootloader_mode_exit();
	return ret;
}

static int stm32bl_erase_ext(size_t argc, char **argv)
{
	if (argc != 2) {
		LOG_ERR(MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		LOG_ERR("device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}

	uint32_t erase_spl = strtol(argv[1], NULL, 10);
	if (erase_spl != 1) {
		LOG_ERR("not implemented, only mass erase is supported. Usage: erase_ext 1");
		return -1;
	}

	uint8_t spl_erase[2] = {0xFF, 0xFF};
	int ret = lib_stm32bl_host_cmd_execute_ext_erase_mem(0, NULL, spl_erase);
	if (!ret) {
		LOG_INF(MSG_PASS);
	} else {
		LOG_ERR(MSG_FAIL", %d", ret);
	}

	return ret;
}

static int stm32bl_write_mem(size_t argc, char **argv)
{
	if (argc != 3) {
		LOG_ERR(MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	if (!m_device_in_bl_mode) {
		LOG_ERR("device not into bootloader mode, run stm32bl start command first!");
		return -1;
	}
	int ret = 0;
	uint32_t addr = strtol(argv[1], NULL, 16);	// start addr

//	char fname[256] = {0x00};
	char fname_with_path[256] = {0x00};
	strcpy(fname_with_path, argv[2]);			// filename to save

//	strcpy(fname, argv[2]);						// filename to save
//	strcpy(fname_with_path, app_storage_mount_point_get());
//	strcat(fname_with_path, "/");
//	strcat(fname_with_path, fname);

	/* open the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, fname_with_path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("file %s open/create failed!", fname_with_path);
		LOG_ERR(MSG_FAIL": file %s open failed!", fname_with_path);
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
				LOG_ERR(MSG_FAIL": fs_read failed");
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
		} else {
			LOG_ERR(MSG_FAIL", %d", ret);
			goto err;
		}
	}

err:
	/* close the file */
	fs_close(&zfp);

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
	ret = fs_open(&zfp, bin_fname_with_path, FS_O_READ);
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

int stm32bl_flashbin(const char *img_path)
{
#define ARGC_MAX		5
#define ARGV_LEN_MAX	20
	int ret = 0;
	char *targv[ARGC_MAX + 1] = {0}; /* +1 reserved for NULL */

	/* start */
	ret = stm32bl_start();
	if (ret) goto err;

	/* get_id */

	/* mass erase */
	LOG_INF("#### Erasing ...");
	targv[0] = "erase_ext ";
	targv[1] = "1";
	ret = stm32bl_erase_ext(2, &targv[0]);
	if (ret) goto err;
	else	LOG_INF("Mass erase successful");

	/* program */
	LOG_INF("#### Writing ...");
	targv[0] = "write_mem ";
	targv[1] = "08000000 ";
	targv[2] = (char*) img_path;
	ret = stm32bl_write_mem(3, &targv[0]);
	if (ret) goto err;
	else	LOG_INF("Writing successful");

	k_sleep(K_MSEC(100));

	/* verify */
	LOG_INF("#### Verifying ...");
	char fname_with_path[256] = {0x00};
//	char fname[256] = {0x00};
//	strcpy(fname, argv[1]);						// filename to save
//	strcpy(fname_with_path, app_storage_mount_point_get());
//	strcat(fname_with_path, "/");
//	strcat(fname_with_path, fname);

	strcpy(fname_with_path, img_path);			// filename to save
	ret = verify_program(fname_with_path, 0x08000000);
	if (ret) goto err;
	else	LOG_INF("Verify successful");

	/* go */

err:
	if (ret !=0)
		LOG_ERR(MSG_FAIL": Flashing failed, %d", ret);
	else
		LOG_INF("Flashing successful");
	return ret;
}
