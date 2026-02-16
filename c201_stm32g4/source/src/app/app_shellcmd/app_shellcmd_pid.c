/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 11-Oct-2022
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

#include "app_shellcmd/app_shellcmd.h"
#include "app_blower/app_blower.h"

static int shellcmd_pid_sv(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: pid %s %s", (argv[0]), (argv[1]));

//	uint32_t sv = strtol(argv[1], NULL, 10);
	float sv = strtof(argv[1], NULL);

	/* apply the pid setvalue */
#if CONFIG_APP_BLOWER
	app_blower_pid_sv_change(sv);
#endif
	shell_print(shell, MSG_PASS);

	return 0;
}

static int shellcmd_pid_sv_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: pid actconst_get");

//	uint32_t actconst = app_blower_pid_actconst_get();
//	shell_print(shell, "actconst = %d ms", actconst);

	shell_print(shell, "not implemented!");

	return 0;
}

static int shellcmd_pid_actconst(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: pid %s %s", (argv[0]), (argv[1]));

	uint32_t actconst = strtol(argv[1], NULL, 10);

	/* apply the pid actconst */
#if CONFIG_APP_BLOWER
	app_blower_pid_actconst_change(actconst);
#endif
	shell_print(shell, MSG_PASS);

	return 0;
}

static int shellcmd_pid_actconst_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: pid actconst_get");

	uint32_t actconst = 0;
#if CONFIG_APP_BLOWER
	actconst = app_blower_pid_actconst_get();
#endif
	shell_print(shell, "actconst = %d", actconst);

	return 0;
}


/* pid */
SHELL_STATIC_SUBCMD_SET_CREATE(pid_subcmds,
		SHELL_CMD_ARG(sv, NULL, "set value in cmH2O (float)", shellcmd_pid_sv, 2, 0),
		SHELL_CMD(sv_get, NULL, "set value get in cmH2O", shellcmd_pid_sv_get),
		SHELL_CMD_ARG(actconst, NULL, "change actuator constant value", shellcmd_pid_actconst, 2, 0),
		SHELL_CMD(actconst_get, NULL, "actuator constant value get", shellcmd_pid_actconst_get),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(pid, &pid_subcmds, "PID commands", NULL);
