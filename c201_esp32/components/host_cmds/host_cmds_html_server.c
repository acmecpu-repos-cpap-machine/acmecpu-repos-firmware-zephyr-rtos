/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_html_server.c
 * Created on: 27-Mar-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include <sys/errno.h>

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"host_cmds"

int host_cmds_html_server_page_get(const char *path)
{
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_data_req_make(&frame);

	sprintf((char*) frame.payload, "%d%s%s%s",
			C20X_M2M_CMD_NET_WS_HTML_PAGE_GET, M2M_CMD_PAYLOAD_DELIM,
			path,M2M_CMD_PAYLOAD_TERM);
	frame.payload_len = strlen((const char *)frame.payload);

	/* compute checksum */
	int ret = lib_m2m_frame_checksum_compute(&frame);

	size_t sdata_len=0;
	uint8_t *serialized_buffer = lib_m2m_frame_alloc_serialize(&frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	host_cmds_send_only((const char *)serialized_buffer, sdata_len);

err:
	free(serialized_buffer);
	return ret;
}
