/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 23-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr.h>
#include <logging/log_backend.h>
#include <logging/log_core.h>
#include <logging/log_msg.h>
#include <logging/log_output.h>
#include <logging/log_backend_std.h>
#include <device.h>
#include <sys/__assert.h>
#include <fs/fs.h>
#include <sys/slist.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bsp_fs_helper.h"

#if CONFIG_APP_LOG_STORAGE_SD_CARD
#define MOUNT_POINT			CONFIG_BSP_FS_SD_CARD_FAT_MOUNT_POINT
#elif CONFIG_APP_LOG_STORAGE_FLASH
#define MOUNT_POINT			CONFIG_BSP_FS_FLASH_FAT_MOUNT_POINT
#endif

#define LOG_CURR_FILE_MAX_SIZE_MB	CONFIG_APP_LOGGER_MAX_CURR_FILESIZE_MB
#define LOG_MAX_NUM_FILES			CONFIG_APP_LOGGER_MAX_FILE_COUNT
#define LOG_MAX_FN_LEN				CONFIG_FS_FATFS_MAX_LFN
#define LOG_MAX_DIR_LEN				32

#define LOG_DIRECTORY_NAME		"log"
#define LOG_DIRECTORY_PATH		MOUNT_POINT "/" LOG_DIRECTORY_NAME
#define LOG_CURR_FILE_NAME		"current.log"
#define LOG_CURR_FILE_PATH		LOG_DIRECTORY_PATH "/" LOG_CURR_FILE_NAME

#define LOG_CURR_FILE_MAX_SIZE_BYTES	(LOG_CURR_FILE_MAX_SIZE_MB * 1000000)
//#define LOG_CURR_FILE_MAX_SIZE_BYTES	(LOG_CURR_FILE_MAX_SIZE_MB * 100000)

static bool m_panic_mode=false;
static uint8_t m_output_buf[CONFIG_LOG_BACKEND_STORAGE_MAX_BUF_SIZE];
static struct data_rotation m_dr = {
		.curr_fname = LOG_CURR_FILE_NAME,
		.curr_fname_path = LOG_CURR_FILE_PATH,
		.oper_dir = LOG_DIRECTORY_PATH,
		.curr_max_fsize = LOG_CURR_FILE_MAX_SIZE_BYTES,
		.max_fcount = LOG_MAX_NUM_FILES,
};

const struct log_backend *app_logger_backend_storage_get(void);

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	size_t ret;

	ret = bsp_fs_perform_data_rotation(&m_dr);
	ret = bsp_fs_create_append_file(LOG_CURR_FILE_PATH, (const void *)data, length);
	if (ret == length) {
		return length;
	} else {
		return 0;
	}
}

LOG_OUTPUT_DEFINE(log_output_storage, line_out, m_output_buf, sizeof(m_output_buf));

static void send_output(const struct log_backend *const backend,
			struct log_msg *msg)
{
	if (m_panic_mode) {
		return;
	}
	log_msg_get(msg);

	uint32_t flags = LOG_OUTPUT_FLAG_LEVEL | LOG_OUTPUT_FLAG_TIMESTAMP;

	if (IS_ENABLED(CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP)) {
		flags |= LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP;
	}

	log_output_msg_process(&log_output_storage, msg, flags);

/*
	log_output_msg_process(&log_output_storage, msg,
				       LOG_OUTPUT_FLAG_FORMAT_SYSLOG 		|
					   LOG_OUTPUT_FLAG_TIMESTAMP			|
					   LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP		|
					   LOG_OUTPUT_FLAG_LEVEL);
*/

	log_msg_put(msg);

}

static void app_logger_backend_storage_panic(struct log_backend const *const backend)
{
	m_panic_mode = true;
}

static void app_logger_backend_storage_init(const struct log_backend *const backend)
{
//	int ret = 0;

	log_backend_deactivate(app_logger_backend_storage_get());
}

const struct log_backend_api log_backend_storage_api = {
	.panic = app_logger_backend_storage_panic,
	.init = app_logger_backend_storage_init,
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : send_output,
	.put_sync_string = NULL,
	.put_sync_hexdump = NULL,
};

/* Note that the backend can be activated only after we have
 * sd card mounted successfully so we must not start it immediately.
 */
LOG_BACKEND_DEFINE(log_backend_storage, log_backend_storage_api, IS_ENABLED(CONFIG_LOG_BACKEND_STORAGE_AUTOSTART));

const struct log_backend *app_logger_backend_storage_get(void)
{
	return &log_backend_storage;
}

