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
LOG_MODULE_REGISTER(app_devcmd);

#include "app_devcmd/app_devcmd.h"
#include "app_devcmd_packet.h"

#define CMD_VERSION	"version"

static int cmd_acpu(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: acpu");
	LOG_INF("responding:");
	LOG_INF(MSG_PASS);
	shell_print(shell, MSG_PASS);

	return 0;
}

static int cmd_version(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: version");
	LOG_INF("responding:");
	LOG_INF("%s", KERNEL_VERSION_STRING);
//	LOG_INF(MSG_PASS);

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_VERSION),
						CMD_VERSION, DEVCMD_STATUS_OK,
						strlen(KERNEL_VERSION_STRING), KERNEL_VERSION_STRING);

	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	/* Response of the command, to be printed on the shell */
//	shell_print(shell, "%s\n", KERNEL_VERSION_STRING);
//	shell_print(shell, MSG_PASS);

	return 0;
}
/* acpu */
SHELL_CMD_REGISTER(acpu, NULL, "Test command to check connectivity", cmd_acpu);

/* version */
SHELL_CMD_REGISTER(version, NULL, "Get kernel version", cmd_version);


int app_devcmd_init() {
	int ret = 0;

	app_devcmd_packet_init();

	return ret;
}
