/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_devinf.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <c20x_m2m_cmds.h>
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include <sys/errno.h>

#include "host_cmds.h"
#include "host_cmds_priv.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"


#define TAG	"host_cmds"

int host_cmds_devinf_mfr_name_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	/* TODO get from Kconfig */
	strcpy(read_buf, "AcmeCPU");

	return ret;
}

int host_cmds_devinf_model_num_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	/* TODO get from Kconfig */
	strcpy(read_buf, "C201");

	return ret;
}

int host_cmds_devinf_serial_num_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	/* TODO get from Kconfig */
	strcpy(read_buf, "AC001");

	return ret;
}
#if 0
int host_cmds_devinf_fw_version_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	char cmd[20] = CMD_VERSION;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	/* We take additional memory for the underlying host driver to allocate,
	 * because we will receive addition data like delimiter and status
	 * */
	buf_size += 20;
	ret = host_cmds_send_with_response(cmd, len, read_buf, buf_size);

	return ret;
}
#else
int host_cmds_devinf_fw_version_read(char *read_buf, size_t buf_size) {
#if 0
	char cmd[20] = CMD_VERSION;
	strcat(cmd, CMD_END_CHARS);
	size_t len = strlen(cmd);

	int ret = host_cmds_send_only(cmd, len);
	return ret;
#endif

	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);
	sprintf((char*) frame.payload, "%d%s%s%s", C20X_M2M_CMD_DEVINFO_FWVER, M2M_CMD_PAYLOAD_DELIM,
			M2M_CMD_PAYLOAD_GET_CHAR, M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((const char *)frame.payload);
#if 0
	/* serialize the frame and send response */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		ESP_LOGE(TAG, "%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
#endif
	/* compute checksum */
	int ret = lib_m2m_frame_checksum_compute(&frame);

	size_t sdata_len=0;
	uint8_t *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ret = host_cmds_send_only((const char *)serialized_buffer, sdata_len);
	ESP_LOG_BUFFER_HEXDUMP(TAG, serialized_buffer, sdata_len, ESP_LOG_WARN);
err:
	free(serialized_buffer);
	return ret;
}
#endif

int host_cmds_devinf_hw_version_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	/* TODO get from Kconfig */
	strcpy(read_buf, "HW001");

	return ret;
}

int host_cmds_devinf_sw_version_read(char *read_buf, size_t buf_size) {
	int ret = 0;

	strncpy(read_buf, esp_get_idf_version(), (buf_size-1));

	return ret;
}
