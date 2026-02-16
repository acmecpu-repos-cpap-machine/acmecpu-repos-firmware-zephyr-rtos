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
#include "app_battery/app_battery.h"
#include "app_devcmd_packet.h"

#define CMD_BATTERY_LEVEL_GET	"battery level_get"

static int cmd_battery_level_get(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: battery level_get");

	/* Get the battery level */
	uint8_t batt_level;
	int ret = app_battery_level_get(&batt_level);

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	if (!ret) {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BATTERY_LEVEL_GET),
							CMD_BATTERY_LEVEL_GET, DEVCMD_STATUS_OK,
							sizeof(batt_level), &batt_level);
	} else {
		app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
							strlen(CMD_BATTERY_LEVEL_GET),
							CMD_BATTERY_LEVEL_GET, DEVCMD_STATUS_FAIL,
							0, NULL);
	}
/*
	resp->type = DEVCMD_PACKET_SINGLE;
	resp->sequence = 1;
	resp->cmd_len = strlen(CMD_BATTERY_LEVEL_GET);
	strncpy(resp->cmd, CMD_BATTERY_LEVEL_GET, resp->cmd_len);
	resp->payload_len = sizeof(batt_level);
	memcpy(resp->payload, &batt_level, resp->payload_len);
*/
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

/*
	LOG_INF("responding:");
	if (!ret) {
		LOG_INF("%s", (serialized_buffer));
		ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);
	} else {
		LOG_INF(MSG_FAIL);
		shell_print(shell, MSG_FAIL);
	}
*/

	free(resp);
	free(serialized_buffer);

	return ret;
}

/* battery */
SHELL_STATIC_SUBCMD_SET_CREATE(battery_subcmds,
		SHELL_CMD(level_get, NULL, "Get the battery level", cmd_battery_level_get),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(battery, &battery_subcmds, "Battery commands", NULL);

