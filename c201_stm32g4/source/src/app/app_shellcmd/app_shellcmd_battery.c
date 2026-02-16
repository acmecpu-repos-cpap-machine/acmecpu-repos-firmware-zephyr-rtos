/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 20-Jan-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

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

#if CONFIG_APP_BATTERY
#include "app_battery/app_battery.h"
#endif

static int shellcmd_battery_level_get(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: battery level_get");

	/* Get the battery level */
	uint8_t batt_level=0;

	int ret = 0;
#if CONFIG_APP_BATTERY
	ret = app_battery_level_get(&batt_level);
#endif
	shell_print(shell, "%d", batt_level);

	return ret;
}

/* battery */
SHELL_STATIC_SUBCMD_SET_CREATE(battery_subcmds,
		SHELL_CMD(level_get, NULL, "Get the battery level", shellcmd_battery_level_get),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(battery, &battery_subcmds, "Battery commands", NULL);

