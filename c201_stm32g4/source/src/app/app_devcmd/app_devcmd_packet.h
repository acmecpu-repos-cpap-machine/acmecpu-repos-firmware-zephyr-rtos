/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_APP_APP_DEVCMD_APP_DEVCMD_PACKET_H_
#define SRC_APP_APP_DEVCMD_APP_DEVCMD_PACKET_H_

#include <stdint.h>

#define DEVCMD_PACKET_SIZE_MAX	512
#define DEVCMD_COMMAND_SIZE_MAX	50
#define DEVCMD_HEADER_SIZE_MAX	14
#define DEVCMD_PAYLOAD_SIZE_MAX	(DEVCMD_PACKET_SIZE_MAX - (DEVCMD_COMMAND_SIZE_MAX + DEVCMD_HEADER_SIZE_MAX))

typedef enum {
	DEVCMD_PACKET_SINGLE,
	DEVCMD_PACKET_STREAM
} DEVCMD_PACKET_TYPE;

typedef enum {
	DEVCMD_STATUS_OK=0,
	DEVCMD_STATUS_FAIL
} DEVCMD_PACKET_STATUS;

struct devcmd_packet_t {
	uint8_t type;
	uint32_t sequence;
	uint32_t cmd_len;
	uint8_t cmd[DEVCMD_COMMAND_SIZE_MAX];
	uint8_t status;
	uint32_t payload_len;
	uint8_t payload[DEVCMD_PAYLOAD_SIZE_MAX];
};

int app_devcmd_serialize_packet(uint8_t *sbuf, uint32_t sbuf_len,
		struct devcmd_packet_t *pac, uint32_t *sdata_len);

int app_devcmd_make_packet(struct devcmd_packet_t *pac, uint8_t type,
		uint32_t sequence, uint32_t cmd_len, uint8_t *cmd, uint8_t status,
		uint32_t payload_len, uint8_t *payload);

int app_devcmd_transmit_data(uint8_t *buf, uint32_t len);

int app_devcmd_packet_init();

#endif /* SRC_APP_APP_DEVCMD_APP_DEVCMD_PACKET_H_ */
