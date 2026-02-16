/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <device.h>
#include <sys/__assert.h>
#include <fs/fs.h>
#include <sys/slist.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(app_data_rec);

#include "app_storage/app_storage.h"
#include "bsp_fs_helper.h"

#if CONFIG_APP_LOG_STORAGE_SD_CARD
#define MOUNT_POINT			CONFIG_BSP_FS_SD_CARD_FAT_MOUNT_POINT
#elif CONFIG_APP_LOG_STORAGE_FLASH
#define MOUNT_POINT			CONFIG_BSP_FS_FLASH_FAT_MOUNT_POINT
#endif

#define DATA_CURR_FILE_MAX_SIZE_MB	CONFIG_APP_DATA_RECORDER_MAX_CURR_FILESIZE_MB
#define DATA_MAX_NUM_FILES			CONFIG_APP_DATA_RECORDER_MAX_FILE_COUNT
#define LOG_MAX_FN_LEN				CONFIG_FS_FATFS_MAX_LFN
#define LOG_MAX_DIR_LEN				32

#define DATA_DIRECTORY_NAME		"data"
#define DATA_DIRECTORY_PATH		APP_STORAGE_MOUNT_POINT_FLASH "/" DATA_DIRECTORY_NAME
#define DATA_CURR_FILE_NAME		"current.dat"
#define DATA_CURR_FILE_PATH		DATA_DIRECTORY_PATH "/" DATA_CURR_FILE_NAME

#define DATA_CURR_FILE_MAX_SIZE_BYTES	(DATA_CURR_FILE_MAX_SIZE_MB * 1000000)

struct k_sem m_data_lock;

static struct data_rotation m_dr = {
		.curr_fname = DATA_CURR_FILE_NAME,
		.curr_fname_path = DATA_CURR_FILE_PATH,
		.oper_dir = DATA_DIRECTORY_PATH,
		.curr_max_fsize = DATA_CURR_FILE_MAX_SIZE_BYTES,
		.max_fcount = DATA_MAX_NUM_FILES,
};

int app_data_recorder_init() {
	int ret = 0;
	k_sem_init(&m_data_lock, 1, 1);

/*
	ret = bsp_fs_mount_sd_card(0);
	if (ret != 0) {
		LOG_ERR("bsp_fs_mount_sd_card failed");
		return ret;
	}
*/

	/* Check and create the log directory if it doesn't exist */
//	if (bsp_fs_sd_card_is_mounted()) {
		if (!bsp_fs_dir_exist(DATA_DIRECTORY_PATH)) {
			bsp_fs_make_dir(DATA_DIRECTORY_PATH);
		}
//	} else {
//		LOG_ERR("app_data_recorder_init failed due to SD Card not mounted");
//	}

	/* TODO Initialize a thread to manage incoming data and saving them to file, so that the process becomes non-blocking */

	return ret;
}

int app_data_recorder_save(const char *data, size_t length) {
	int ret = 0;

	if ((data == NULL) || length <= 0) {
		LOG_ERR("Invalid params to app_data_recorder_save");
		return -EINVAL;
	}

	/* TODO Enqueue the incoming for the thread to process
	 * Now proceeding as a blocking function
	 * */

	k_sem_take(&m_data_lock, K_FOREVER);

	ret = bsp_fs_perform_data_rotation(&m_dr);
	ret = bsp_fs_create_append_file(DATA_CURR_FILE_PATH, data, length);
	if (ret == length) {
		ret = 0;
	} else {
		goto err;
	}

err:
	k_sem_give(&m_data_lock);
	return ret;
}
