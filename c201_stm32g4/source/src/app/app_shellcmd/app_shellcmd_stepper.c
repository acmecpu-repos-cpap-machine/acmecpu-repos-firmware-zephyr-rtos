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

#include "app_shellcmd/app_shellcmd.h"
#include "app_settings/app_settings.h"
#include "app_stepper/app_stepper.h"

static int shellcmd_stepper_dir_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper dir_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint8_t stepper_dir = strtol(argv[1], NULL, 10);

	/* set the stepper direction */
	int ret = app_stepper_direction_set(stepper_dir);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_dir_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper dir_get");

	/* get the stepper_dir */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint8_t dir = stepper.dir;

	if (!ret)	shell_print(shell, "%d", dir);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_speed_hz_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper speed_hz_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint32_t speed_hz = strtol(argv[1], NULL, 10);

	/* set the stepper speed in hz */
	int ret = app_stepper_speed_hz_set(speed_hz);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_speed_hz_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper speed_hz_get");

	/* get the spped_hz */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint32_t speed_hz = stepper.step_speed_hz;

	if (!ret)	shell_print(shell, "%d", speed_hz);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_num_rot_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper num_rot_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint32_t num_rot = strtol(argv[1], NULL, 10);

	/* set the stepper number of rotations */
	int ret = app_stepper_num_rot_set(num_rot);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_num_rot_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper num_rot_get");

	/* get the configured number of rotations */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint32_t num_rot = stepper.num_rot;

	if (!ret)	shell_print(shell, "%d", num_rot);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_relpos_go(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper pos_rel_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint16_t pos_rel = strtol(argv[1], NULL, 10);

	/* set the stepper relative position */
	int ret = app_stepper_pos_rel_set(pos_rel);
//	if (!ret)	shell_print(shell, MSG_PASS);
//	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_abspos_go(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper pos_abs_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint16_t pos_abs = strtol(argv[1], NULL, 10);

	/* set the stepper absolute position */
	int ret = app_stepper_pos_abs_set(pos_abs);
//	if (!ret)	shell_print(shell, MSG_PASS);
//	else		shell_print(shell, MSG_FAIL);

	return ret;
}


static int shellcmd_stepper_pos_cur_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper pos_cur_get");

	/* get the pos_cur */
	uint16_t pos_cur;
	int ret = app_stepper_pos_curr_get(&pos_cur);

	if (!ret)	shell_print(shell, "%d", pos_cur);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_stepper_zero_set(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper zero_set");

	/* set the stepper zero position */
	int ret = app_stepper_zero_set();
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

/* stepper */
SHELL_STATIC_SUBCMD_SET_CREATE(stepper_subcmds,
		SHELL_CMD_ARG(dir_set, NULL, "Set stepper direction: 0 clockwise, 1 anti-clockwise", shellcmd_stepper_dir_set, 2, 0),
		SHELL_CMD(dir_get, NULL, "Get stepper direction: 0 clockwise, 1 anti-clockwise", shellcmd_stepper_dir_get),
		SHELL_CMD_ARG(hz_set, NULL, "Set stepper speed in Hz (1 to 10000)", shellcmd_stepper_speed_hz_set, 2, 0),
		SHELL_CMD(hz_get, NULL, "Get stepper speed in Hz", shellcmd_stepper_speed_hz_get),
		SHELL_CMD_ARG(rot_set, NULL, "Number of rotations to make after stepping is started set (>= 0)", shellcmd_stepper_num_rot_set, 2, 0),
		SHELL_CMD(rot_get, NULL, "Get the number of rotations", shellcmd_stepper_num_rot_get),
		SHELL_CMD_ARG(relpos_go, NULL, "Go to relative position (0 - 359)", shellcmd_stepper_relpos_go, 2, 0),
		SHELL_CMD_ARG(abspos_go, NULL, "Go to absolute position (0 - 359)", shellcmd_stepper_abspos_go, 2, 0),
		SHELL_CMD(curpos_get, NULL, "Get stepper's current position (0 - 359)", shellcmd_stepper_pos_cur_get),
		SHELL_CMD(zero_set, NULL, "Set current position as 0", shellcmd_stepper_zero_set),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(stepper, &stepper_subcmds, "Stepper control commands", NULL);

