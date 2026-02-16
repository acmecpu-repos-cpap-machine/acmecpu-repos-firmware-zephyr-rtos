/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 01-May-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_http_client.h"

#include "host_cmds.h"
#include "c20x_m2m_cmds.h"
//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"file_dnl"

#define FILE_DNL_STR_TASK_STACK		(1024*4)
#define FILE_DNL_STR_TASK_PRIO		(10)
#define FILE_DNL_BUFF_SIZE			(1024*2)

static TaskHandle_t m_th_file_dnl_str = NULL;
static char* m_fdnl_url = NULL;
static char* m_fdnl_cert = NULL;

//static void http_perform_as_stream_reader(void)
//{
//    char *buffer = malloc(MAX_HTTP_RECV_BUFFER + 1);
//    if (buffer == NULL) {
//        ESP_LOGE(TAG, "Cannot malloc http receive buffer");
//        return;
//    }
//    esp_http_client_config_t config = {
//        .url = "http://"CONFIG_EXAMPLE_HTTP_ENDPOINT"/get",
//    };
//    esp_http_client_handle_t client = esp_http_client_init(&config);
//    esp_err_t err;
//    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
//        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
//        free(buffer);
//        return;
//    }
//    int content_length =  esp_http_client_fetch_headers(client);
//    int total_read_len = 0, read_len;
//    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
//        read_len = esp_http_client_read(client, buffer, content_length);
//        if (read_len <= 0) {
//            ESP_LOGE(TAG, "Error read data");
//        }
//        buffer[read_len] = 0;
//        ESP_LOGD(TAG, "read_len = %d", read_len);
//    }
//    ESP_LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %"PRId64,
//                    esp_http_client_get_status_code(client),
//                    esp_http_client_get_content_length(client));
//    esp_http_client_close(client);
//    esp_http_client_cleanup(client);
//    free(buffer);
//}

static void file_download_and_stream_task( void * pvParameters )
{
	int ret = 0;
	/* allocate memory to read http data  */
    char *buffer = (char *) calloc(1, FILE_DNL_BUFF_SIZE);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "Cannot malloc http receive buffer");
        goto exit_task;
    }

    /* initialize http client */
	esp_http_client_config_t config = { .url = m_fdnl_url,
//				.event_handler = _http_event_handler,
//				.buffer_size = 10,
//				.buffer_size = 1024 * 2,
//				.is_async = true,
//				.user_data = output_buffer,
				.cert_pem = m_fdnl_cert, };
	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL) {
		ESP_LOGE(TAG, "esp_http_client_init failed");
		goto exit_task;
	}

	/* open an http connection */
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        goto exit_task;
    }
    ESP_LOGI(TAG, "Made HTTP connection with: %s", m_fdnl_url);

    /* read the content length */
    int content_length = esp_http_client_fetch_headers(client);
    ESP_LOGI(TAG, "HTTP content length = %d", content_length);

    host_cmds_net_total_payload_sent_reset();

    int dnl_total = 0;
    while (1) {
    	int read_len = esp_http_client_read(client, buffer, FILE_DNL_BUFF_SIZE);
    	if (read_len < 0) {
    		ESP_LOGE(TAG, "esp_http_client_read failed");
    		/* incase of error, end the stream so host processor does not hang and then exit */
        	ret = host_cmds_net_send_stream_resp(UART_M2M_FRAME_DATA_RESP_ENDSTR,
        			C20X_M2M_CMD_FILE_DOWNLOAD, buffer, read_len);
    		break;
    	}

    	ESP_LOGD(TAG, "HTTP read %d bytes", read_len);
//    	ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, read_len, ESP_LOG_WARN);

    	/* send data (stream response packet) to host processor */
    	ret = host_cmds_net_send_stream_resp(UART_M2M_FRAME_DATA_RESP,
    			C20X_M2M_CMD_FILE_DOWNLOAD, buffer, read_len);
    	if (ret != 0) {
    		ESP_LOGE(TAG, "failed to send stream response %d", ret);
    		continue;
    	}

    	dnl_total += read_len;
    	if (dnl_total >= content_length) {
    		ESP_LOGI(TAG, "Read complete %d bytes", dnl_total);

    		/* send stream end packet to host processor */
        	ret = host_cmds_net_send_stream_resp(UART_M2M_FRAME_DATA_RESP_ENDSTR,
        			C20X_M2M_CMD_FILE_DOWNLOAD, buffer, read_len);
        	ESP_LOGI(TAG, "Sent %d bytes to host processor", host_cmds_net_total_payload_sent_get());
    		break;
    	}
	}
    esp_http_client_close(client);
    esp_http_client_cleanup(client);


exit_task:
	free(buffer);
	vTaskDelete(NULL);
}

int app_net_file_download_url_set(const char* url, int url_len)
{
	if ((url == NULL) || (url_len <= 0))	return -EINVAL;

	if (m_fdnl_url != NULL)	free(m_fdnl_url);

	m_fdnl_url = (char*)calloc(1, url_len+1);
	if (m_fdnl_url == NULL) {
		ESP_LOGE(TAG, "calloc failed, %s", __func__);
		return -ENOMEM;
	}

	memcpy(m_fdnl_url, url, url_len);
	ESP_LOGI(TAG, "settings file download URL to: %s", m_fdnl_url);
	return 0;
}

int app_net_file_download_cert_set(const char* cert, int cert_len)
{
	if ((cert == NULL) || (cert_len <= 0))	return -EINVAL;

	if (m_fdnl_cert != NULL)	free(m_fdnl_cert);

	m_fdnl_cert = (char*)calloc(1, cert_len+1);
	if (m_fdnl_cert == NULL) {
		ESP_LOGE(TAG, "calloc failed, %s", __func__);
		return -ENOMEM;
	}

	memcpy(m_fdnl_cert, cert, cert_len);
	ESP_LOGI(TAG, "settings file download CERT to: %s", m_fdnl_cert);
	return 0;
}

int app_net_http_file_download_and_stream_start(void)
{
	if (m_fdnl_url == NULL) {
		ESP_LOGE(TAG, "URL not set, call app_net_file_download_url_set() first");
		return -EINVAL;
	}

	/**
	 * This function starts a thread which downloads a file using https protocol.
	 * The file is not saved into the storage but it is streamed to the host processor
	 * using a FIFO mechanism.
	 */
	BaseType_t xret = xTaskCreate(file_download_and_stream_task, "file_dnl_str",
			FILE_DNL_STR_TASK_STACK, NULL, FILE_DNL_STR_TASK_PRIO,
			&m_th_file_dnl_str);
	if (xret == pdPASS) {
		ESP_LOGI(TAG, "file download and stream task created successfully");
	} else {
		ESP_LOGE(TAG, "file download and stream task creation failed");
		return -1;
	}
	return 0;
}

int app_net_http_file_download_init(void)
{
	int ret = 0;

	return ret;
}
