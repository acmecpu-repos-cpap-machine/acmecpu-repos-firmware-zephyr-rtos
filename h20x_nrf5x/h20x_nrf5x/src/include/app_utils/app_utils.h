/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 20-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_UTILS_APP_UTILS_H_
#define SRC_INCLUDE_APP_UTILS_APP_UTILS_H_

typedef enum {
    USB_DATA_CHANNEL_CHARGER=0,
    USB_DATA_CHANNEL_HOST,
} USB_DATA_CHANNEL;

/**
 * @brief
 * 		Enables all power chips on the board
 * 
 * @return
 * 		0 success
 * 		-1 if failed
 */
int app_utils_power_enable();

/**
 * @brief
 *      Select an USB data channel to be mapped with the USB port by switching the USB mux
 * 
 * @param
 * 		channel[in]			from USB_DATA_CHANNEL enum
 *
 * @return
 * 		0 success
 * 		-1 if failed
 */
int app_utils_usb_channel_select(USB_DATA_CHANNEL channel);

#endif /* SRC_INCLUDE_APP_UTILS_APP_UTILS_H_ */