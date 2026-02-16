/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_dfu.c
 * Created on: 23-Jun-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"

#define TAG	"host_cmds"

int host_cmds_dfu_status_write(uint8_t status) {
	char cmd[20] = {0x00};

	if (status == DFU_START) {
		strcpy(cmd, CMD_DFU_START);
	} else if (status == DFU_STOP) {
		strcpy(cmd, CMD_DFU_STOP);
	}

	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = 0;
	/* host controller only accepts start command, stop must be handled by esp32 application */
	if (status == DFU_START) {
		ret = host_cmds_send_only(cmd, len);
	} else if (status == DFU_STOP) {

	}

	return ret;
}

int host_cmds_dfu_status_read() {
	int ret = 0;
	return ret;
}
