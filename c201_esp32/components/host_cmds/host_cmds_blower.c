/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_blower.c
 * Created on: 28-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <inttypes.h>

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"

#define TAG	"host_cmds"

int host_cmds_blower_status_write(uint8_t status) {
	char cmd[20] = {0x00};

	if (status == BLOWER_ON) {
		strcpy(cmd, CMD_BLOWER_START);
	} else if (status == BLOWER_OFF) {
		strcpy(cmd, CMD_BLOWER_STOP);
	}

	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_voltage_write(uint32_t milli_volts) {
	char cmd[50] = CMD_BLOWER_SET_VOLTAGE;
	char mvolt[10] = {0x00};
	sprintf(mvolt, " %ld", milli_volts);
	strcat(cmd, mvolt);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_duty_write(uint32_t duty_percent) {
	char cmd[50] = CMD_BLOWER_SET_DUTY;
	char mvolt[10] = {0x00};
	sprintf(mvolt, " %ld", duty_percent);
	strcat(cmd, mvolt);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_status_read() {
	char cmd[20] = CMD_BLOWER_STATUS_GET;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_voltage_read() {
	char cmd[50] = CMD_BLOWER_GET_VOLTS_MV;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_speed_hz_read() {
	char cmd[50] = CMD_BLOWER_GET_SPEED_HZ;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_blower_speed_rpm_read() {
	char cmd[50] = CMD_BLOWER_GET_SPEED_RPM;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}
