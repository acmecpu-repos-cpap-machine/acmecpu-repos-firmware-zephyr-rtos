/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 17-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_paths.h"
#include "app_net/app_net.h"

static int appnet_status(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int wifi_stat = app_net_conn_wifi_status_get();
	switch(wifi_stat) {
	case APP_NET_CONN_STAT_WIFI_STA_CONNECTED:
		shell_print(shell, "APP_NET_CONN_STAT_WIFI_STA_CONNECTED");
		break;
	case APP_NET_CONN_STAT_WIFI_STA_DISCONNECTED:
		shell_print(shell, "APP_NET_CONN_STAT_WIFI_STA_DISCONNECTED");
		break;
	default:
		shell_print(shell, "APP_NET_CONN_STAT_UNKNOWN");
		break;
	}

	int softap_stat = app_net_conn_softap_status_get();
	switch (softap_stat) {
	case APP_NET_CONN_STAT_WIFI_SOFTAP_CONNECTED:
		shell_print(shell, "APP_NET_CONN_STAT_WIFI_SOFTAP_CONNECTED");
		break;
	case APP_NET_CONN_STAT_WIFI_SOFTAP_DISCONNECTED:
		shell_print(shell, "APP_NET_CONN_STAT_WIFI_SOFTAP_DISCONNECTED");
		break;
	default:
		shell_print(shell, "APP_NET_CONN_STAT_UNKNOWN");
		break;
	}
	return 0;
}

static int appnet_wifi_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	struct setting_value val;
	val.val1 = 1;
	val.val2 = 0;
//	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_LOD, &val, sizeof (struct setting_value), 10, true);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

static int appnet_wifi_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	ret = app_net_wifi_hotspot_start_stop(APP_NET_WIFI, 0, NULL, NULL);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		appnet_status(shell, argc, argv);
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

static int appnet_wifiap_on(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	char ap_ip[20];
	ret = app_net_wifi_hotspot_start_stop(APP_NET_HOTSPOT, 1, ap_ip, NULL);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "hotspot start successful, IP = %s", ap_ip);
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

static int appnet_wifiap_off(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	ret = app_net_wifi_hotspot_start_stop(APP_NET_HOTSPOT, 0, NULL, NULL);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		appnet_status(shell, argc, argv);
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

static int appnet_ifconfig(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	struct setting_value val;
	val.val1 = 1;
	val.val2 = 0;
//	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_LOD, &val, sizeof (struct setting_value), 10, true);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}


/* appnet */
SHELL_STATIC_SUBCMD_SET_CREATE(appnet_subcmds,
		SHELL_CMD(wifi_on, NULL, "Turn on WiFi in station mode. List of SSID will be displayed", appnet_wifi_on),
		SHELL_CMD(wifi_off, NULL, "Turn off WiFi station mode.", appnet_wifi_off),
		SHELL_CMD(wifiap_on, NULL, "Turn on WiFi in SoftAP mode. SSID and IP will be displayed", appnet_wifiap_on),
		SHELL_CMD(wifiap_off, NULL, "Turn off WiFi SoftAP mode.", appnet_wifiap_off),
		SHELL_CMD(status, NULL, "Prints connectivity status", appnet_status),
		SHELL_CMD(ifconfig, NULL, "Displays network configurations", appnet_ifconfig),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(appnet, &appnet_subcmds, "Network commands", NULL);
