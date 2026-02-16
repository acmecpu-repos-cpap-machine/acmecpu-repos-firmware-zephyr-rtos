/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_BSP_FS_HELPER_H_
#define SRC_INCLUDE_BSP_FS_HELPER_H_

#include <stdio.h>
#include <stdbool.h>
#include <sys/slist.h>

struct fname_node {
	sys_snode_t node;
	char fname[32];
};

int bsp_fs_init();

bool bsp_fs_sd_card_is_mounted();

int bsp_fs_mount_sd_card(uint8_t card_idx);

int bsp_fs_unmount_sd_card(uint8_t card_idx);

size_t bsp_fs_create_append_file(const char* file_name, const void *data, size_t len);

int bsp_fs_read_file(size_t argc, char **argv);

int bsp_fs_make_dir(const char* dir_path);

bool bsp_fs_dir_exist(const char* dir_path);

int bsp_fs_delete_file(const char* file_name_with_path);

int bsp_fs_count_files(const char* dir_path, uint8_t option, uint32_t *file_count);

int bsp_fs_get_file_size(const char* file_name_with_path, uint32_t *file_size);

int bsp_fs_file_rename(const char *from, const char *to);

int bsp_fs_file_list_get(const char *dir_path, sys_slist_t *file_list);


struct data_rotation {
	char *curr_fname;				/* name only of the current file which is being updated */
	char *curr_fname_path;			/* name with path of the current file which is being updated */
	char *oper_dir;					/* path to the directory where the files are present */
	uint32_t curr_max_fsize;		/* max size of the current file allowed, after which it has to be backed up */
	uint32_t max_fcount;			/* max number of files allowed in the directory, after which oldest will be deleted */
};

int bsp_fs_perform_data_rotation(struct data_rotation *dr);

#endif /* SRC_INCLUDE_BSP_FS_HELPER_H_ */
