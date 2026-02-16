/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shell/shell.h>
#include <version.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(app_devcmd);

#include "app_devcmd/app_devcmd.h"
#include "app_settings/app_settings.h"
#include "app_stepper/app_stepper.h"
#include "app_devcmd_packet.h"

#define CMD_STEPPER_DIR_SET			"stepper dir_set"
#define CMD_STEPPER_DIR_GET			"stepper dir_get"
#define CMD_STEPPER_SPEED_HZ_SET	"stepper speed_hz_set"
#define CMD_STEPPER_SPEED_HZ_GET	"stepper speed_hz_get"
#define CMD_STEPPER_NUM_ROT_SET		"stepper num_rot_set"
#define CMD_STEPPER_NUM_ROT_GET		"stepper num_rot_get"
#define CMD_STEPPER_POS_REL_SET		"stepper pos_rel_set"
#define CMD_STEPPER_POS_ABS_SET		"stepper pos_abs_set"
#define CMD_STEPPER_POS_CUR_GET		"stepper pos_cur_get"
#define CMD_STEPPER_ZERO_SET		"stepper zero_set"

static int cmd_stepper_dir_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper dir_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint8_t stepper_dir = strtol(argv[1], NULL, 10);

	/* set the stepper direction */
	int ret = app_stepper_direction_set(stepper_dir);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_DIR_SET),
						CMD_STEPPER_DIR_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_dir_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper dir_get");

	/* get the stepper_dir */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint8_t dir = stepper.dir;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_DIR_GET),
							CMD_STEPPER_DIR_GET, DEVCMD_STATUS_OK,
							sizeof(dir), (uint8_t*)&dir);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_DIR_GET),
							CMD_STEPPER_DIR_GET, DEVCMD_STATUS_FAIL,
							0, NULL);
	}

	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_speed_hz_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper speed_hz_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint32_t speed_hz = strtol(argv[1], NULL, 10);

	/* set the stepper speed in hz */
	int ret = app_stepper_speed_hz_set(speed_hz);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_SPEED_HZ_SET),
						CMD_STEPPER_SPEED_HZ_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_speed_hz_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper speed_hz_get");

	/* get the spped_hz */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint32_t speed_hz = stepper.step_speed_hz;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_SPEED_HZ_GET),
							CMD_STEPPER_SPEED_HZ_GET, DEVCMD_STATUS_OK,
							sizeof(speed_hz), (uint8_t*)&speed_hz);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_SPEED_HZ_GET),
							CMD_STEPPER_SPEED_HZ_GET, DEVCMD_STATUS_FAIL,
							0, NULL);
	}

	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_num_rot_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper num_rot_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint32_t num_rot = strtol(argv[1], NULL, 10);

	/* set the stepper number of rotations */
	int ret = app_stepper_num_rot_set(num_rot);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_NUM_ROT_SET),
						CMD_STEPPER_NUM_ROT_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_num_rot_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper num_rot_get");

	/* get the configured number of rotations */
	struct stepper_params stepper;
	int ret = app_stepper_params_get(&stepper);
	uint32_t num_rot = stepper.num_rot;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_NUM_ROT_GET),
							CMD_STEPPER_NUM_ROT_GET, DEVCMD_STATUS_OK,
							sizeof(num_rot), (uint8_t*)&num_rot);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_NUM_ROT_GET),
							CMD_STEPPER_NUM_ROT_GET, DEVCMD_STATUS_FAIL,
							0, NULL);
	}

	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_pos_rel_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper pos_rel_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint16_t pos_rel = strtol(argv[1], NULL, 10);

	/* set the stepper relative position */
	int ret = app_stepper_pos_rel_set(pos_rel);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_POS_REL_SET),
						CMD_STEPPER_POS_REL_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_pos_abs_set(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("stepper pos_abs_set: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper %s %s", (argv[0]), (argv[1]));

	uint16_t pos_abs = strtol(argv[1], NULL, 10);

	/* set the stepper absolute position */
	int ret = app_stepper_pos_abs_set(pos_abs);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_POS_ABS_SET),
						CMD_STEPPER_POS_ABS_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}


static int cmd_stepper_pos_cur_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper pos_cur_get");

	/* get the pos_cur */
	uint16_t pos_cur;
	int ret = app_stepper_pos_curr_get(&pos_cur);

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_POS_CUR_GET),
							CMD_STEPPER_POS_CUR_GET, DEVCMD_STATUS_OK,
							sizeof(pos_cur), (uint8_t*)&pos_cur);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_STEPPER_POS_CUR_GET),
							CMD_STEPPER_POS_CUR_GET, DEVCMD_STATUS_FAIL,
							0, NULL);
	}

	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

static int cmd_stepper_zero_set(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: stepper zero_set");

	/* set the stepper zero position */
	int ret = app_stepper_zero_set();
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_STEPPER_ZERO_SET),
						CMD_STEPPER_ZERO_SET, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	return ret;
}

/* stepper */
SHELL_STATIC_SUBCMD_SET_CREATE(stepper_subcmds,
		SHELL_CMD_ARG(dir_set, NULL, "Set stepper direction", cmd_stepper_dir_set, 2, 0),
		SHELL_CMD(dir_get, NULL, "Get stepper direction", cmd_stepper_dir_get),
		SHELL_CMD_ARG(speed_hz_set, NULL, "Set the stepper speed in Hz", cmd_stepper_speed_hz_set, 2, 0),
		SHELL_CMD(speed_hz_get, NULL, "Get the stepper speed in Hz", cmd_stepper_speed_hz_get),
		SHELL_CMD_ARG(num_rot_set, NULL, "Set the number of rotations", cmd_stepper_num_rot_set, 2, 0),
		SHELL_CMD(num_rot_get, NULL, "Get the number of rotations", cmd_stepper_num_rot_get),
		SHELL_CMD_ARG(pos_rel_set, NULL, "Set the relative position", cmd_stepper_pos_rel_set, 2, 0),
		SHELL_CMD_ARG(pos_abs_set, NULL, "Set the absolute position", cmd_stepper_pos_abs_set, 2, 0),
		SHELL_CMD(pos_cur_get, NULL, "Get the stepper's current position", cmd_stepper_pos_cur_get),
		SHELL_CMD(zero_set, NULL, "Set the current position as absolute 0", cmd_stepper_zero_set),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(stepper, &stepper_subcmds, "Stepper control commands", NULL);

