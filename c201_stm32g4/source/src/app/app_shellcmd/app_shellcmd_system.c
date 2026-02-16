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

static int system_settings_reload(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	struct setting_value val;
	val.val1 = 1;
	val.val2 = 0;
	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_LOD, &val, sizeof (struct setting_value), 10, true);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "system will be rebooted to reload new settings");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

/* system */
SHELL_STATIC_SUBCMD_SET_CREATE(system_subcmds,
		SHELL_CMD(settings_reload, NULL, "A new set of settings are available, this command will reload them and perform a reboot", system_settings_reload),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(system, &system_subcmds, "System level commands", NULL);
