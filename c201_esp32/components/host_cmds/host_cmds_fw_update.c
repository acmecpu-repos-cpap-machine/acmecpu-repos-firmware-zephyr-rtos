/*
 * Copyright (c) 2024 Acme CPU
 *
 * Created on: 19-July-2024
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <sys/errno.h>
#include "esp_event.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "errno.h"

#include "host_cmds.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"
#include "lib_m2m_frame.h"

#include "comm_wifi.h"

#define TAG	"host_cmds"

typedef enum {
	FW_UPDATE_OK,
	FW_UPDATE_ABORT
} FW_UPDATE_ERR;
static struct host_cmd_callback cb_fwapp_get;

/* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
static esp_ota_handle_t update_handle = 0 ;
static const esp_partition_t *update_partition = NULL;
static int m_fw_update_err = FW_UPDATE_OK;
static volatile bool image_header_was_checked = false;
static int binary_file_length = 0;
static const esp_partition_t *configured;
static const esp_partition_t *running;

static void reset_ota_globals()
{
	update_handle = 0;
	update_partition = NULL;
	m_fw_update_err = FW_UPDATE_OK;
	image_header_was_checked = false;
	binary_file_length = 0;
}

static void cb_fwapp_get_handler(struct host_cmd_callback *cb, uint32_t cmd, void *frame)
{
	esp_err_t err;
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (m_fw_update_err != FW_UPDATE_OK) {
		ESP_LOGE(TAG, "FW_UPDATE_ABORT ignoring data!");
		goto err;
	}

	if (cmd == C20X_M2M_CMD_NET_FWAPP_GET) {
		ESP_LOGD(TAG, "cb_fwapp_get_handler: cmd %d", C20X_M2M_CMD_NET_FWAPP_GET);

		if (fr->payload_len <= 0) {
			ESP_LOGE(TAG, "Invalid payload length");
			m_fw_update_err = FW_UPDATE_ABORT;
			goto err;
		}

		char *tok = strtok((char*)fr->payload, ",");		// cmd id
		int preamble_len = strlen(tok) + 1;					// length of cmd_id + comma, e.g. 204,...
		int copy_len = fr->payload_len - preamble_len;		// we need to exclude the preamble and copy the data only
		uint8_t *ota_write_data = fr->payload + preamble_len;
		ESP_LOGD(TAG, "payload_len = %ld, sequence = %ld, preamble_len = %d, copy_len = %d",
				fr->payload_len, fr->sequence, preamble_len, copy_len);
//		binary_file_length += copy_len;
//		ESP_LOGI(TAG, "payload_len = %ld, sequence = %ld, preamble_len = %d, copy_len = %d, binary_file_length = %d",
//				fr->payload_len, fr->sequence, preamble_len, copy_len, binary_file_length);

#if 1
		if (fr->type == UART_M2M_FRAME_DATA_RESP) {

			/**
			 * the below code is taken from esp-idf/examples/system/ota/native_ota_example
			 * esp idf version used - v5.2.1
			 */

			if (image_header_was_checked == false) {
				esp_app_desc_t new_app_info;
				if (copy_len > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
					// check current version with downloading
					memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
					ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

					esp_app_desc_t running_app_info;
					if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
						ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
					}

					const esp_partition_t *last_invalid_app = esp_ota_get_last_invalid_partition();
					esp_app_desc_t invalid_app_info;
					if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
						ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
					}

					// check current version with last invalid partition
					if (last_invalid_app != NULL) {
						if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
							ESP_LOGW(TAG, "New version is the same as invalid version.");
							ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
							ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
							m_fw_update_err = FW_UPDATE_ABORT;
							goto err;
						}
					}

					if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
						ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
						m_fw_update_err = FW_UPDATE_ABORT;
						goto err;
					}

					image_header_was_checked = true;

					err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
					if (err != ESP_OK) {
						ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
						m_fw_update_err = FW_UPDATE_ABORT;
						goto err;
					}
					ESP_LOGI(TAG, "esp_ota_begin succeeded");
				} else {
					ESP_LOGE(TAG, "received size will not fit len");
					m_fw_update_err = FW_UPDATE_ABORT;
					goto err;
				}
			}

            err = esp_ota_write( update_handle, (const void *)ota_write_data, copy_len);
            if (err != ESP_OK) {
                esp_ota_abort(update_handle);
                m_fw_update_err = FW_UPDATE_ABORT;
                ESP_LOGE(TAG, "esp_ota_write failed, abort!");
                goto err;
            }
            binary_file_length += copy_len;
            ESP_LOGI(TAG, "Written image length %d", binary_file_length);
		} else if (fr->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
			tok = strtok(NULL, "\n");	// cmd_stat, we are expecting STREND (end of stream)
			if (tok != NULL) {
				if (strcmp(tok, M2M_CMD_RESP_STREND) == 0) {
				    ESP_LOGI(TAG, "Total Write binary data length: %d", binary_file_length);

				    err = esp_ota_end(update_handle);
				    if (err != ESP_OK) {
				        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
				            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
				        } else {
				            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
				        }
						m_fw_update_err = FW_UPDATE_ABORT;
						goto err;
				    }

				    err = esp_ota_set_boot_partition(update_partition);
				    if (err != ESP_OK) {
				        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
						m_fw_update_err = FW_UPDATE_ABORT;
						goto err;
				    }
				}
			}
		}
#endif
	}
err:
	/* free the frame */
	free(fr);
}

void host_cmds_fw_update_do_update()
{
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

int host_cmds_fw_update_netapp_get()
{
	/* prepare the OTA variables */
	reset_ota_globals();

	configured = esp_ota_get_boot_partition();
	running = esp_ota_get_running_partition();

	if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08"PRIx32", but running from offset 0x%08"PRIx32,
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08"PRIx32")",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%"PRIx32,
             update_partition->subtype, update_partition->address);

    /* make and send C20X_M2M_CMD_NET_FWAPP_GET command to host to fetch the new firmware
     * binary in chunks */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_data_req_make(&frame);

	sprintf((char*) frame.payload, "%d%s",
			C20X_M2M_CMD_NET_FWAPP_GET, M2M_CMD_PAYLOAD_TERM);
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

int host_cmds_fw_update_init()
{
	host_cmds_add_callback(&cb_fwapp_get, cb_fwapp_get_handler, C20X_M2M_CMD_NET_FWAPP_GET);
	return 0;
}
