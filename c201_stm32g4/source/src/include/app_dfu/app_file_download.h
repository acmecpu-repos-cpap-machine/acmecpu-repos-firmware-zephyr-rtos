/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jun-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_DFU_APP_FILE_DOWNLOAD_H_
#define SRC_INCLUDE_APP_DFU_APP_FILE_DOWNLOAD_H_

/**
 * @brief	This function sets the url for a file downloading operation
 * 			It internally sends a command to the network processor using
 * 			m2m_comm protocol to set the url. Following this command there
 * 			must be a file download command.
 *
 * @param url[in] url string
 * @return
 * 	0 		successfully sent the url to the network processor
 * 	-EINVAL	invalid input
 * 	-1		other error
 */
int app_file_download_url_set(char *url);

/**
 * @brief	Start the file download process by sending a download command to the
 * 			network processor. Prior to calling this function app_file_download_url_set()
 * 			must be called. A file is downloaded in chunks and each chunk is saved
 * 			to the file.
 *
 * @param img_op[in] image option, possible values are defined in APP_DFU_FW_TYPE
 * @return
 * 	0		successfully started the file download process
 * 	-ve		failed
 */
int app_fw_file_download(uint32_t img_op);

#endif /* SRC_INCLUDE_APP_DFU_APP_FILE_DOWNLOAD_H_ */
