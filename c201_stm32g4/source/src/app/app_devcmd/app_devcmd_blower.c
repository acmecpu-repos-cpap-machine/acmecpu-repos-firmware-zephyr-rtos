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
#include "app_blower/app_blower.h"
#include "app_devcmd_packet.h"

#define CMD_BLOWER_START			"blower start"
#define CMD_BLOWER_STOP				"blower stop"
#define CMD_BLOWER_STATUS_GET		"blower status_get"
#define CMD_BLOWER_GET_VOLTS_MV		"blower get_volts_mv"
#define CMD_BLOWER_GET_SPEED_HZ		"blower get_speed_hz"
#define CMD_BLOWER_GET_SPEED_RPM 	"blower get_speed_rpm"
#define CMD_BLOWER_SET_VOLTAGE		"blower set_voltage"
#define CMD_BLOWER_SET_DUTY			"blower set_duty"

static int cmd_blower_start(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower start");

	/* Start the blower */
	int ret = app_blower_settings_change_state(APP_BLOWER_START);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_BLOWER_START),
						CMD_BLOWER_START, status,
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

static int cmd_blower_stop(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower stop");

	/* Stop the blower */
	int ret = app_blower_settings_change_state(APP_BLOWER_STOP);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_BLOWER_STOP),
						CMD_BLOWER_STOP, status,
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

static int cmd_blower_set_voltage(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("blower set_voltage: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower %s %s", (argv[0]), (argv[1]));

	uint32_t voltage_mv = strtol(argv[1], NULL, 10);

	/* set the blower voltage */
	int ret = app_blower_settings_change_voltage_mv(voltage_mv);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_BLOWER_SET_VOLTAGE),
						CMD_BLOWER_SET_VOLTAGE, status,
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

static int cmd_blower_set_duty(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("blower set_duty: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower %s %s", (argv[0]), (argv[1]));

	uint8_t duty_percent = strtol(argv[1], NULL, 10);

	/* set the blower voltage */
	int ret = app_blower_settings_change_duty_percent(duty_percent);
	uint8_t status = DEVCMD_STATUS_FAIL;
	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_BLOWER_SET_DUTY),
						CMD_BLOWER_SET_DUTY, status,
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

static int cmd_blower_get_volts_mv(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower get_volts_mv");

	/* set the blower voltage */
	struct app_blower_params blower;
	int ret = app_blower_params_get(&blower);
	uint32_t volts_mv = blower.acq_volt_mv;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_VOLTS_MV),
							CMD_BLOWER_GET_VOLTS_MV, DEVCMD_STATUS_OK,
							sizeof(volts_mv), (uint8_t*)&volts_mv);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_VOLTS_MV),
							CMD_BLOWER_GET_VOLTS_MV, DEVCMD_STATUS_FAIL,
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

static int cmd_blower_get_speed_hz(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower get_speed_hz");

	/* set the blower voltage */
	struct app_blower_params blower;
	int ret = app_blower_params_get(&blower);
	uint32_t speed_hz = blower.acq_speed_hz;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_SPEED_HZ),
							CMD_BLOWER_GET_SPEED_HZ, DEVCMD_STATUS_OK,
							sizeof(speed_hz), (uint8_t*)&speed_hz);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_SPEED_HZ),
							CMD_BLOWER_GET_SPEED_HZ, DEVCMD_STATUS_FAIL,
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

static int cmd_blower_get_speed_rpm(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower get_speed_rpm");

	/* set the blower voltage */
	struct app_blower_params blower;
	int ret = app_blower_params_get(&blower);
	uint32_t speed_rpm = blower.acq_speed_rpm;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_SPEED_RPM),
							CMD_BLOWER_GET_SPEED_RPM, DEVCMD_STATUS_OK,
							sizeof(speed_rpm), (uint8_t*)&speed_rpm);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_GET_SPEED_RPM),
							CMD_BLOWER_GET_SPEED_RPM, DEVCMD_STATUS_FAIL,
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

static int cmd_blower_status_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: blower status_get");

	/* set the blower voltage */
	uint8_t blower_state=0;
	int ret = app_blower_state_get(&blower_state);

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_STATUS_GET),
							CMD_BLOWER_STATUS_GET, DEVCMD_STATUS_OK,
							sizeof(blower_state), &blower_state);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BLOWER_STATUS_GET),
							CMD_BLOWER_STATUS_GET, DEVCMD_STATUS_FAIL,
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

/* blower */
SHELL_STATIC_SUBCMD_SET_CREATE(blower_subcmds,
		SHELL_CMD(start, NULL, "Start the blower", cmd_blower_start),
		SHELL_CMD(stop, NULL, "Stop the blower", cmd_blower_stop),
		SHELL_CMD_ARG(set_voltage, NULL, "Set the blower operating voltage in mV", cmd_blower_set_voltage, 2, 0),
		SHELL_CMD_ARG(set_duty, NULL, "Set the blower duty cycle in percentage", cmd_blower_set_duty, 2, 0),
		SHELL_CMD(get_volts_mv, NULL, "Get the measured blower operating voltage mV", cmd_blower_get_volts_mv),
		SHELL_CMD(get_speed_hz, NULL, "Get the blower speed in Hz", cmd_blower_get_speed_hz),
		SHELL_CMD(get_speed_rpm, NULL, "Get the blower speed in RPM", cmd_blower_get_speed_rpm),
		SHELL_CMD(status_get, NULL, "Get the blower running status", cmd_blower_status_get),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(blower, &blower_subcmds, "Blower control commands", NULL);

