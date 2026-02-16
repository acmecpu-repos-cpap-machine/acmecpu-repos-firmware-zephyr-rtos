/*
 * Copyright (c) 2021 Acme CPU
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

#define SD_MOUNT_POINT				CONFIG_BSP_FS_SD_CARD_FAT_MOUNT_POINT
#define LOG_CURR_FILE_MAX_SIZE_MB	CONFIG_APP_LOGGER_MAX_CURR_FILESIZE_MB
#define LOG_MAX_NUM_FILES			CONFIG_APP_LOGGER_MAX_FILE_COUNT
#define LOG_MAX_FN_LEN				CONFIG_FS_FATFS_MAX_LFN
#define LOG_MAX_DIR_LEN				32

#define LOG_DIRECTORY_NAME		"log"
#define LOG_DIRECTORY_PATH		SD_MOUNT_POINT "/" LOG_DIRECTORY_NAME
#define LOG_CURR_FILE_NAME		"current.log"
#define LOG_CURR_FILE_PATH		LOG_DIRECTORY_PATH "/" LOG_CURR_FILE_NAME

#define LOG_CURR_FILE_MAX_SIZE_BYTES	(LOG_CURR_FILE_MAX_SIZE_MB * 1000000)
//#define LOG_CURR_FILE_MAX_SIZE_BYTES	(LOG_CURR_FILE_MAX_SIZE_MB * 100000)

static bool m_panic_mode=false;
static uint8_t m_output_buf[CONFIG_LOG_BACKEND_SD_CARD_MAX_BUF_SIZE];
static struct data_rotation m_dr = {
		.curr_fname = LOG_CURR_FILE_NAME,
		.curr_fname_path = LOG_CURR_FILE_PATH,
		.oper_dir = LOG_DIRECTORY_PATH,
		.curr_max_fsize = LOG_CURR_FILE_MAX_SIZE_BYTES,
		.max_fcount = LOG_MAX_NUM_FILES,
};

const struct log_backend *app_logger_backend_sdcard_get(void);

#if 0
static int make_filename_with_path(const char *filepath, const char *new_name, char *out_name) {

	if ((filepath == NULL) || (new_name == NULL) || (out_name == NULL))		return -EINVAL;
	if (strlen(new_name) > LOG_MAX_FN_LEN)									return -ENAMETOOLONG;

	/* Concatenate filepath with new_filename and make new file_name_with_path */
//	int len = strlen(filepath) + strlen(new_name) + 1;

	strcpy(out_name, filepath);
	strcat(out_name, "/");
	strcat(out_name, new_name);

	return 0;
}

static int get_oldest_filename_with_path(const char *searchpath, char *out_name_path) {
	/* Get a slist of file names from the LOG directory */
	int ret = 0;
	struct fname_node *fname_node;
	sys_slist_t fname_list;

	/* Initialize a slist */
	sys_slist_init(&fname_list);

	/* Get file names in the list */
	ret = bsp_fs_file_list_get(searchpath, &fname_list);

	/* Search for the current.log and delete from the list */
	SYS_SLIST_FOR_EACH_CONTAINER(&fname_list, fname_node, node) {
		if (!strcmp(LOG_CURR_FILE_NAME, fname_node->fname)) {
			sys_slist_find_and_remove(&fname_list, &fname_node->node);
			free(fname_node);
		}
	}

	/* Iterate the name slist and find the smallest number */
	uint32_t min = UINT_MAX;
	SYS_SLIST_FOR_EACH_CONTAINER(&fname_list, fname_node, node) {
		uint32_t i_name = atoi(fname_node->fname);
		if (min > i_name) {
			min = i_name;
		}
	}

	/* Smallest number is the oldest file */
	char temp_fname[LOG_MAX_FN_LEN + 1] = {0};
	sprintf(temp_fname, "%u", min);
	make_filename_with_path(searchpath, temp_fname, out_name_path);

	/* Delete the slist */
	SYS_SLIST_FOR_EACH_CONTAINER(&fname_list, fname_node, node) {
		sys_slist_find_and_remove(&fname_list, &fname_node->node);
		free(fname_node);
	}

	return ret;
}
#endif

static int line_out(uint8_t *data, size_t length, void *output_ctx)
{
	size_t ret;

#if 0
	/* Log Rotation logic
	 * - if the current log file reaches max size
	 * - rename the file to keep a backup and open a new current file for logging
	 * - before renaming check if the max allowed file count has reached
	 * - if reached, delete the oldest file, then do the renaming
	 * - in this process the logs are rotated
	 * */

	/* Check if current file size has reached max size */
	uint32_t file_size_bytes;
	ret = bsp_fs_get_file_size(LOG_CURR_FILE_PATH, &file_size_bytes);
	if ((ret == 0) && (file_size_bytes >= LOG_CURR_FILE_MAX_SIZE_BYTES)) {
		/* Max size reached, check log file count */
		uint32_t log_file_count = 1;
		ret = bsp_fs_count_files(LOG_DIRECTORY_PATH, 0, &log_file_count);
		if (log_file_count >= LOG_MAX_NUM_FILES) {
			/* Max file count reached, delete the oldest file */
			char oldest_file[LOG_MAX_DIR_LEN + LOG_MAX_FN_LEN + 1] = { 0 };
			ret = get_oldest_filename_with_path(LOG_DIRECTORY_PATH, oldest_file);
			ret = bsp_fs_delete_file(oldest_file);
		}
		/* Take backup of the current file (rename the current file) */
		uint32_t ts = k_uptime_get_32();
		char a_ts[LOG_MAX_FN_LEN + 1] = { 0 };
		sprintf(a_ts, "%u", ts);
		char new_filename[LOG_MAX_DIR_LEN + LOG_MAX_FN_LEN + 1] = { 0 };
		ret = make_filename_with_path(LOG_DIRECTORY_PATH, a_ts, new_filename);
		ret = bsp_fs_file_rename(LOG_CURR_FILE_PATH, new_filename);
	}
#endif

	ret = bsp_fs_perform_data_rotation(&m_dr);
	ret = bsp_fs_create_append_file(LOG_CURR_FILE_PATH, (const void *)data, length);
	if (ret == length) {
		return length;
	} else {
		return 0;
	}
}

LOG_OUTPUT_DEFINE(log_output_sdcard, line_out, m_output_buf, sizeof(m_output_buf));

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

	log_output_msg_process(&log_output_sdcard, msg, flags);

/*
	log_output_msg_process(&log_output_sdcard, msg,
				       LOG_OUTPUT_FLAG_FORMAT_SYSLOG 		|
					   LOG_OUTPUT_FLAG_TIMESTAMP			|
					   LOG_OUTPUT_FLAG_FORMAT_TIMESTAMP		|
					   LOG_OUTPUT_FLAG_LEVEL);
*/

	log_msg_put(msg);

}

static void app_logger_backend_sdcard_panic(struct log_backend const *const backend)
{
	m_panic_mode = true;
}

static void app_logger_backend_sdcard_init(const struct log_backend *const backend)
{
//	int ret = 0;

	log_backend_deactivate(app_logger_backend_sdcard_get());
}

const struct log_backend_api log_backend_sdcard_api = {
	.panic = app_logger_backend_sdcard_panic,
	.init = app_logger_backend_sdcard_init,
	.put = IS_ENABLED(CONFIG_LOG_IMMEDIATE) ? NULL : send_output,
	.put_sync_string = NULL,
	.put_sync_hexdump = NULL,
};

/* Note that the backend can be activated only after we have
 * sd card mounted successfully so we must not start it immediately.
 */
LOG_BACKEND_DEFINE(log_backend_sdcard, log_backend_sdcard_api, IS_ENABLED(CONFIG_LOG_BACKEND_SD_CARD_AUTOSTART));

const struct log_backend *app_logger_backend_sdcard_get(void)
{
	return &log_backend_sdcard;
}

