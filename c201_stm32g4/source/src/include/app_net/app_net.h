/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 1-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_NET_APP_NET_H_
#define SRC_INCLUDE_APP_NET_APP_NET_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdlib.h>

#define APP_NET_OK	(0)
#define APP_NET_ERR	(1)

#define APP_NET_SSIDLEN_MAX		32
#define APP_NET_PWDLEN_MAX		64

typedef enum {
	APP_NET_WIFI,
	APP_NET_HOTSPOT,
	APP_NET_WIFI_DUMMY,		// used for sending cmds without response

	APP_NET_MAX
} APP_NET_TYPES;

typedef enum {
	APP_NET_CONN_STAT_UNKNOWN=0,
	APP_NET_CONN_STAT_WIFI_STA_CONNECTED,
	APP_NET_CONN_STAT_WIFI_STA_DISCONNECTED,
	APP_NET_CONN_STAT_WIFI_SOFTAP_CONNECTED,
	APP_NET_CONN_STAT_WIFI_SOFTAP_DISCONNECTED,

	APP_NET_CONN_STAT_MAX
} APP_NET_CONN_STATUS;

struct wifi_sta_config {
	char ssid[APP_NET_SSIDLEN_MAX+1];
	char pwd[APP_NET_PWDLEN_MAX+1];
};

struct app_net_wifi_ssid {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	char ssid[APP_NET_SSIDLEN_MAX+1];
};

/* static inline functions */
static inline int ssid_count_get(sys_slist_t *list) {
	int ssid_count=0;
	struct app_net_wifi_ssid *ssid, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, ssid, tmp, node)
	{
		if (ssid) {
			ssid_count++;
		}
	}
	return ssid_count;
}

static inline void ssid_list_remove(sys_slist_t *list) {
	struct app_net_wifi_ssid *ssid, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, ssid, tmp, node)
	{
		if (ssid) {
			sys_slist_remove(list, NULL, &ssid->node);
			free(ssid);
		}
	}
}

/**
 * @brief:	parses a SSID array and makes a single linked list
 * 			the array must be like shown below
 * 			[ssid_len0ssid_name0ssid_len1ssid_name1...]
 *
 * @param	ssid_list[in]		ssid list char array
 *								caller must free the memory of the linked list by calling app_net_ssid_ist_remove()
 * @return	valid pointer to the single linked list = SUCCESS
 * 			NULL = FAILED
 */
sys_slist_t* app_net_parse_ssid_list(char *ssid_list);

/**
 * @brief:	Connect to a network and returns the IP
 * @note:	Currently supports Wi-Fi only
 * @param	ssid[in]	the SSID name
 * @param	pwd[in]		the passphrase for the given SSID
 * @param	ip[out]		IP address once connection is successful
 * @return	0 = SUCCESS
 * 			-ve = FAILED
 */
int app_net_wifi_sta_connect(const char *ssid, const char *pwd, char *ip);

/**
 * @brief:	Tries to connect to available network
 *
 * @note:	Currently supports Wi-Fi only
 *
 * @return	0 = SUCCESS
 * 			-ve = FAILED
 */
int app_net_connect();

/**
 * @brief:	Sends command to network processor to start/stop wifi or hotspot based on parameters
 *
 * @param	wifi_hp[in]			APP_NET_WIFI or APP_NET_HOTSPOT
 * @param	start_stop[in]		1 = start, 0 = stop
 * @param	ip[out]				ip address of the access point's webserver (this will be NULL for APP_NET_WIFI)
 * @param	ssid_list[out]		linked list of struct app_net_wifi_ssid (this will be NULL for APP_NET_HOTSPOT)
 *								caller must free the memory of the linked list by calling app_net_ssid_ist_remove()
 * @return	0 = SUCCESS
 * 			-ve = FAILED
 */
int app_net_wifi_hotspot_start_stop(uint8_t wifi_hp, uint8_t start_stop, char *ip,
		sys_slist_t **ssid_list);

/**
 * @brief:	Get the wifi connectivity status
 * @return	values of APP_NET_CONN_STATUS enum
 */
APP_NET_CONN_STATUS app_net_conn_wifi_status_get();

/**
 * @brief:	Get the softap connectivity status
 * @return	values of APP_NET_CONN_STATUS enum
 */
APP_NET_CONN_STATUS app_net_conn_softap_status_get();

/**
 * @brief:	Initialize the network interfaces
 *
 * @return	0 = SUCCESS
 * 			-ve = FAILED
 */
int app_net_init();


#endif /* SRC_INCLUDE_APP_NET_APP_NET_H_ */
