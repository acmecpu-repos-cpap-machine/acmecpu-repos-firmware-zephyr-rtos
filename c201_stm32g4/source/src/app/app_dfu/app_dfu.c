/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jun-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_dfu);

#include "app_dfu/app_dfu.h"
#include "app_dfu_stm32bl.h"
#include "app_dfu_stm32bl_uart.h"
#include "app_dfu_netproc_update.h"
#include "lib_stm32bl_host/lib_stm32bl_host.h"
#include "lib_events/lib_events.h"

int app_dfu_main_bin_program(const char *img_path)
{
	if (img_path == NULL) return -EINVAL;
	int ret = 0;

	/**
	 * erase partition
	 * */
	const struct flash_area *pfa;
	uint8_t id = FIXED_PARTITION_ID(slot1_partition);
	uint32_t sec_cnt;
	struct flash_sector fs_sectors[16];
	uint8_t wr[256] = {0x00};
	uint8_t rd[256] = {0x00};

	/* open and get the flash_area object */
	ret = flash_area_open(id, &pfa);
	if (ret < 0) {
		LOG_ERR("flash_area_open failed");
		return ret;
	}

	/* First erase the area so it's ready for use. */
	const struct device *flash_dev = flash_area_get_device(pfa);
	ret = flash_erase(flash_dev, pfa->fa_off, pfa->fa_size);
	if (ret < 0) {
		LOG_ERR("flash_erase failed");
		return ret;
	}

	/**
	 * write image to partition
	 * */
	/* get the number of sectors in the area */
	sec_cnt = ARRAY_SIZE(fs_sectors);
	ret = flash_area_get_sectors(id, &sec_cnt, fs_sectors);
	if (ret < 0) {
		LOG_ERR("flash_area_get_sectors failed");
		return ret;
	}

	LOG_INF("sector count = %d", sec_cnt);

	/* open the file */
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, img_path, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("file %s open failed!", img_path);
		return ret;
	}

	ssize_t rd_len=0;
	int wr_tot=0, wr_done=0;
	int off = 0;
//	for (int i=0; i<sec_cnt; i++) {
		do {
			rd_len = fs_read(&zfp, wr, sizeof(wr));
			if (rd_len > 0) {
				ret = flash_area_write(pfa, off, wr, rd_len);
				if (ret < 0) {
					LOG_ERR("write: flash_area_write failed, %d", ret);
					goto err;
				}
				off += rd_len;
			} else if (rd_len == 0) {
				wr_done = 1;
				break;
			} else if (rd_len < 0) {
				LOG_ERR("write: fs_read failed, %d", rd_len);
				ret = -1;
				goto err;
			}
		} while (1);//(off < fs_sectors[i].fs_size);

		wr_tot += off;
		if (wr_done) {
			LOG_INF("write complete, %d bytes", wr_tot);
//			break;
		}
//	}

	/**
	 * verify image
	 * */
	ret = fs_seek(&zfp, 0, FS_SEEK_SET);
	if (ret < 0) {
		LOG_ERR("fs_fseek failed");
		goto err;
	}

	int rd_tot=0, ver_done=0;;
	off = 0;
//	for (int i=0; i<sec_cnt; i++) {
		do {
			/* read from file */
			rd_len = fs_read(&zfp, wr, sizeof(wr));
			if (rd_len > 0) {
				/* read from flash */
				ret = flash_area_read(pfa, off, rd, rd_len);
				if (ret < 0) {
					LOG_ERR("verify: flash_area_read failed");
					goto err;
				}
				LOG_HEXDUMP_DBG(rd, sizeof(rd), "rd");

				/* compare */
				if (memcmp(wr, rd, rd_len) != 0) {
					LOG_ERR("verify failed");
					ret = -EILSEQ;
					goto err;
				}
				off += rd_len;
			} else if (rd_len == 0) {
				ver_done = 1;
				break;
			} else if (rd_len < 0) {
				LOG_ERR("verify: fs_read failed, %d", rd_len);
				ret = -1;
				goto err;
			}
		} while(1); //while (off < fs_sectors[i].fs_size);

		rd_tot += off;
		if (ver_done && (rd_tot == wr_tot)) {
			LOG_INF("verify successful");
//			break;
		}
//	}

err:
	fs_close(&zfp);

	return ret;
}

int app_dfu_net_bin_program(const char *img_path)
{
	if (img_path == NULL) return -EINVAL;
	int ret = 0;
	/**
	 * The network bin file must be downloaded prior to calling this function
	 *
	 * Sequence of operation:
	 * 1. App processor tells the network processor that a new firmware is available
	 * 	  by sending command C20X_M2M_CMD_NET_FWAPP_AVAIL
	 * 2. The network processor prepares itself and sends command C20X_M2M_CMD_NET_FWAPP_GET
	 *    to the app processor to get the bin file in chunks via
	 *    Data Request-Response-Ack m2m_comm protocol
	 * 3. Finally the app processor tells the network processor to boot the new firmware
	 *    by sending C20X_M2M_CMD_NET_FWAPP_UPDATE command
	 * */

	// 1
	ret = app_dfu_netproc_fw_available_send();

	return ret;
}

int app_dfu_blwdrv_bin_program(const char *img_path)
{
	if (img_path == NULL) return -EINVAL;
	int ret = 0;

	/**
	 * Configure uart interface for programming
	 */
	app_dfu_stm32bl_uart_configure();
	if (ret != 0) {
		LOG_ERR("app_dfu_stm32bl_uart_configure failed");
		return -1;
	}

	/**
	 * Initialize stm32 bootloader library
	 */
	struct lib_stm32bl_host_funcs funcs;
	funcs.ms_delay = app_dfu_stm32bl_uart_ms_delay;
	funcs.usart_close = app_dfu_stm32bl_uart_close;
	funcs.usart_open = app_dfu_stm32bl_uart_open;
	funcs.usart_recv = app_dfu_stm32bl_uart_read_bytes;
	funcs.usart_send = app_dfu_stm32bl_uart_write_bytes;
	lib_stm32bl_host_init(&funcs);

	/**
	 * stm32 bootloader program and verify
	 */
	ret = stm32bl_flashbin(img_path);
	if (!ret) {
		LOG_INF("Programming successful");
	} else {
		LOG_ERR("Programming failed");
	}
	return ret;
}

int app_dfu_fw_program(uint32_t img_op)
{
	int ret = 0;

	switch (img_op) {
	case APP_DFU_FW_MAIN:
		ret = app_dfu_main_bin_program(FW_MAIN_FILE_PATH);
		lib_events_report_event(LIB_EVENT_FW_PROGRAM_COMPLETED);
		break;
	case APP_DFU_FW_NET:
		ret = app_dfu_net_bin_program(FW_NET_FILE_PATH);
		break;
	case APP_DFU_FW_BLWDRV:
		ret = app_dfu_blwdrv_bin_program(FW_BLWDRV_FILE_PATH);
		lib_events_report_event(LIB_EVENT_FW_PROGRAM_COMPLETED);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

int app_dfu_upgrade_test()
{
	int ret = 0;
#if CONFIG_BOOTLOADER_MCUBOOT
	ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
#endif
	if (ret == 0)
		LOG_INF("next reboot will swap");
	else
		LOG_ERR("boot_request_upgrade failed");
	return ret;
}

int app_dfu_upgrade_permanent()
{
	int ret = 0;
#if CONFIG_BOOTLOADER_MCUBOOT
	ret = boot_write_img_confirmed();
#endif
	if (ret == 0)
		LOG_INF("Image marked permanent");
	else
		LOG_ERR("boot_write_img_confirmed failed");
	return ret;
}

int app_dfu_upgrade_net()
{
	return app_dfu_netproc_fw_upgrade();
}

