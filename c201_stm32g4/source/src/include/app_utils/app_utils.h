/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 20-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UTILS_APP_UTILS_H_
#define SRC_INCLUDE_APP_UTILS_APP_UTILS_H_

typedef enum {
    USB_DATA_CHANNEL_STM32=0,
	USB_DATA_CHANNEL_ESP32,
	USB_DATA_CHANNEL_OTHER,
} USB_DATA_CHANNEL;

typedef enum {
	APP_UTILS_DEVICE_DISABLE = 0,
	APP_UTILS_DEVICE_ENABLE
} APP_UTILS_DEVICE_CONTROL;

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

/**
 * @brief
 *      Connect or disconnect the I2C bus to the UCPD controller via the I2C MUX
 *
 * @param
 * 		en_dis[in]			APP_UTILS_DEVICE_DISABLE = disconnect
 * 							APP_UTILS_DEVICE_ENABLE = connect
 * @return
 * 		0 success
 * 		-1 if failed
 */
int app_utils_ucpd_i2c_mux_control(APP_UTILS_DEVICE_CONTROL en_dis);

#endif /* SRC_INCLUDE_APP_UTILS_APP_UTILS_H_ */
