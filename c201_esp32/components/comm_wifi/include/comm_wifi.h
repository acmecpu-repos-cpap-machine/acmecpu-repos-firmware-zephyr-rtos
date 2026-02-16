/*
 * Copyright (c) 2022 Acme CPU
 *
 * comm_wifi.h
 * Created on: 03-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_WIFI_INCLUDE_COMM_WIFI_H_
#define COMPONENTS_COMM_WIFI_INCLUDE_COMM_WIFI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"

#define DEFAULT_SCAN_LIST_SIZE 10

typedef enum {
	COMM_WIFI_STA_CONNECTED=0,
	COMM_WIFI_STA_DISCONNECTED
} COMM_WIFI_STAT_CONNECTION_STAT;

uint16_t comm_wifi_scan(wifi_ap_record_t *ap_info);
int comm_wifi_mode_get();

void comm_wifi_softap_ip_get(char *softap_ip, int *len);
int comm_wifi_softap_and_sta_init(void);
int comm_wifi_softap_deinit(void);

int comm_wifi_mac_get(char *mac, int if_type, bool pretty);
int comm_wifi_connection_status_get();
int comm_wifi_sta_ip_get(char *sta_ip, int *len);
int comm_wifi_sta_init(void);
int comm_wifi_sta_connect(char *ssid, char *password);
int comm_wifi_sta_disconnect();

int comm_wifi_init();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_WIFI_INCLUDE_COMM_WIFI_H_ */
