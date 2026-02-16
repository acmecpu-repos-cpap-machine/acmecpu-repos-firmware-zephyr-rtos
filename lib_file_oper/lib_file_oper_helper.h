/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_HELPER_H_
#define SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_HELPER_H_

#include <stdio.h>
#include <stdbool.h>
#include <zephyr/sys/slist.h>
#include <zephyr/fs/fs.h>

struct fname_node {
	sys_snode_t node;
	char fname[32];
};

int lib_file_oper_helper_init();

int lib_file_oper_helper_lsdir_recursive(const char *path);

bool lib_file_oper_helper_sd_card_is_mounted();

int lib_file_oper_helper_mount_sd_card(uint8_t card_idx);

int lib_file_oper_helper_unmount_sd_card(uint8_t card_idx);

size_t lib_file_oper_helper_create_append_file(const char* file_name, const void *data, size_t len);

int lib_file_oper_helper_read_file(size_t argc, char **argv);

int lib_file_oper_helper_make_dir(const char* dir_path);

bool lib_file_oper_helper_dir_exist(const char* dir_path);

int lib_file_oper_helper_delete_file(const char* file_name_with_path);

/**
 * @brief 	Recursively deletes all files and directories present in a given directory
 *
 * @param	dir_path[in]	full path to the directory
 *
 * @return	-EINVAL		incorrect parameter
 * 			-ve			other errors
 * 			0 			Success
 */
int lib_file_oper_helper_delete_all_recursive(const char* dir_path);

int lib_file_oper_helper_count_files(const char* dir_path, uint8_t option, uint32_t *file_count);

int lib_file_oper_helper_get_file_size(const char* file_name_with_path, uint32_t *file_size);

int lib_file_oper_helper_file_rename(const char *from, const char *to);

int lib_file_oper_helper_file_list_get(const char *dir_path, sys_slist_t *file_list);


struct data_rotation {
	const char *curr_fname;			/* name only of the current file which is being updated */
	const char *curr_fname_path;	/* name with path of the current file which is being updated */
	const char *oper_dir;			/* path to the directory where the files are present */
	uint32_t curr_max_fsize;		/* max size of the current file allowed, after which it has to be backed up */
	uint32_t max_fcount;			/* max number of files allowed in the directory, after which oldest will be deleted */
};

int lib_file_oper_helper_perform_data_rotation(struct data_rotation *dr, struct fs_file_t *zfp);

#endif /* SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_HELPER_H_ */
