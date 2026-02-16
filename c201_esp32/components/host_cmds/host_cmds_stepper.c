/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_blower.c
 * Created on: 28-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"

#define TAG	"host_cmds"

int host_cmds_stepper_dir_write(uint8_t dir) {

	if ((dir != STEPPER_DIR_CLOCKWISE) && (dir != STEPPER_DIR_ANTICLOCKWISE))
		return -1;

	char cmd[50] = CMD_STEPPER_DIR_SET;

	char step_dir[10] = {0x00};
	sprintf(step_dir, " %d", dir);

	strcat(cmd, step_dir);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_dir_read() {
	char cmd[20] = CMD_STEPPER_DIR_GET;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_speed_hz_write(uint32_t speed_hz) {
	char cmd[50] = CMD_STEPPER_SPEED_HZ_SET;

	char step_speed[10] = {0x00};
	sprintf(step_speed, " %ld", speed_hz);

	strcat(cmd, step_speed);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_speed_hz_read() {
	char cmd[20] = CMD_STEPPER_SPEED_HZ_GET;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_num_rot_write(uint32_t num_rot) {
	char cmd[50] = CMD_STEPPER_NUM_ROT_SET;

	char rotations[10] = {0x00};
	sprintf(rotations, " %ld", num_rot);

	strcat(cmd, rotations);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_num_rot_read() {
	char cmd[20] = CMD_STEPPER_NUM_ROT_GET;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_pos_rel_write(uint16_t pos_rel) {
	char cmd[50] = CMD_STEPPER_POS_REL_SET;

	char position[10] = {0x00};
	sprintf(position, " %d", pos_rel);

	strcat(cmd, position);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_pos_abs_write(uint16_t pos_abs) {
	char cmd[50] = CMD_STEPPER_POS_ABS_SET;

	char position[10] = {0x00};
	sprintf(position, " %d", pos_abs);

	strcat(cmd, position);
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_pos_cur_read() {
	char cmd[20] = CMD_STEPPER_POS_CUR_GET;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

int host_cmds_stepper_zeroset_write(uint8_t zero_set) {
/*
	if (zero_set == 0) {
		return -1;
	}
*/

	char cmd[50] = CMD_STEPPER_ZERO_SET;

	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
}

