/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/uart.h>

#include "acpu_c201_modules.h"
#include "app_devcmd_packet.h"

#define UART_DEV_LABEL	ACPU_C201_MOD_NAME_NWPROC_COM_PORT
/* static variables */
static struct k_mutex m_lock;

int app_devcmd_serialize_packet(uint8_t *sbuf, uint32_t sbuf_len, struct devcmd_packet_t *pac, uint32_t *sdata_len) {
	int ret = 0;
	int buf_ptr = 0;

	memset(sbuf, 0, sbuf_len);

	memcpy(sbuf, &pac->type, sizeof(pac->type));
	buf_ptr += sizeof(pac->type);

	memcpy(sbuf + buf_ptr, &pac->sequence, sizeof(pac->sequence));
	buf_ptr += sizeof(pac->sequence);

	memcpy(sbuf + buf_ptr, &pac->cmd_len, sizeof(pac->cmd_len));
	buf_ptr += sizeof(pac->cmd_len);

	memcpy(sbuf + buf_ptr, pac->cmd, pac->cmd_len);
	buf_ptr += pac->cmd_len;

	memcpy(sbuf + buf_ptr, &pac->status, sizeof(pac->status));
	buf_ptr += sizeof(pac->status);

	memcpy(sbuf + buf_ptr, &pac->payload_len, sizeof(pac->payload_len));
	buf_ptr += sizeof(pac->payload_len);

	if (pac->status == DEVCMD_STATUS_OK) {
		memcpy(sbuf + buf_ptr, pac->payload, pac->payload_len);
		buf_ptr += pac->payload_len;
	}

	/* copy the serialized buffer length */
	*sdata_len = buf_ptr;

	return ret;
}

int app_devcmd_make_packet(struct devcmd_packet_t *pac, uint8_t type,
		uint32_t sequence, uint32_t cmd_len, uint8_t *cmd, uint8_t status,
		uint32_t payload_len, uint8_t *payload) {

	if (pac == NULL) {
		return -EINVAL;
	}

	pac->type = type;
	pac->sequence = sequence;

	pac->cmd_len = cmd_len;
	if ((cmd_len > 0) && (cmd != NULL))
		memcpy(pac->cmd, cmd, cmd_len);

	pac->status = status;

	pac->payload_len = payload_len;
	if ((payload_len > 0) && (payload != NULL))
		memcpy(pac->payload, payload, payload_len);

	return 0;
}

int app_devcmd_transmit_data(uint8_t *buf, uint32_t len) {
	if ((buf == NULL) || (len == 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&m_lock, K_FOREVER);

	const struct device *uart_dev = device_get_binding(UART_DEV_LABEL);
	if (!uart_dev) {
//		LOG_ERR("Cannot get UART device\n");
		return -1;
	}
	/* Verify uart_poll_out() */
	for (int i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}

	k_mutex_unlock(&m_lock);

	return 0;
}

int app_devcmd_packet_init() {
	k_mutex_init(&m_lock);
	return 0;
}
