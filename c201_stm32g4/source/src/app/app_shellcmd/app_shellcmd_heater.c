/*
 * app_shellcmd_heater.c
 *
 *  Created on: 01-Mar-2024
 *      Author: Shubham Kesahari (shubhamk@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <version.h>
#if CONFIG_APP_HAS_HEATER
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#include "app_heater/app_heater.h"

#define UNSET -999

static int shellcmd_heater_start(const struct shell *shell, size_t argc,
		char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	char heater_fnc[2];
	char heater_tube_fnc[] = "12";
	char heater_water_fnc[] = "24";
	int ret = 0;

	if (argc == 2) {
		int i = 0;
		if (strlen(argv[1]) <= 2) {
			for (i = 0; argv[1][i] != '\0'; i++) {
				heater_fnc[i] = argv[1][i];
			}
			heater_fnc[i] = '\0';
			/*shell_print(shell, "%s", heater_fnc);*/
		} else {
			shell_print(shell, "Invalid request");
			ret = -1;
			return ret;
		}
	}

	if (strcmp(heater_water_fnc, heater_fnc) == 0) {
		int ret = app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_DEFAULT, UNSET,
				UNSET, UNSET, UNSET, UNSET);
		if (!ret) {
			shell_print(shell, MSG_PASS);
			shell_print(shell, "HEATER_MODULE_24W is ON");
		} else {
			shell_print(shell, MSG_FAIL);
			ret = -1;
			return ret;
		}
	} else if (strcmp(heater_tube_fnc, heater_fnc) == 0) {
		int ret = app_heater_control_thread_en(APP_HEATER_MODULE_12, HEATER_CONFIG_DEFAULT, UNSET,
				UNSET, UNSET, UNSET, UNSET);
		if (!ret) {
			shell_print(shell, MSG_PASS);
			shell_print(shell, "HEATER_MODULE_12W is ON");
		} else {
			shell_print(shell, MSG_FAIL);
			ret = -1;
			return ret;
		}
	} else
		shell_print(shell, "ERR");

	return ret;
}

static int shellcmd_heater_stop(const struct shell *shell, size_t argc,
		char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	char heater_fnc[2];
	char heater_tube_fnc[] = "12";
	char heater_water_fnc[] = "24";
	int ret = 0;

	if (argc == 2) {
		int i = 0;
		if (strlen(argv[1]) <= 2) {
			for (i = 0; argv[1][i] != '\0'; i++) {
				heater_fnc[i] = argv[1][i];
			}
			heater_fnc[i] = '\0';
			/*shell_print(shell, "%s", heater_fnc);*/
		} else {
			shell_print(shell, "Invalid request");
			ret = -1;
			return ret;
		}
	}

	if (strcmp(heater_water_fnc, heater_fnc) == 0) {
		int ret = app_heater_control_thread_dis(APP_HEATER_MODULE_24);
		if (!ret) {
			shell_print(shell, MSG_PASS);
			shell_print(shell, "HEATER_MODULE_24W is OFF");
		} else {
			shell_print(shell, MSG_FAIL);
			ret = -1;
			return ret;
		}
	} else if (strcmp(heater_tube_fnc, heater_fnc) == 0) {
		int ret = app_heater_control_thread_dis(APP_HEATER_MODULE_12);
		if (!ret) {
			shell_print(shell, MSG_PASS);
			shell_print(shell, "HEATER_MODULE_12W is Off");
		} else {
			shell_print(shell, MSG_FAIL);
			ret = -1;
			return ret;
		}
	} else
		shell_print(shell, "ERR");

	return ret;
}

static int shellcmd_24W_heater_temp_set(const struct shell *shell, size_t argc,
		char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	float temp_c = strtof(argv[1], 0);
	shell_print(shell, "Temp = %0.1f C", (double)temp_c);

	int ret = app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_UPDATE, temp_c,
			UNSET, UNSET, UNSET, UNSET);
	if (!ret)
		shell_print(shell, MSG_PASS);
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

static int shellcmd_24W_heater_humid_set(const struct shell *shell, size_t argc,
		char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	float humid_per = strtof(argv[1], NULL);
	shell_print(shell, "Humid = %0.1f Percent", (double)humid_per);

	int ret = app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_UPDATE, UNSET, UNSET,
			humid_per, UNSET, UNSET);
	if (!ret)
		shell_print(shell, MSG_PASS);
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

static int shellcmd_24W_heater_interval_set(const struct shell *shell,
		size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int interval_ms = strtol(argv[1], NULL, 10);
	shell_print(shell, "Interval = %d ms", interval_ms);

	int ret = app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_UPDATE, UNSET, UNSET,
			UNSET, interval_ms, UNSET);
	if (!ret)
		shell_print(shell, MSG_PASS);
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

static int shellcmd_12W_heater_temp_set(const struct shell *shell, size_t argc,
		char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	float temp_c = strtof(argv[1], NULL);
	shell_print(shell, "Temp = %0.1f C", (double)temp_c);

	int ret = app_heater_control_thread_en(APP_HEATER_MODULE_12, HEATER_CONFIG_UPDATE, UNSET,
			temp_c, UNSET, UNSET, UNSET);
	if (!ret)
		shell_print(shell, MSG_PASS);
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

static int shellcmd_12W_heater_interval_set(const struct shell *shell,
		size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int interval_ms = strtol(argv[1], NULL, 10);
	shell_print(shell, "Interval = %d ms", interval_ms);

	int ret = app_heater_control_thread_en(APP_HEATER_MODULE_12, HEATER_CONFIG_UPDATE, UNSET, UNSET,
			UNSET, UNSET, interval_ms);
	if (!ret)
		shell_print(shell, MSG_PASS);
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(heater_subcmds,
		SHELL_CMD_ARG(on, NULL, "heater on", shellcmd_heater_start, 2, 0),
		SHELL_CMD_ARG(off, NULL, "heater off", shellcmd_heater_stop, 2, 0),
		SHELL_CMD_ARG(temp_24W_set, NULL, "heater_24W temp set", shellcmd_24W_heater_temp_set, 2, 0),
		SHELL_CMD_ARG(humid_24W_set, NULL, "heater_24W humid set", shellcmd_24W_heater_humid_set, 2, 0),
		SHELL_CMD_ARG(interval_24W_set, NULL, "heater_24W interval set", shellcmd_24W_heater_interval_set, 2, 0),
		SHELL_CMD_ARG(temp_12W_set, NULL, "heater_12W temp set", shellcmd_12W_heater_temp_set, 2, 0),
		SHELL_CMD_ARG(interval_12W_set, NULL, "heater_12W interval set", shellcmd_12W_heater_interval_set, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
		);
SHELL_CMD_REGISTER(heater, &heater_subcmds, "Heater control commands", NULL);
#endif
