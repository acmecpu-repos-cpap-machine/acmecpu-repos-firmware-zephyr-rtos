/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jun-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_DFU_APP_DFU_H_
#define SRC_INCLUDE_APP_DFU_APP_DFU_H_

#include <stdint.h>
#include "app_storage/app_storage.h"

/* firmware download file constants */
#define FW_DIR_NAME						CONFIG_FW_DIR_NAME //"fw"
#define FW_DIR_PATH						APP_STORAGE_MOUNT_POINT_FLASH "/" FW_DIR_NAME
#define FW_FILE_MAX_FILE_COUNT			CONFIG_FW_FILE_MAX_FILE_COUNT

/* main.bin file for the app processor */
#define FW_MAIN_FILE_NAME				CONFIG_FW_MAIN_FILE_NAME //"main.bin"
#define FW_MAIN_FILE_PATH				FW_DIR_PATH "/" FW_MAIN_FILE_NAME
#define FW_MAIN_FILE_MAX_SIZE_BYTES		(CONFIG_FW_MAIN_FILE_MAX_KB * 1024)

/* net.bin file for the network processor */
#define FW_NET_FILE_NAME				CONFIG_FW_NET_FILE_NAME //"net.bin"
#define FW_NET_FILE_PATH				FW_DIR_PATH "/" FW_NET_FILE_NAME
#define FW_NET_FILE_MAX_SIZE_BYTES		(CONFIG_FW_NET_FILE_MAX_KB * 1024)

/* blwdrv.bin file for the bldc driver co-processor */
#define FW_BLWDRV_FILE_NAME				CONFIG_FW_BLWDRV_FILE_NAME //"blwdrv.bin"
#define FW_BLWDRV_FILE_PATH				FW_DIR_PATH "/" FW_BLWDRV_FILE_NAME
#define FW_BLWDRV_FILE_MAX_SIZE_BYTES	(CONFIG_FW_BLWDRV_FILE_MAX_KB * 1024)


typedef enum {
	APP_DFU_FW_MAIN = 0,
	APP_DFU_FW_NET,
	APP_DFU_FW_BLWDRV,

	APP_DFU_FW_MAX
} APP_DFU_FW_TYPE;

/**
 * @brief	Writes the main.bin firmware in to slot1_partition and verifies the same
 * @param img_path[in]	file name with full path of the new main.bin firmware
 * @return
 * 	0			Write and verify operation successful
 * 	-EILSEQ		Verify failed
 * 	other -ve	fail
 */
int app_dfu_main_bin_program(const char *img_path);

/**
 * @brief	Sends the net.bin firmware to external co-processor using
 * 			uart_m2m_comm protocol
 * @param img_path[in]	file name with full path of the new net.bin firmware
 * @return
 * 	0			Write and verify operation successful
 * 	-EILSEQ		Verify failed
 * 	other -ve	fail
 */
int app_dfu_net_bin_program(const char *img_path);

/**
 * @brief	Programs the blwdrv.bin firmware to external co-processor using
 * 			stm32 bootloader host library
 *
 * @note	Before calling this function the uart_m2m_comm with BLDC co-processor
 * 			must be stopped. The application must ensure that no thread is
 * 			trying to communicate with the BLDC co-processor. This is because,
 * 			the DFU module will change the UART hardware configurations to
 * 			communicate with the external BLDC co-processor.
 *
 * @param img_path[in]	file name with full path of the new blwdrv.bin firmware
 * @return
 * 	0			Write and verify operation successful
 * 	-EILSEQ		Verify failed
 * 	other -ve	fail
 */
int app_dfu_blwdrv_bin_program(const char *img_path);

/**
 * @brief	Programs (writes) the bin to the memory
 * @param img_op[in]	image option, possible values are defined in APP_DFU_FW_TYPE
 * @return
 * 	0			Write and verify operation successful
 * 	-EILSEQ		Verify failed
 * 	other -ve	fail
 */
int app_dfu_fw_program(uint32_t img_op);

/**
 * @brief 	Marks the image in slot1 as pending. On the next reboot, the system
 * 			will perform a boot of the slot1 image. The image is marked as
 * 			test mode so the new image should mark itself as permanent else
 * 			the slot0 image will be rolled back.
 * @return
 * 	0 on success, negative errno code on fail.
 */
int app_dfu_upgrade_test();

/**
 * @brief	Marks the currently running image as confirmed.
 * @return
 * 	0 on success, negative errno code on fail.
 */
int app_dfu_upgrade_permanent();

/**
 * @brief	Tells the network processor to update to the new firmware
 * @return
 * 	0 on success, negative errno code on fail.
 */
int app_dfu_upgrade_net();

int app_dfu_netproc_fw_send(uint32_t cmd);

#endif /* SRC_INCLUDE_APP_DFU_APP_DFU_H_ */
