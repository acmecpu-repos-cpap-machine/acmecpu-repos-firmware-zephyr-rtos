/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_packet.h
 * Created on: 28-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_PACKET_H_
#define COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define HOST_CMD_PACKET_SIZE_MAX	512
#define HOST_CMD_COMMAND_SIZE_MAX	50
#define HOST_CMD_HEADER_SIZE_MAX	16
#define HOST_CMD_PAYLOAD_SIZE_MAX	(HOST_CMD_PACKET_SIZE_MAX - (HOST_CMD_COMMAND_SIZE_MAX + HOST_CMD_HEADER_SIZE_MAX))

typedef enum {
	HOST_CMD_PACKET_SINGLE,
	HOST_CMD_PACKET_STREAM,
	HOST_CMD_PACKET_MAX
} HOST_CMD_PACKET_TYPE;

typedef enum {
	HOST_CMD_STATUS_OK=0,
	HOST_CMD_STATUS_FAIL
} HOST_CMD_PACKET_STATUS;


struct host_cmd_packet_t {
	uint8_t dummy[2];
	uint8_t type;
	uint32_t sequence;
	uint32_t cmd_len;
	uint8_t cmd[HOST_CMD_COMMAND_SIZE_MAX];
	uint8_t status;
	uint32_t payload_len;
	uint8_t payload[HOST_CMD_PAYLOAD_SIZE_MAX];
};

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_INCLUDE_HOST_CMDS_PACKET_H_ */
