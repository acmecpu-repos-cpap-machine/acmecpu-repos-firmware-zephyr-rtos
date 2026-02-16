/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 04-May-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_WIFI_BT_APP_WIFI_BT_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_WIFI_BT_APP_WIFI_BT_H_

typedef enum {
	APP_WIFI_BT_COM_STATE_IDLE=0,
	APP_WIFI_BT_COM_STATE_BUSY,

	APP_WIFI_BT_COM_STATE_MAX
} APP_WIFI_BT_COM_STATE;

int app_wifi_bt_com_state_get();
int app_wifi_bt_init();

#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_WIFI_BT_APP_WIFI_BT_H_ */
