/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_file);

#include "lib_file_oper.h"
#include "lib_file_oper_config.h"
#include "lib_file_oper_helper.h"

#define MAX_FILES	LIB_FILE_OPER_MAX_FILES

static K_THREAD_STACK_ARRAY_DEFINE(tstack, MAX_FILES, LIB_THREAD_STACK_SIZE_FILE_OPER);

typedef enum {
	THREAD_IDLE=0,
	THREAD_RUNNING,
	THREAD_TRY_SUSPEND,
	THREAD_SUSPENDED,
	THREAD_TRY_TERMINATE,
	THREAD_TERMINATED
} FILE_THREAD_MODES;

struct file_oper_data {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t file_handle;		// handle returned to the caller when a new file is opened

	/* thread variables */
	uint8_t stack_idx;			// stack index used to create the file oper thread
	struct k_thread tdata;
	k_tid_t tid;
	uint8_t tsuspend;			// see enum FILE_THREAD_MODES for values

	/* fifo */
	struct k_fifo fifo_rd;
	struct k_fifo fifo_wr;

	/* semaphore */
	struct k_sem lock;

	bool k_tools_init;

	/* fs data */
	struct fs_file_t zfp;
	bool read_file;				// if true read file and put in read fifo
	bool log_rotation;			// if true log rotation is enabled
	uint32_t fsize;				// size of the current file
	struct data_rotation dr;
	uint32_t mode_flags;		// file open modes read | write | append | create etc.
};

/* List of file objects */
static sys_slist_t m_fod_list;

/* Currently Zephyr supports only statically allocated stacks for threads
 * So, in order to reuse a stack we need to maintain a queue of free stack indexes */
static struct k_fifo m_free_stack_idx_fifo;
struct free_stack_data {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	uint8_t stack_index;
};
static struct free_stack_data m_free_idx[MAX_FILES];		// array containing the indexes to be used in the fifo

static int m_file_handle = 0;
static uint16_t m_file_count = 0;	// number of files opened

/* static functions */
static inline int file_oper_list_node_add(sys_slist_t *list, struct file_oper_data *fod_data)
{
	sys_slist_append(list, &fod_data->node);
	return 0;
}
static inline int file_oper_list_node_delete(sys_slist_t *list, uint8_t file_handle)
{
	struct file_oper_data *fdata, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, fdata, tmp, node)
	{
		if (fdata && (fdata->file_handle == file_handle)) {
			sys_slist_remove(list, NULL, &fdata->node);
			free(fdata);
			return 0;
		}
	}
	return -1;
}
static inline void file_oper_list_remove(sys_slist_t *list)
{
	struct file_oper_data *fdata, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, fdata, tmp, node)
	{
		if (fdata) {
			sys_slist_remove(list, NULL, &fdata->node);
			free(fdata);
		}
	}
}

static struct file_oper_data* file_oper_obj_get(int file_handle)
{
	sys_slist_t *list = &m_fod_list;
	struct file_oper_data *fod, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, fod, tmp, node)
	{
		if (fod && (fod->file_handle == file_handle)) {
			return fod;
		}
	}
	return NULL;
}

static void free_thread_stack_index(struct file_oper_data *fod)
{
	/* This should get called when a lib_file_oper_thread is being terminated
	 * So, add the stack index of the calling thread to the free stack index queue
	 * */
	int idx = 0;
	for (idx=0; idx<MAX_FILES; idx++) {
		if (m_free_idx[idx].stack_index == fod->stack_idx)
			break;
	}
	k_fifo_put(&m_free_stack_idx_fifo, &m_free_idx[idx]);
}

static void lib_file_oper_thread(void *p1, void *p2, void *p3)
{
	struct file_oper_data *fod = (struct file_oper_data *)p1;

	struct lib_file_oper_rw *rd = NULL;
	struct lib_file_oper_rw *wr = NULL;
	bool is_writing = false;
	uint8_t read_file_start = 0;
	int ret = 0;
	uint32_t bytes_written = fod->fsize;	// the size of existing or new current file
	fod->tsuspend = THREAD_RUNNING;
	while (1) {
		if (fod->read_file)
			goto read_file;		// don't write when reading

		/* get data from the write fifo */
		wr = k_fifo_get(&fod->fifo_wr, K_MSEC(1));
		if (wr != NULL) {
			is_writing = true;
			if ((fod->log_rotation) && (bytes_written >= fod->dr.curr_max_fsize)) {
				fs_close(&fod->zfp);
				/* check for log rotation */
				int rot = lib_file_oper_helper_perform_data_rotation(&fod->dr, &fod->zfp);
				if (rot == 1) {
					LOG_INF("Log file rotated, creating new file %s", fod->dr.curr_fname_path);
					/* create new current file */
					fs_file_t_init(&fod->zfp);
					ret = fs_open(&fod->zfp, fod->dr.curr_fname_path, fod->mode_flags);
					if (ret != 0) {
						LOG_ERR("file open/create failed!");
						goto free_mem;
					}
					bytes_written = 0;
				}
			}
			int wrbytes = fs_write(&fod->zfp, wr->data, wr->len);
			if (wrbytes != wr->len) {
				LOG_ERR("fs_write failed");
			} else {
				bytes_written += wrbytes;
				LOG_INF("wrote %d bytes", wrbytes);
				LOG_DBG("%s", wr->data);
			}
free_mem:
			/* free memory */
			free(wr->data);
			free(wr);
		}
read_file:
		/* if there is a read request, then read data from the file and put in the read fifo */
		if (fod->read_file) {
			if (!read_file_start) {
				fs_seek(&fod->zfp, 0, FS_SEEK_SET);
				read_file_start = 1;
			}
			int rdbytes;
			if (is_writing) rdbytes = LIB_FILE_OPER_READ_WHILE_WRITE_CHUNK_SIZE;
			else			rdbytes = LIB_FILE_OPER_READ_ONLY_CHUNK_SIZE;

			rd = (struct lib_file_oper_rw *)calloc(1, sizeof(struct lib_file_oper_rw));
			if (rd != NULL) {
				rd->data = (char *)calloc(1, rdbytes);
				if (rd->data == NULL) {
					LOG_ERR("calloc failed at %s", __func__);
					continue;
				}
				rd->len = fs_read(&fod->zfp, rd->data, rdbytes);
				if (rd->len < 0) {
					LOG_ERR("fs_read failed");
					continue;
				} else if (rd->len < rdbytes) {
					/* file has less bytes than requested, indicates end of file */
					fod->read_file = false;
					read_file_start = 0;
				}
				k_fifo_put(&fod->fifo_rd, rd);
			}
		}
		is_writing = false;

		/* check if thread should be terminated */
		if (fod->tsuspend == THREAD_TRY_TERMINATE) {
			/* check and cleanup write fifo */
			if (!k_fifo_is_empty(&fod->fifo_wr)) {
				continue;
			}

			/* check and cleanup read fifo */
			struct lib_file_oper_rw *tmprd = NULL;
			while (!k_fifo_is_empty(&fod->fifo_rd)) {
				tmprd = k_fifo_get(&fod->fifo_rd, K_FOREVER);
				free(tmprd->data);
				free(tmprd);
			}

			/* give semaphores */
			k_sem_give(&fod->lock);

			/* close the file */
			fs_close(&fod->zfp);

			/* TODO do other necessary cleanup  */

			/* terminate the thread and free the stack index */
			fod->tsuspend = THREAD_TERMINATED;
			free_thread_stack_index(fod);
			return;
		}
	}
}

int lib_file_oper_write(int file_handle, struct lib_file_oper_rw *wr)
{
	if ((wr == NULL) || (wr->data == NULL)) {
		return -EINVAL;
	}
	if (file_handle < 1) {
		return -EINVAL;
	}

	/* search and get the correct node */
	struct file_oper_data *fod = file_oper_obj_get(file_handle);
	if (fod == NULL) {
		LOG_ERR("could not get file oper object");
		return -1;
	}

	if ((fod->tsuspend == THREAD_IDLE) || (fod->tsuspend == THREAD_RUNNING)) {
		k_sem_take(&fod->lock, K_FOREVER);
		k_fifo_put(&fod->fifo_wr, wr);
		k_sem_give(&fod->lock);
	}
	return 0;
}

struct k_fifo* lib_file_oper_read_whole_file(int file_handle)
{
	if (file_handle < 1) {
		return NULL;
	}

	/* search and get the correct node */
	struct file_oper_data *fod = file_oper_obj_get(file_handle);
	if (fod == NULL) {
		LOG_ERR("could not get file oper object");
		return NULL;
	}

	k_sem_take(&fod->lock, K_FOREVER);
	fod->read_file = true;
	k_sem_give(&fod->lock);

	return &fod->fifo_rd;
}

int lib_file_oper_create_open_file(const char *dir_path, const char *fname, const char *fname_with_path,
		uint32_t fsize_max, uint32_t fnum_max, uint32_t mode_flags)
{
	int ret = 0;

	if ((dir_path == NULL) || (fname == NULL)) {
		return -EINVAL;
	}
	if (m_file_count >= MAX_FILES) {
		LOG_ERR("max files reached");
		return -1;
	}

	/* allocate memory for file oper */
	struct file_oper_data *fod = (struct file_oper_data *) calloc(1, sizeof(struct file_oper_data));
	if (fod == NULL) {
		LOG_ERR("calloc failed %s", __func__);
		return -ENOMEM;
	}

	/* Check and create the directory if it doesn't exist */
	if (!lib_file_oper_helper_dir_exist(dir_path)) {
		ret = lib_file_oper_helper_make_dir(dir_path);
		if (ret) {
			LOG_ERR("could not create directory");
			return ret;
		}
	}

	/* get and store the file size */
	uint32_t fsize_bytes;
	ret = lib_file_oper_helper_get_file_size(fname_with_path, &fsize_bytes);
	LOG_INF("fname = %s, fsize = %d", fname_with_path, fsize_bytes);
	fod->fsize = fsize_bytes;

	/* open or create the file */
	fs_file_t_init(&fod->zfp);
	ret = fs_open(&fod->zfp, fname_with_path, mode_flags);
	if (ret != 0) {
		LOG_ERR("file open/create failed!");
		free(fod);
		return ret;
	}

	/* set data rotation parameters */
	if (fnum_max > 1)	fod->log_rotation = true;
	fod->dr.curr_fname = fname;
	fod->dr.curr_fname_path = fname_with_path;
	fod->dr.oper_dir = dir_path;
	fod->dr.curr_max_fsize = fsize_max;
	fod->dr.max_fcount = fnum_max;
	fod->mode_flags = mode_flags;

	/* create a new file handle and return the same to the caller */
	fod->file_handle = ++m_file_handle;
	++m_file_count;		// new file created, increment the file count

	/* Initialize the fifos */
	k_fifo_init(&fod->fifo_rd);
	k_fifo_init(&fod->fifo_wr);

	/* initialize the semaphore */
	k_sem_init(&fod->lock, 1, 1);

	/* get a free stack index from the queue */
	struct free_stack_data *stack_data = k_fifo_get(&m_free_stack_idx_fifo, K_FOREVER);
	if (stack_data == NULL) {
		LOG_ERR("no free thread stack available");
		free(fod);
		return -ENOMEM;
	}
	fod->stack_idx = stack_data->stack_index;

	/* Start the thread */
	fod->tid = k_thread_create(&fod->tdata, tstack[fod->stack_idx],
								K_THREAD_STACK_SIZEOF(tstack[fod->stack_idx]),
								lib_file_oper_thread, fod, NULL, NULL, LIB_THREAD_PRIO_FILE_OPER, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	char name[10] = { 0x00 };
	sprintf(name, "%s_%d", LIB_THREAD_NAME_FILE_OPER, m_file_handle);
	k_thread_name_set(fod->tid, name);
#endif
	fod->k_tools_init = true;
	fod->tsuspend = THREAD_IDLE;

	/* add this node to the list */
	file_oper_list_node_add(&m_fod_list, fod);

	return m_file_handle;
}

int lib_file_oper_close_file(int file_handle)
{
	if (file_handle < 1) {
		return -EINVAL;
	}

	/* search and get the correct node */
	struct file_oper_data *fod = file_oper_obj_get(file_handle);
	if (fod == NULL) {
		LOG_ERR("file handle not found, file does not exist");
		return -1;
	}

	int ret = 0;

	/* signal the thread to terminate and wait until terminated
	 * the file gets closed before the thread is terminated
	 * */
	fod->tsuspend = THREAD_TRY_TERMINATE;
	ret = k_thread_join(&fod->tdata, K_FOREVER);
	if (ret != 0) {
		LOG_WRN("could not terminate file oper thread %d, file note closed", ret);
		return -1;
	}

	/* delete the node from the list, this also frees the allocated node data */
	file_oper_list_node_delete(&m_fod_list, file_handle);

	--m_file_count;			/* decrement the file count */

	return ret;
}

int lib_file_oper_delete_file(int file_handle, const char *fname, bool delete_all_in_dir)
{
	if (fname == NULL) {
		return -EINVAL;
	}
	if (file_handle < 1) {
		return -EINVAL;
	}

	/* search and get the correct node */
	struct file_oper_data *fod = file_oper_obj_get(file_handle);
	if (fod == NULL) {
		LOG_ERR("file handle not found, file does not exist");
		return -1;
	}

	int ret = 0;

	/* set the delete path */
	char *delete_path = NULL;
	if (delete_all_in_dir) {
		char *hptr = (char*) fname;
		char *tptr = (char*) fname + strlen(fname);
		while ((tptr-hptr) && (*tptr != '/')) {
			tptr--;
		}
		int dir_path_len = tptr - hptr;
		if (dir_path_len <= 0) {
			LOG_ERR("no directory in path");
			return -EINVAL;
		}
		char *dir_path = calloc(1, dir_path_len+1);
		if (dir_path == NULL) {
			LOG_ERR("calloc failed at %s", __func__);
			return -ENOMEM;
		}
		strncpy(dir_path, fname, dir_path_len);
		LOG_INF("deleting directory %s", dir_path);

		delete_path = dir_path;
//		lib_file_oper_helper_lsdir_recursive(delete_path);
	} else {
		delete_path = (char*) fname;
	}

	/* signal the thread to terminate and wait until terminated */
	fod->tsuspend = THREAD_TRY_TERMINATE;
	ret = k_thread_join(&fod->tdata, K_FOREVER);
	if (ret != 0) {
		LOG_WRN("could not terminate file oper thread %d, not deleting file", ret);
		return -1;
	}
#if 0
	/* thread has been terminated, add the stack index to the queue */
	int idx = 0;
	for (idx=0; idx<MAX_FILES; idx++) {
		if (m_free_idx[idx].stack_index == fod->stack_idx)
			break;
	}
	k_fifo_put(&m_free_stack_idx_fifo, &m_free_idx[idx]);
#endif
	/* delete the node from the list, this also frees the allocated node data */
	file_oper_list_node_delete(&m_fod_list, file_handle);

	/* delete the file(s) */
	if (delete_all_in_dir) {
		ret = lib_file_oper_helper_delete_all_recursive(delete_path);
		ret = fs_unlink(delete_path);
	} else {
		ret = fs_unlink(delete_path);
	}

	if (ret == 0) {
		--m_file_count;			/* decrement the file count */
		LOG_INF("delete successful");
	} else if (ret == -EROFS) {
		LOG_ERR("file system is readonly");
	} else if (ret == -ENOTSUP) {
		LOG_ERR("not implemented by underlying file system driver");
	} else {
		LOG_ERR("could not delete");
	}

	return ret;
}

int lib_file_oper_init()
{
	/* all indexes of the thread stack array are free initially */
	k_fifo_init(&m_free_stack_idx_fifo);
	for (int i=0; i<MAX_FILES; i++) {
		m_free_idx[i].stack_index = i;
		k_fifo_put(&m_free_stack_idx_fifo, &m_free_idx[i]);
	}

	lib_file_oper_helper_init();
	return 0;
}
