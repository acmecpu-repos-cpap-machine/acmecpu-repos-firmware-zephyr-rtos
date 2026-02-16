/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 23-Apr-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"

#include "app_settings/app_settings_cmd_iface.h"
#include "app_settings/app_settings_multi.h"
#include "app_dfu/app_file_download.h"
#include "app_dfu/app_dfu.h"
#include "lib_events/lib_events.h"

static struct k_sem *m_sem_dnl;
static struct k_sem *m_sem_pgr;

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_FILE_DOWNLOAD_COMPLETED:
	{
		k_sem_give(m_sem_dnl);
		break;
	}
	case LIB_EVENT_FW_PROGRAM_COMPLETED:
	{
		k_sem_give(m_sem_pgr);
		break;
	}
	default:
		break;
	}
}

static int settings_file_dnl_url_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int ret = app_settings_file_download_url_set_dynamic(argv[1]);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int settings_file_dnl_cert_set(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = app_settings_file_download_cert_set();

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int settings_file_download(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = app_settings_file_download();

	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "File download started");
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int settings_file_delete(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = app_settings_file_delete();

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int update_settings(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	ret = app_settings_multi_save();
	if (!ret) {
		shell_print(shell, "*** all settings have been saved, now reboot to apply them");
		shell_print(shell, MSG_PASS);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int fw_file_dnl_url_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int ret = app_file_download_url_set(argv[1]);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int fw_file_download(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}


	uint32_t img_op = strtol(argv[1], NULL, 10);

	if ((img_op < 0) || (img_op > 2)) {
		shell_print(shell, "Invalid input, options: 0-main, 1-net, 2-blwdrv");
		return -1;
	}

	m_sem_dnl = (struct k_sem *)calloc(1, sizeof(struct k_sem));
	k_sem_init(m_sem_dnl, 0, 1);

	struct lib_events_callback *p_file_dnl_stat = (struct lib_events_callback*)calloc(1, sizeof(struct lib_events_callback));
	lib_events_callback_add(p_file_dnl_stat, app_event_handler, LIB_EVENT_FILE_DOWNLOAD_COMPLETED);

	int ret = app_fw_file_download(img_op);

	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "File download process started");
	}
	else		shell_print(shell, MSG_FAIL);

	k_sem_take(m_sem_dnl, K_FOREVER);	// wait until download has been completed
	shell_print(shell, "File download complete");

	lib_events_callback_remove(p_file_dnl_stat, app_event_handler, LIB_EVENT_FILE_DOWNLOAD_COMPLETED);
	free(m_sem_dnl);
	free(p_file_dnl_stat);

	return ret;
}

static int fw_program(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	uint32_t img_op = strtol(argv[1], NULL, 10);

	if ((img_op < 0) || (img_op > 2)) {
		shell_print(shell, "Invalid input, options: 0-main, 1-net, 2-blwdrv");
		return -1;
	}

	m_sem_pgr = (struct k_sem *)calloc(1, sizeof(struct k_sem));
	k_sem_init(m_sem_pgr, 0, 1);

	struct lib_events_callback *p_file_pgr_stat = (struct lib_events_callback*)calloc(1, sizeof(struct lib_events_callback));
	lib_events_callback_add(p_file_pgr_stat, app_event_handler, LIB_EVENT_FW_PROGRAM_COMPLETED);

	shell_print(shell, "Program of %d image started", img_op);
	int ret = app_dfu_fw_program(img_op);

	if (!ret) {
		shell_print(shell, MSG_PASS);
	}
	else		shell_print(shell, MSG_FAIL);

	if (img_op == 1)
		k_sem_take(m_sem_pgr, K_FOREVER);	// wait until programing of net bin has been completed
	shell_print(shell, "Program of %d image completed", img_op);

	lib_events_callback_remove(p_file_pgr_stat, app_event_handler, LIB_EVENT_FW_PROGRAM_COMPLETED);
	free(m_sem_pgr);
	free(p_file_pgr_stat);

	return ret;
}

static int update_main_firmware(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;

	ret = app_dfu_upgrade_test();

	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "*** Reboot to load new main firmware");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

static int update_net_firmware(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;

	ret = app_dfu_upgrade_net();

	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "*** Reboot to load new net firmware");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

/* update */
SHELL_STATIC_SUBCMD_SET_CREATE(update_subcmds,
		SHELL_CMD_ARG(settings_url_set, NULL, "set the settings file download url", settings_file_dnl_url_set, 2, 0),
		SHELL_CMD(settings_cert_set, NULL, "set the settings file download SSL certificate", settings_file_dnl_cert_set),
		SHELL_CMD(settings_file_download, NULL, "download the settings csv file", settings_file_download),
		SHELL_CMD(settings_file_delete, NULL, "Delete the settings csv file", settings_file_delete),
		SHELL_CMD(settings, NULL, "update the settings from the downloaded settings csv file", update_settings),
		SHELL_CMD_ARG(fw_download_url_set, NULL, "set the firmware download file url", fw_file_dnl_url_set, 2, 0),
		SHELL_CMD_ARG(fw_download, NULL, "download firmware image file (options: 0-main, 1-net, 2-blwdrv)", fw_file_download, 2, 0),
		SHELL_CMD_ARG(fw_program, NULL, "write the firmware image file to memory (options: 0-main, 1-net, 2-blwdrv)", fw_program, 2, 0),
		SHELL_CMD(fw_main, NULL, "upgrade the main firmware image", update_main_firmware),
		SHELL_CMD(fw_net, NULL, "upgrade the network processor's firmware image", update_net_firmware),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(update, &update_subcmds, "Commands to do settings update, firmware update etc.", NULL);
