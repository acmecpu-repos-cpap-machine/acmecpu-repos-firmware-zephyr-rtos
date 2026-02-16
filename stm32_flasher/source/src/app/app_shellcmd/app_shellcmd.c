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
LOG_MODULE_REGISTER(app_shellcmd);

#if (CONFIG_APP_STORAGE)
#include "app_storage/app_storage.h"
#endif


static int shellcmd_version(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: version");
	LOG_INF("responding:");
	LOG_INF("%s", KERNEL_VERSION_STRING);

	/* Response of the command, to be printed on the shell */
	shell_print(shell, KERNEL_VERSION_STRING);

	return 0;
}

static int shellcmd_check_storage(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	app_storage_list_root_dir();
	return 0;
}

/* version */
SHELL_CMD_REGISTER(version, NULL, "Get kernel version", shellcmd_version);
SHELL_CMD_REGISTER(check_storage, NULL, "Get kernel version", shellcmd_check_storage);

int app_shellcmd_init() {
	int ret = 0;

	return ret;
}
