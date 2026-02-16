/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <version.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/slist.h>
#if (CONFIG_FAT_FILESYSTEM_ELM)
	#include <ff.h>
#endif
#if (CONFIG_FILE_SYSTEM_LITTLEFS)
	#include <zephyr/fs/littlefs.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(lib_file);

#include "lib_file_oper_helper.h"

#define PRINTK_INF	0	// to log or not log the printk info messages

static struct k_sem m_fs_lock;

int lib_file_oper_helper_log_mgmt_init();

int lib_file_oper_helper_lsdir_recursive(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", path, res);
		return res;
	}

	LOG_INF("Listing dir %s:", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			if (res < 0) {
				LOG_ERR("Error reading dir [%d]", res);
			}
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s\n", entry.name);
			char new_path[300] = {0x00};
			sprintf(new_path, "%s/%s", path, entry.name);
			lib_file_oper_helper_lsdir_recursive(new_path);
		} else {
			char fullpath[300] = {0x00};
			sprintf(fullpath, "%s/%s", path, entry.name);
			LOG_INF("[FILE] %s, %s (size = %zu)", entry.name, fullpath, entry.size);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}


static int lib_file_oper_helper_count_dir_files(const char *dir_path, uint32_t *dir_count, uint32_t *file_count) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	uint32_t d_count=0;
	uint32_t f_count=0;

	/* Verify fs_opendir() */
	fs_dir_t_init(&dirp);
	res = fs_opendir(&dirp, dir_path);
	if (res) {
		printk("Error opening dir %s [%d]\n", dir_path, res);
		return res;
	}
#if PRINTK_INF
	printk("\nListing dir %s ...\n", dir_path);
#endif
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			d_count++;
#if PRINTK_INF
			printk("[DIR ] %s\n", entry.name);
#endif
		} else if (entry.type == FS_DIR_ENTRY_FILE){
			f_count++;
#if PRINTK_INF
			printk("[FILE] %s (size = %zu)\n", entry.name, entry.size);
#endif
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	if (res == 0) {
		*dir_count = d_count;
		*file_count = f_count;
	} else {
		*dir_count = 0;
		*file_count = 0;
	}

	return res;
}

int lib_file_oper_helper_init() {
	k_sem_init(&m_fs_lock, 1, 1);
	lib_file_oper_helper_log_mgmt_init();
	return 0;
}

int lib_file_oper_helper_file_list_get(const char *dir_path, sys_slist_t *file_list) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	k_sem_take(&m_fs_lock, K_FOREVER);

	/* Verify fs_opendir() */
	fs_dir_t_init(&dirp);
	res = fs_opendir(&dirp, dir_path);
	if (res) {
		printk("Error opening dir %s [%d]\n", dir_path, res);
		goto err;
	}

	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
#if PRINTK_INF
			printk("[DIR ] %s\n", entry.name);
#endif
		} else if (entry.type == FS_DIR_ENTRY_FILE){
			/* Allocate a node, add data, append to list */
			struct fname_node *fnode = (struct fname_node *) calloc(1, sizeof(struct fname_node));
			if (fnode) {
				strcpy(fnode->fname, entry.name);
				sys_slist_append(file_list, &fnode->node);
			}
#if PRINTK_INF
			printk("[FILE] %s (size = %zu)\n", entry.name, entry.size);
#endif
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

err:
	k_sem_give(&m_fs_lock);
	return res;
}

size_t lib_file_oper_helper_create_append_file(const char* file_name, const void *data, size_t len) {
#if PRINTK_INF
	printk("lib_file_oper_helper_create_append_file: file_name=%s, length=%d\n", file_name, len);
#endif
	int res=0;
	size_t wrbytes=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
//	memset(&zfp, 0x00, sizeof(struct fs_file_t));

	/* Open the file */
	res = fs_open(&zfp, file_name, (FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND));
	if (res == 0) {
	} else {
		printk("file create/open failed!");
		wrbytes = res;
		goto err;
	}

	/* Do write operations here */
	size_t datalen = len;
	wrbytes = fs_write(&zfp, data, datalen);
	if (wrbytes == datalen) {
//		printk("%d bytes written to file", wrbytes);
	} else if (wrbytes > 0 && wrbytes < datalen) {
//		printk("less bytes written, %d bytes", wrbytes);
	} else if (wrbytes == -ENOTSUP) {
//		printk("not implemented by underlying file system driver");
		res = -ENOTSUP;
	} else if (wrbytes < 0) {
		printk("could not write to file");
		res = -1;
	}

	if (res < 0) {
		wrbytes = res;
		goto err;
	}

	/* Close the file */
	res = fs_close(&zfp);
	if (res == 0) {
//		printk("file closed");
	} else {
		printk("could not close file");
	}

err:
	k_sem_give(&m_fs_lock);
	return wrbytes;
}

/* TODO agruments of read_file function has to be changed to support data get */
int lib_file_oper_helper_read_file(size_t argc, char **argv) {
#if PRINTK_INF
	printk("read_file: argc=%d, argv[1]=%s\n", argc, argv[1]);
#endif
	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
//	memset(&zfp, 0x00, sizeof(struct fs_file_t));

	/* Open the file */
	res = fs_open(&zfp, argv[1], (FS_O_CREATE | FS_O_READ));
	if (res == 0) {
//		printk("file opened");
	} else if (res == -EINVAL) {
		printk("bad file name");
	} else if (res == -EROFS) {
		printk("read only file / file system");
	} else if (res == -ENOENT) {
		printk("file path not possible");
	} else {
		printk("failed to open file");
	}

	/* Do read operations here */
	char rdbuf = '\0';
	size_t rdbytes = 0, totbytes = 0;

	printk("Data read = ");
	do {
		rdbytes = fs_read(&zfp, &rdbuf, 1);
		printk("%c", rdbuf);
		totbytes++;
	} while (rdbytes > 0);
	printk("%d bytes read", totbytes);

	/* Close the file */
	res = fs_close(&zfp);
	if (res == 0) {
//		printk("file closed");
	} else {
		printk("could not close file");
	}

//err:
	k_sem_give(&m_fs_lock);
	return 0;
}

int lib_file_oper_helper_delete_file(const char* file_name_with_path) {

	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	res = fs_unlink(file_name_with_path);
	if (res == 0) {
//		printk("delete successful\n");
	} else if (res == -EROFS) {
		printk("file system is readonly\n");
	} else if (res == -ENOTSUP) {
		printk("not implemented by underlying file system driver\n");
	} else {
		printk("could not delete\n");
	}

//err:
	k_sem_give(&m_fs_lock);
	return res;
}

int lib_file_oper_helper_delete_all_recursive(const char* dir_path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, dir_path);
	if (res) {
		LOG_ERR("Error opening dir %s [%d]", dir_path, res);
		return res;
	}

	LOG_INF("Deleting files and directories in %s:", dir_path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			if (res < 0) {
				LOG_ERR("Error reading dir [%d]", res);
			}
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			LOG_INF("[DIR ] %s\n", entry.name);
			char new_path[300] = {0x00};
			sprintf(new_path, "%s/%s", dir_path, entry.name);
			lib_file_oper_helper_delete_all_recursive(new_path);
			fs_unlink(new_path);
		} else {
			char fullpath[300] = {0x00};
			sprintf(fullpath, "%s/%s", dir_path, entry.name);
			LOG_INF("[FILE] %s, %s (size = %zu)", entry.name, fullpath, entry.size);
			fs_unlink(fullpath);
		}
	}

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}

int lib_file_oper_helper_make_dir(const char* dir_path) {
	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	res = fs_mkdir(dir_path);
	if (res == 0) {
//		printk("directory created\n");
	} else if (res == -ENOTSUP) {
		printk("not implemented by underlying file system driver\n");
	} else {
		printk("could not create directory\n");
	}

//err:
	k_sem_give(&m_fs_lock);
	return res;
}

bool lib_file_oper_helper_dir_exist(const char* dir_path) {
	int ret = 0;
	bool dir_exist = false;

	k_sem_take(&m_fs_lock, K_FOREVER);

	struct fs_dir_t zdp;
	fs_dir_t_init(&zdp);
//	memset(&zdp, 0x00, sizeof(struct fs_file_t));

	ret = fs_opendir(&zdp, dir_path);
	if (ret == 0) {
#if PRINTK_INF
		printk("Dir %s exists\n", dir_path);
#endif
		dir_exist = true;
	} else {
		printk("Dir %s does not exist\n", dir_path);
		dir_exist = false;
	}

	fs_closedir(&zdp);

//err:
	k_sem_give(&m_fs_lock);
	return dir_exist;
}

int lib_file_oper_helper_count_files(const char* dir_path, uint8_t option, uint32_t *file_count) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	if ((dir_path == NULL) || (file_count == NULL))	{
		ret = -EINVAL;
		goto err;
	}

	/* TODO Currently we support only 1 directory level */
	if (option != 0) {
		ret = -ENOTSUP;
		goto err;
	}

	uint32_t d_count=0;
	uint32_t f_count=0;

	ret = lib_file_oper_helper_count_dir_files(dir_path, &d_count, &f_count);
	*file_count = f_count;

err:
	k_sem_give(&m_fs_lock);
	return ret;
}

int lib_file_oper_helper_get_file_size(const char* file_name_with_path, uint32_t *file_size) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	if ((file_name_with_path == NULL) || (file_size == NULL)) {
		ret = -EINVAL;
		goto err;
	}

	static struct fs_dirent entry;

	ret = fs_stat(file_name_with_path, &entry);
	if (ret == 0) {
		*file_size = entry.size;
	} else {
		*file_size = 0;
	}

err:
	k_sem_give(&m_fs_lock);
	return ret;
}

int lib_file_oper_helper_file_rename(const char *from, const char *to) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

	if ((from == NULL) || (to == NULL)) {
		ret = -EINVAL;
		goto err;
	}

	ret = fs_rename(from, to);

err:
	k_sem_give(&m_fs_lock);
	return ret;
}
