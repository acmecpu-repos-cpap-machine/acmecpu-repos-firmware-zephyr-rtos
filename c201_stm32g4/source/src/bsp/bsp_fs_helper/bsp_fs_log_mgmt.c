/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <fs/fs.h>
#include <sys/slist.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bsp_fs_helper.h"

#define LOG_MAX_FN_LEN			32 //CONFIG_FS_FATFS_MAX_LFN
#define LOG_MAX_DIR_LEN			32

struct k_sem m_log_mgmt_lock;

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

static int get_oldest_filename_with_path(const char *searchpath, const char *curr_fname, char *out_name_path) {
	/* Get a slist of file names from the LOG directory */
	int ret = 0;
	struct fname_node *fname_node;
	sys_slist_t fname_list;

	/* Initialize a slist */
	sys_slist_init(&fname_list);

	/* Get file names in the list */
	ret = bsp_fs_file_list_get(searchpath, &fname_list);

	/* Each directory will have a current file which is always being updated (current.log, current.data)
	 * So, we search for the current file and delete from the list
	 * Hence this function take the curr_fname as input
	 * */
	SYS_SLIST_FOR_EACH_CONTAINER(&fname_list, fname_node, node) {
		if (!strcmp(curr_fname, fname_node->fname)) {
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

int bsp_fs_log_mgmt_init() {
	k_sem_init(&m_log_mgmt_lock, 1, 1);
	return 0;
}

int bsp_fs_perform_data_rotation(struct data_rotation *dr) {
	int ret = 0;

	k_sem_take(&m_log_mgmt_lock, K_FOREVER);

	const char *curr_fname = dr->curr_fname;
	const char *curr_fname_path = dr->curr_fname_path;
	const char *oper_dir = dr->oper_dir;
	const uint32_t curr_max_fsize = dr->curr_max_fsize;
	const uint32_t max_fcount = dr->max_fcount;

	/* Log Rotation logic
	 * - if the current log file reaches max size
	 * - rename the file to keep a backup and open a new current file for logging
	 * - before renaming check if the max allowed file count has reached
	 * - if reached, delete the oldest file, then do the renaming
	 * - in this process the logs are rotated
	 * */

	/* Check if current file size has reached max size */
	uint32_t file_size_bytes;
	ret = bsp_fs_get_file_size(curr_fname_path, &file_size_bytes);
	if ((ret == 0) && (file_size_bytes >= curr_max_fsize)) {
		/* Max size reached, check log file count */
		uint32_t log_file_count = 1;
		ret = bsp_fs_count_files(oper_dir, 0, &log_file_count);
		if (log_file_count >= max_fcount) {
			/* Max file count reached, delete the oldest file */
			char oldest_file[LOG_MAX_DIR_LEN + LOG_MAX_FN_LEN + 1] = { 0 };
			ret = get_oldest_filename_with_path(oper_dir, curr_fname, oldest_file);
			ret = bsp_fs_delete_file(oldest_file);
		}
		/* Take backup of the current file (rename the current file) */
		uint32_t ts = k_uptime_get_32();
		char a_ts[LOG_MAX_FN_LEN + 1] = { 0 };
		sprintf(a_ts, "%u", ts);
		char new_filename[LOG_MAX_DIR_LEN + LOG_MAX_FN_LEN + 1] = { 0 };
		ret = make_filename_with_path(oper_dir, a_ts, new_filename);
		ret = bsp_fs_file_rename(curr_fname_path, new_filename);
	}

	k_sem_give(&m_log_mgmt_lock);

	return ret;
}
