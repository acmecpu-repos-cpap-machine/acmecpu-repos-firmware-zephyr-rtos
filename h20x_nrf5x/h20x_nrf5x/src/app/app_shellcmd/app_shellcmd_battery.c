/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 13-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */


// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);
#include "app_shellcmd/app_shellcmd.h"

#include "app_battery/app_battery.h"
#include "app_analog/app_analog.h"

static int charger_status_get(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
    
    uint8_t charging_status=0, source_type=0;
    uint32_t ichg=0, in_curr_lim=0;
    
    ret = app_battery_charging_status_get(&charging_status, &ichg, &source_type, &in_curr_lim);
    
    shell_print(shell, "Charging status = %s, Source type = %d, Charge current = %d, Input current lim = %d", 
                (charging_status ? "CHARGING" : "NOT CHARGING"),
                source_type,
                ichg,
                in_curr_lim);

	return ret;
}

static int charger_current_limits_set(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 3) {
		LOG_ERR("incorrect number of arguments = %d", argc);
		return -EINVAL;
	}
    int ret=0;
	uint32_t chg_cur_lim = strtol(argv[1], NULL, 10);
    uint32_t in_cur_lim = strtol(argv[2], NULL, 10);

    uint8_t charging_status=0, source_type=0;
    uint32_t ichg=0, in_curr=0;
    ret = app_battery_charging_status_get(&charging_status, &ichg, &source_type, &in_curr);

    if ((chg_cur_lim <= 0) || (chg_cur_lim > APP_BATTERY_CHARGE_CURRENT_MAX)) {
        chg_cur_lim = ichg;
    }

    if ((in_cur_lim <= 0) || (in_cur_lim > APP_BATTERY_CHARGE_CURRENT_MAX)) {
        in_cur_lim = in_curr;
    }

    ret = app_battery_current_limits_set(chg_cur_lim, in_cur_lim);
	if (!ret)
		shell_print(shell, "OK, Charge current = %d, Input current lim = %d", chg_cur_lim, in_cur_lim);
	else
		shell_print(shell, "ERR");

	return ret;
}


static int fg_level_get(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: battery level_get");

	/* Get the battery level */
	uint8_t batt_level=0;

	int ret = 0;
#if CONFIG_APP_BATTERY
	ret = app_battery_FG_level_get(&batt_level);
#endif
	shell_print(shell, "%d", batt_level);

	return ret;
}

static int shellcmd_battery_voltage_get(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Get the battery level */
	int32_t vbat_mv=0;

	int ret = 0;
	ret = app_analog_vbat_mv_get(&vbat_mv);

	shell_print(shell, "%d", vbat_mv);

	return ret;
}

static int fg_charge_complete(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
#if CONFIG_APP_BATTERY
	ret = app_battery_FG_charge_complete_set();
#endif
	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}

static int fg_acc_charge_set(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments = %d", argc);
		return -EINVAL;
	}
    int ret=0;
	uint32_t charge_mah = strtol(argv[1], NULL, 10);

    ret = app_battery_FG_accumulated_charge_set(charge_mah);
	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}

/* charger */
SHELL_STATIC_SUBCMD_SET_CREATE(charger_cmds,
		SHELL_CMD(charge_curr_get, NULL, "Get the charge current", charger_status_get),
		SHELL_CMD_ARG(curlim_set, NULL, "usage: battery charger curlim_set <charge current limit> <input current limit>", charger_current_limits_set, 3, 0),
		SHELL_CMD(input_curr_get, NULL, "Get the input current", charger_status_get),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* fuel gauge */
SHELL_STATIC_SUBCMD_SET_CREATE(fg_cmds,
		SHELL_CMD(level, NULL, "Get the battery level", fg_level_get),
		SHELL_CMD(charge_complete, NULL, "Indicate battery is full", fg_charge_complete),
		SHELL_CMD_ARG(charge_set, NULL, "usage: battery fg <available charge in mah>", fg_acc_charge_set, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);

/* battery */
SHELL_STATIC_SUBCMD_SET_CREATE(battery_subcmds,
        SHELL_CMD(voltage_get, NULL, "Get the battery voltage in mv", shellcmd_battery_voltage_get),
        SHELL_CMD(charger, &charger_cmds, "Charger commands", NULL),
		SHELL_CMD(fg, &fg_cmds, "Fuel gauge commands", NULL),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(battery, &battery_subcmds, "Battery commands", NULL);
