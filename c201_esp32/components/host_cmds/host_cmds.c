/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include <sys/errno.h>

#include "host_cmds_uart_config.h"
#include "host_cmds.h"
#include "host_cmds_priv.h"
#include  "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "c20x_m2m_cmds.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"host_cmds"

static struct host_cmd_callback cb_comm_check;		/* uart communication check callback object */
static bool m_comm = false;

static void cb_handler_comm_check(struct host_cmd_callback *cb, uint32_t cmd, void *frame) {
	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	/* extract the command id */
	char *tok = strtok((char*)fr->payload, ",\n");
	if (tok == NULL)	goto err;

	uint16_t cmd_id = atoi(tok);
	if (cmd_id == C20X_M2M_CMD_ID_COMM_CHK) {
		tok = strtok(NULL, ",\n");
		if (tok == NULL)	goto err;

		if (strcmp(tok, M2M_CMD_RESP_OK) == 0) {
			ESP_LOGI(TAG, "Communication check is successful");
			m_comm = true;
		} else {
			ESP_LOGE(TAG, "Communication check failed!");
			m_comm = false;
		}
	}
err:
	free(fr);
}

static int send_data(uart_port_t uart_num, const uint8_t *data, size_t len) {

	const int txBytes = uart_write_bytes(uart_num, data, len);

	ESP_LOGI(TAG, "Wrote %d bytes", txBytes);
	ESP_LOGI(TAG, "Wrote Data = %s", data);

	return txBytes;
}

static int comm_check_pac_make(struct m2m_frame_t *pac) {
	if (pac == NULL)
		return -1;

	pac->sof = UART_M2M_START_OF_FRAME;
	pac->type = UART_M2M_FRAME_SINGLE_REQ;
	pac->sequence = 0;

	uint32_t len=0;
	char payload[20] = {0x00};

	len = sprintf(payload, "%d,?\n", C20X_M2M_CMD_ID_COMM_CHK);

	pac->payload_len = len;
	memcpy(pac->payload, payload, pac->payload_len);

	return 0;
}

int host_cmds_verify() {
	int ret = 0;

	host_cmds_add_callback(&cb_comm_check, cb_handler_comm_check, C20X_M2M_CMD_ID_COMM_CHK);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    /* verify if connectivity with the host processor is OK or not */
    int retry=0;
	while (1) {
#if 0
		char cmd[20] = CMD_ACPU;
		strcat(cmd, CMD_END_CHARS);
		size_t len = strlen(cmd);

		int tx_bytes = send_data(UART_NUM, cmd, len);
#else
		struct m2m_frame_t frame;
		comm_check_pac_make(&frame);
		uint32_t len = 0;
		uint8_t sbuf[UART_M2M_FRAME_SIZE_MAX] = {0x00};
		lib_m2m_frame_serialize(sbuf, sizeof(sbuf), &frame, &len);

		int tx_bytes = send_data(UART_NUM, sbuf, len);
#endif
		if (tx_bytes == len) {
#if 1
			ESP_LOGW(TAG, "Sent %d bytes", tx_bytes);
			ESP_LOG_BUFFER_HEXDUMP(TAG, &sbuf, len, ESP_LOG_WARN);

			/* check max retry count */
			if (retry > 10) {
				ESP_LOGE(TAG, "Host communication verification failed");
				ret = ESP_FAIL;
				break;
			}

			/* wait */
			vTaskDelay(pdMS_TO_TICKS(200));
			/* check if we got a correct response frame */
			if (!m_comm) {
				++retry;
				ESP_LOGE(TAG, "Received incorrect response, retrying ... %d", retry);
				continue;
			} else {
				ESP_LOGI(TAG, "Host communication verified successfully");
				ret = ESP_OK;
				break;
			}
#else
			char resp[100] = { 0x00 }; /* we are expecting response of less than 256 bytes */

			const int rxBytes = uart_read_bytes(UART_NUM, resp, 100, 500 / portTICK_PERIOD_MS);
			if (rxBytes > 0) {
				resp[rxBytes] = 0;
				ESP_LOGI(TAG, "Read %d bytes: '%s'", rxBytes, resp);
				ESP_LOG_BUFFER_HEXDUMP(TAG, resp, rxBytes, ESP_LOG_INFO);

				if (retry > 10) {
					ESP_LOGE(TAG, "Host communication verification failed");
					break;
				}

				char *cmp = strstr(resp, CMD_RESP_OK);
				if (cmp == NULL) {
					++retry;
					ESP_LOGE(TAG, "Received incorrect response, retrying ... %d", retry);
					continue;
				} else {
					ESP_LOGI(TAG, "Host communication verified successfully");
					ret = ESP_OK;
					break;
				}
			}
#endif
		} else {
			ESP_LOGE(TAG, "Failed to send data to host processor, verification failed");
			ret = ESP_FAIL;
		}
	}

	return ret;
}

int host_cmds_init_verify_start() {
	int ret = 0;

	/* initialize the uart interface */
    ret = host_cmds_send_recv_init_for_cmd();
    if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize uart driver");
		return ret;
	} else {
		ESP_LOGI(TAG, "UART init successful");
	}

    /* initialize the uart send recv modude */
	ret = host_cmds_send_recv_init();
    if (ret != 0) {
		ESP_LOGE(TAG, "host_cmds_send_recv_init failed!");
		return ret;
    }


    /* start the uart send recv thread */
    ret = host_cmds_send_recv_start();
    if (ret != 0) {
		ESP_LOGE(TAG, "host_cmds_send_recv_start failed!");
		return ret;
    }

    /* verify communication with host */
//    ret = host_cmds_verify();
//    if (ret != 0) {
//		ESP_LOGE(TAG, "host_cmds_verify failed!");
//		return ret;
//    }

	/*char sbuf[50] = "Hello\r\n";
	send_data(UART_NUM, (uint8_t*)sbuf, strlen(sbuf));
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	send_data(UART_NUM, (uint8_t*)sbuf, strlen(sbuf));
*/
    return ret;
}
