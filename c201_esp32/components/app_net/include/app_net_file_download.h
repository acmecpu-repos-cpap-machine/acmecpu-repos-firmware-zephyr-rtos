/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 01-May-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_APP_NET_INCLUDE_APP_NET_FILE_DOWNLOAD_H_
#define COMPONENTS_APP_NET_INCLUDE_APP_NET_FILE_DOWNLOAD_H_

/**
 * @brief	This function copies the url from the input parameter and stores
 * 			it into a dynamically allocated memory location.
 * @param url[in]		a string from where the URL will be copied
 * @param url_len[in]	Length of the URL in bytes
 * @return
 * 		0 		if succeeded
 * 		-EINVAL	invalid input parameters
 * 		-ENOMEM	no memory to store the URL
 */
int app_net_file_download_url_set(const char* url, int url_len);

/**
 * @brief	This function copies the SSL certificate from the input parameter and stores
 * 			it into a dynamically allocated memory location. This certificate will be used
 * 			to connect to an HTTPS server
 * @param cert[in]		a string from where the SSL certificate will be copied
 * @param cert_len[in]	Length of the SSL certificate in bytes
 * @return
 * 		0 		if succeeded
 * 		-EINVAL	invalid input parameters
 * 		-ENOMEM	no memory to store the URL
 */
int app_net_file_download_cert_set(const char* cert, int cert_len);

/**
 * @brief	This function starts a thread which downloads a file using http protocol.
 * 			The file is not saved into the storage but it is streamed to the host processor
 * 			using a FIFO mechanism. URL and Certificate must be set prior to calling this function.
 * @return
 * 		0 		Successfully started the file download thread
 * 		-EINVAL	File download URL was not set. Call app_net_file_download_url_set() before calling this function
 * 		-ve		Failed to start the thread
 */
int app_net_http_file_download_and_stream_start(void);

/**
 * @brief	Initialize the http file download module
 * @return
 * 		0	Success
 * 		-ve Failed
 */
int app_net_http_file_download_init(void);

#endif /* COMPONENTS_APP_NET_INCLUDE_APP_NET_FILE_DOWNLOAD_H_ */
