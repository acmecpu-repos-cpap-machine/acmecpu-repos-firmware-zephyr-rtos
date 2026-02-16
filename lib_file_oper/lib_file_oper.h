/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_H_
#define SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_H_

#include <stdio.h>

struct lib_file_oper_rw {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	char *data;
	size_t len;
};

/**
 * @brief 	Puts the data to be written into the write queue of the file
 *
 * @param	file_handle[in]	handle of the file returned from lib_file_oper_create_open_file()
 * @param	wr[in]			valid object of struct lib_file_oper_rw containing the data and length
 *
 * @return
 * 			-EINVAL		If any of the input parameters are NULL or invalid
 * 			0			Success
 */
int lib_file_oper_write(int file_handle, struct lib_file_oper_rw *wr);

/**
 * @brief 	Starts read file operation.
 *
 * @note	This function is non-blocking, it returns a pointer to a fifo which the caller
 * 			must do k_fifo_get to get an object of struct lib_file_oper_rw which contains
 * 			the data and length.
 *
 * @note	Caller must free the data and the struct lib_file_oper_rw object
 *
 * @param	file_handle[in]	handle of the file returned from lib_file_oper_create_open_file()
 *
 * @return
 * 			NULL						if failed
 * 			pointer to struct k_fifo* 	if successful
 */
struct k_fifo* lib_file_oper_read_whole_file(int file_handle);

/**
 * @brief 	Opens a file or creates it if it does not exist.
 * 			Also initializes read and write queues and starts the files thread
 *
 * @param	dir_path[in]		the complete directory path excluding the file name.
 * 								Directory will be created is it does not exist. Only one level of directory is supported.
 * @param	fname[in]			the file name without directory path
 * @param	fname_with_path[in]	the complete directory path including the file name
 * @param	fsize_max[in]		max size (in bytes) of the current file to be reached before it is backed up (log rotation)
 * @param	fnum_max[in]		max number of files to be created before the oldest file can be deleted (log rotation)
 * 								if fnum_max = 1, then log rotation is disabled
 * @param	mode_flags[in]		file open mode of type fs_mode_t, e.g. FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND
 *
 * @return
 * 			-EINVAL		If any of the input parameters are NULL
 * 			-1			If max number of files reached (configured by LIB_FILE_OPER_MAX_FILES)
 * 			0 or more	File handle. This file handle must be passed to subsequent calls
 * 						to the library functions
 */
int lib_file_oper_create_open_file(const char *dir_path, const char *fname, const char *fname_with_path,
		uint32_t fsize_max, uint32_t fnum_max, uint32_t mode_flags);

/**
 * @brief	Close a file
 * 			Calling this function internally stops the file's thread. The file needs to be
 * 			reopened by lib_file_oper_create_open_file() before reading or writing
 * @param file_handle[in]	handle of the file returned from lib_file_oper_create_open_file()
 * @return	-EINVAL		incorrect parameter
 * 			-ve			other errors
 * 			0 			Success
 */
int lib_file_oper_close_file(int file_handle);
/**
 * @brief 	Delete a file or the directory containing it
 *
 * @param	file_handle[in]	handle of the file returned from lib_file_oper_create_open_file()
 * @param	fname[in]		the complete file name including the directory path
 * @param	delete_all_in_dir[in]	if true, deletes all the files present in the directory containing fname
 *
 * @return	-EINVAL		incorrect parameter
 * 			-ve			other errors
 * 			0 			Success
 */
int lib_file_oper_delete_file(int file_handle, const char *fname, bool delete_all_in_dir);

/**
 * @brief 	Basic initialization of the file operation library
 *
 * @return	-1	Init failed
 * 			0 	Success
 */
int lib_file_oper_init();

#endif /* SRC_LIB_LIB_FILE_OPER_LIB_FILE_OPER_H_ */
