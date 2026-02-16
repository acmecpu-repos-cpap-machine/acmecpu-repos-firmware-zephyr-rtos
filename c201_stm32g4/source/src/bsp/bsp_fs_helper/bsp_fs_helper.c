/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <sys/printk.h>
#include <version.h>
#include <stdlib.h>
#include <string.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <sys/slist.h>
#if (CONFIG_FAT_FILESYSTEM_ELM)
	#include <ff.h>
#endif
#if (CONFIG_FILE_SYSTEM_LITTLEFS)
	#include <zephyr/fs/littlefs.h>
#endif


//#include <logging/log.h>

#include "bsp_fs_helper.h"

#define PRINTK_INF	0	// to log or not log the printk info messages

struct k_sem m_fs_lock;

int bsp_fs_log_mgmt_init();

static int bsp_fs_count_dir_files(const char *dir_path, uint32_t *dir_count, uint32_t *file_count) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;
	uint32_t d_count=0;
	uint32_t f_count=0;

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		return -ENXIO;
	}
*/

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

int bsp_fs_init() {
	k_sem_init(&m_fs_lock, 1, 1);
	bsp_fs_log_mgmt_init();
	return 0;
}

int bsp_fs_file_list_get(const char *dir_path, sys_slist_t *file_list) {
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		res = -ENXIO;
		goto err;
	}
*/

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

size_t bsp_fs_create_append_file(const char* file_name, const void *data, size_t len) {
#if PRINTK_INF
	printk("bsp_fs_create_append_file: file_name=%s, length=%d\n", file_name, len);
#endif
	int res=0;
	size_t wrbytes=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		res = -ENXIO;
		goto err;
	}
*/

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
int bsp_fs_read_file(size_t argc, char **argv) {
#if PRINTK_INF
	printk("read_file: argc=%d, argv[1]=%s\n", argc, argv[1]);
#endif
	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		res = -ENXIO;
		goto err;
	}
*/

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

int bsp_fs_delete_file(const char* file_name_with_path) {

	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		res = -ENXIO;
		goto err;
	}
*/

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

int bsp_fs_make_dir(const char* dir_path) {
	int res=0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		res = -ENXIO;
		goto err;
	}
*/

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

bool bsp_fs_dir_exist(const char* dir_path) {
	int ret = 0;
	bool dir_exist = false;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		goto err;
	}
*/

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

int bsp_fs_count_files(const char* dir_path, uint8_t option, uint32_t *file_count) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		ret = -ENXIO;
		goto err;
	}
*/
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

	ret = bsp_fs_count_dir_files(dir_path, &d_count, &f_count);
	*file_count = f_count;

err:
	k_sem_give(&m_fs_lock);
	return ret;
}

int bsp_fs_get_file_size(const char* file_name_with_path, uint32_t *file_size) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		ret = -ENXIO;
		goto err;
	}
*/
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

int bsp_fs_file_rename(const char *from, const char *to) {
	int ret = 0;

	k_sem_take(&m_fs_lock, K_FOREVER);

/*
	if (!bsp_fs_sd_card_is_mounted()) {
		ret = -ENXIO;
		goto err;
	}
*/
	if ((from == NULL) || (to == NULL)) {
		ret = -EINVAL;
		goto err;
	}

	ret = fs_rename(from, to);

err:
	k_sem_give(&m_fs_lock);
	return ret;
}
