/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 24-Apr-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_CMD_IFACE_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_CMD_IFACE_H_

/**
 * @brief	Calling this function initiates settings file download process by
 * 			sending a stream request to the network processor
 * 			The url and certificate must be set before calling this function
 * 			The downloaded data is received in chunks in a callback internally set
 * 			by this function, and the data is saved in the settings file configured
 * 			via KConfig (see KConfig.settings)
 * @return
 * 	0		Success
 * 	-1		Failed
 */
int app_settings_file_download();

/**
 * @brief	Sends the file download URL to be set to the the network processor
 * 			The file URL is read from a config file or from KConfig
 * @return
 * 0		Success
 * -ve		Failure
 */
int app_settings_file_download_url_set();

/**
 * @brief	Sends the file download URL to be set to the the network processor
 * @param url	the URL to download the file from
 * @return
 * 	0			Success
 * 	-EINVAL		If passed URL is NULL
 */
int app_settings_file_download_url_set_dynamic(char *url);

/**
 * @brief	Sends the file download SSL certificate to be set to the the network
 * 			processor
 * @return
 * 0		Success
 * -ve		Failure
 */
int app_settings_file_download_cert_set();

/**
 * @brief	Delete the downloaded settings file
 * @return
 * 	0		Success
 * 	-ve		Failure
 */
int app_settings_file_delete();


#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_CMD_IFACE_H_ */
