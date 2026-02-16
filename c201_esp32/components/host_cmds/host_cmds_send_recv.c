/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_send_recv.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include <sys/errno.h>

#include "host_cmds_uart_config.h"
#include "host_cmds_priv.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_callback.h"
#include "host_cmds_packet.h"
#include "gll.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"
#include "c20x_m2m_cmds.h"

#define TAG	"host_cmds"


/* static variables */
static gll_t *m_callbacks;	/* a list of callback objects */
static int m_pos = 0;		/* node position in the list */
static SemaphoreHandle_t m_mutex;	/* mutex for transmission of data */
static TaskHandle_t m_recv_task = NULL;

/* callback firing tools */
#define CMD_CBFIRE_Q_LEN		10
static TaskHandle_t m_cbfire_task = NULL;
static SemaphoreHandle_t m_cbfire_mutex;
static QueueHandle_t m_cbfire_q;

static HOST_CMDS_SEND_RECV_MODE m_mode = SEND_RECV_MODE_CMD;

static volatile uint32_t m_last_seq = 0;	/* the last incomming packet sequence received */

static int data_response_ack_make_send(uint16_t cmd_id, struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame);

/* 'heap' command prints minumum heap size */
static int heap_size_print()
{
    uint32_t heap_size = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    printf("min heap size: %"PRIu32"\n", heap_size);
    return 0;
}

static int send_data(uart_port_t uart_num, const char *data, size_t len) {
	const int txBytes = uart_write_bytes(uart_num, data, len);
	return txBytes;
}

int host_cmds_send_only(const char *data, size_t len) {
	int tx_bytes;

	if (xSemaphoreTake(m_mutex, (TickType_t) 100/portTICK_PERIOD_MS) == pdTRUE) {
		tx_bytes = send_data(UART_NUM, data, len);
		xSemaphoreGive(m_mutex);
		ESP_LOGD(TAG, "Wrote %d bytes and Data = %s", tx_bytes, data);
		return tx_bytes;
	}
	return -1;
}

int host_cmds_recv_bytes(void* buf, uint32_t length, uint32_t ms_to_wait) {
	if (ms_to_wait == 0xFFFFFFFF)
		return uart_read_bytes(UART_NUM, buf, length, (TickType_t) portMAX_DELAY);
	else
		return uart_read_bytes(UART_NUM, buf, length, (TickType_t) ((ms_to_wait) / portTICK_PERIOD_MS));
}

void host_cmds_delay(uint32_t ms_delay) {
	vTaskDelay(ms_delay / portTICK_PERIOD_MS);
}

static void cmd_recv_task( void * pvParameters ) {
	ESP_LOGW(TAG, "Starting cmd_recv_task!");

	struct host_cmd_packet_t pac;
	int rx_bytes=0;

	while (1) {
//		uart_flush(UART_NUM);

		switch (m_mode) {
		case SEND_RECV_MODE_CMD: {
			struct m2m_frame_t *frame = (struct m2m_frame_t *)calloc(1, sizeof(struct m2m_frame_t));
			if (frame == NULL) {
				ESP_LOGE(TAG, "calloc failed at %s", __func__);
				return;
			}
//			memset(&frame, 0x00, sizeof(struct m2m_frame_t));

			/* Read and decode the received buffer */

			// SOF
			rx_bytes = uart_read_bytes(UART_NUM, &frame->sof, sizeof(frame->sof), (TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->sof)) {
			}
			if (frame->sof == UART_M2M_START_OF_FRAME) {
				ESP_LOGD(TAG, "received SOF 0x%x, proceeding", frame->sof);
			} else {
				ESP_LOGE(TAG, "incorrect SOF 0x%x, aborting!", frame->sof);
				ESP_LOG_BUFFER_HEXDUMP(TAG, &frame->sof, sizeof(frame->sof), ESP_LOG_DEBUG);
				continue;
			}

			// type
			rx_bytes = uart_read_bytes(UART_NUM, &frame->type, sizeof(frame->type), (TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->type)) {
				ESP_LOGE(TAG, "did not receive type, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "frame->type = %d", frame->type);
			if (frame->type >= UART_M2M_FRAME_MAX) {
				ESP_LOGE(TAG, "incorrect packet type received, aborting!");
				continue;
			}

			// sequence
			rx_bytes = uart_read_bytes(UART_NUM, &frame->sequence, sizeof(frame->sequence), (TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->sequence)) {
				ESP_LOGE(TAG, "did not receive sequence, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "frame->sequence = %ld", frame->sequence);

			// acknowledgment
			rx_bytes = uart_read_bytes(UART_NUM, &frame->ack, sizeof(frame->ack), (TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->ack)) {
				ESP_LOGE(TAG, "did not receive sequence, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "frame->ack = %ld", frame->ack);

			// checksum
			rx_bytes = uart_read_bytes(UART_NUM, &frame->checksum, sizeof(frame->checksum), (TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->checksum)) {
				ESP_LOGE(TAG, "did not receive sequence, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "frame->checksum = 0x%x", frame->checksum);

			// payload_len
			rx_bytes = uart_read_bytes(UART_NUM, &frame->payload_len, sizeof(frame->payload_len),
					(TickType_t) portMAX_DELAY);
			if (rx_bytes != sizeof(frame->payload_len)) {
				ESP_LOGE(TAG, "did not receive payload_len, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "frame->payload_len = %ld", frame->payload_len);

			// payload
			if ((frame->payload_len > 0) && (frame->payload_len <= UART_M2M_PAYLOAD_SIZE_MAX)) {
				rx_bytes = uart_read_bytes(UART_NUM, frame->payload, frame->payload_len, (TickType_t) portMAX_DELAY);
				if (rx_bytes != frame->payload_len) {
					ESP_LOGE(TAG, "did not receive payload, aborting!");
					continue;
				}
			} else {
				ESP_LOGE(TAG, "incorrect payload length, aborting!");
				continue;
			}
			ESP_LOGD(TAG, "queue sending");

			/* send extracted frame to callback firing thread */
			if ( xQueueSend( m_cbfire_q, (void *) &frame, ( TickType_t ) 10 ) != pdPASS) {
				ESP_LOGE(TAG, "xQueueSend failed for m_cbfire_q");
				return;
			}
			break;
		}

		case SEND_RECV_MODE_DFU: {
			vTaskDelay(200 / portTICK_PERIOD_MS);
			break;
		}

		case SEND_RECV_MODE_STREAM: {
			vTaskDelay(200 / portTICK_PERIOD_MS);
			break;
		}

		default: {
			vTaskDelay(200 / portTICK_PERIOD_MS);
			break;
		}
		}

//		vTaskDelay(50 / portTICK_PERIOD_MS);
	}
	ESP_LOGE(TAG, "Deleting cmd_recv_task!");
	m_recv_task = NULL;
	vTaskDelete(NULL);
}

static void cb_fire_task( void * pvParameters )
{
	ESP_LOGI(TAG, "cb_fire_task started");
	struct m2m_frame_t *frame;
	struct m2m_frame_t out_frame;;
	int ret = 0;

	while (1) {
		if (xQueueReceive(m_cbfire_q, &(frame), (TickType_t) portMAX_DELAY) != pdPASS) {
			ESP_LOGE(TAG, "xQueueReceive failed!");
			continue;
		}

//		heap_size_print();

		ESP_LOGD(TAG, "frame sof = 0x%x", frame->sof);
		ESP_LOGD(TAG, "frame type = %d", frame->type);
		ESP_LOGD(TAG, "frame seq = %ld", frame->sequence);
		ESP_LOGD(TAG, "frame ack = %ld", frame->ack);
		ESP_LOGD(TAG, "frame checksum = 0x%x", frame->checksum);
		ESP_LOGD(TAG, "frame payload length = %ld", frame->payload_len);
//		ESP_LOG_BUFFER_HEXDUMP(TAG, frame->payload, frame->payload_len, ESP_LOG_WARN);

		// verify checksum
		int ck = lib_m2m_frame_checksum_verify(frame);
		if (ck != 0) {
			ESP_LOGE(TAG, "checksum did not match!\n");
			goto err;
		}

		/* extract the command id and fire appropriate callback */
		char payload[UART_M2M_PAYLOAD_SIZE_MAX] = {0x00};
		memcpy(payload, frame->payload, frame->payload_len);
		char *scmd = strtok(payload, ",");
		if (scmd != NULL) {
			int cmd = atoi(scmd);
			ESP_LOGD(TAG, "Extracted command = %d", cmd);

			/* if frame type is DATA_RESP or DATA_RESP_ENDSTR then send acknowledgment */
			if (frame->type == UART_M2M_FRAME_DATA_RESP) {	// received data response frame
				ret = data_response_ack_make_send(cmd, frame, &out_frame);
				if (ret == 0) {	// ok
					// nothing to here, the callback gets called down below
				} else if (ret == -EBADF) {	// duplicate frame
					goto err;
				} else if (ret < 0) {		// error
					ESP_LOGE(TAG, "data_response_ack_make_send failed!");
					goto err;
				}
			} else if (frame->type == UART_M2M_FRAME_DATA_RESP_ENDSTR) {
				ret = data_response_ack_make_send(cmd, frame, &out_frame);
				if (ret == 0) {	// ok
					m_last_seq = 0;		// reset the sequence variable
					// the callback gets called down below
				} else if (ret == -EBADF) {	// duplicate frame
					goto err;
				} else if (ret < 0) {		// error
					ESP_LOGE(TAG, "data_response_ack_make_send failed!");
					goto err;
				}
			}

			/* Search and fire the appropriate callback */
			host_cmds_fire_callback(m_callbacks, cmd, frame);

			ESP_LOGD(TAG, "host_cmds_fire_callback returned");
		} else {
			ESP_LOGE(TAG, "Could not extract command!");
		}
err:
		;
//		free(frame);
	}
}

static int data_response_ack_make_send(uint16_t cmd_id, struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame)
{
	int ret = 0;
	/* make the acknowledgment frame */
	lib_m2m_frame_header_data_ack_make(out_frame, in_frame->sequence);

	memset(out_frame->payload, 0x00, UART_M2M_PAYLOAD_SIZE_MAX);
	sprintf((char*) out_frame->payload, "%d%s%s%s", cmd_id, M2M_CMD_PAYLOAD_DELIM,
			M2M_CMD_RESP_OK, M2M_CMD_PAYLOAD_TERM);
	out_frame->payload_len = strlen((const char *)out_frame->payload);

	/* compute checksum */
	ret = lib_m2m_frame_checksum_compute(out_frame);
	if (ret < 0) goto err;

	/* send acknowledgment frame */
	size_t sdata_len=0;
	uint8_t *serialized_buffer = lib_m2m_frame_alloc_serialize(out_frame, &sdata_len);
	if (serialized_buffer == NULL) {
		ret = -1;
		goto err;
	}
	host_cmds_send_only((const char *)serialized_buffer, sdata_len);

	/* free memory */
	free(serialized_buffer);

	/* check if this was a duplicate packet */
	if (m_last_seq == in_frame->sequence) {
		ESP_LOGW(TAG, "duplicate packet, ignoring");
		/* we should discard the duplicate packet's data but must acknowledge
		 * the packet so that the sender does not re-transmit the same packet again */
		ret = -EBADF;
	}
	m_last_seq = in_frame->sequence;
	ESP_LOGD(TAG, "m_last_seq = %ld", m_last_seq);

err:
	return ret;
}


int host_cmds_add_callback(struct host_cmd_callback *cb_data,
		host_cmd_callback_handler_t handler, uint32_t cmd) {

	if (cb_data == NULL) {
		ESP_LOGE(TAG, "Incorrect data!");
		return ESP_FAIL;
	}

	if (handler == NULL) {
		ESP_LOGE(TAG, "Incorrect handler!");
		return ESP_FAIL;
	}

	cb_data->pos = m_pos++;
	cb_data->handler = handler;
	cb_data->cmd = cmd;

	host_cmds_manage_callback(m_callbacks, cb_data, true);

	return 0;
}

void host_cmds_send_recv_set_mode(HOST_CMDS_SEND_RECV_MODE mode) {
	if (mode > MODE_MAX)
		m_mode = SEND_RECV_MODE_CMD;

	m_mode = mode;
}

int host_cmds_send_recv_init() {
	m_callbacks = gll_init();
	m_mutex = xSemaphoreCreateMutex();

	if ((m_callbacks == NULL) || (m_mutex == NULL)) {
		return -1;
	} else {
		return 0;
	}
}

int host_cmds_send_recv_start() {
	int ret = 0;
	host_cmds_send_recv_set_mode(SEND_RECV_MODE_CMD);

	BaseType_t xret = xTaskCreate(cmd_recv_task, "cmd_recv_task", HOST_CMDS_RECEIVE_TASK_STACK, NULL,
									HOST_CMDS_RECEIVE_TASK_PRIO, &m_recv_task);
	if (xret == pdPASS) {
		ESP_LOGI(TAG, "commands receive task created successfully");
		ret = 0;
	} else {
		ESP_LOGE(TAG, "commands receive task creation failed");
		return -1;
	}

	/* callback firing thread and tools */
	m_cbfire_mutex = xSemaphoreCreateMutex();
	m_cbfire_q = xQueueCreate(CMD_CBFIRE_Q_LEN, sizeof(struct cmd_req_data *));

	if ((m_cbfire_mutex == NULL) || (m_cbfire_q == NULL)) {
		ESP_LOGE(TAG, "could not allocate resources!");
		return -1;
	}
	xret = xTaskCreate(cb_fire_task, "cb_fire_task", HOST_CMDS_CBFIRE_TASK_STACK, NULL,
									HOST_CMDS_CBFIRE_TASK_PRIO, &m_cbfire_task);
	if (xret == pdPASS) {
		ESP_LOGI(TAG, "callback fire task created successfully");
		ret = 0;
	} else {
		ESP_LOGE(TAG, "commands receive task creation failed");
		return -1;
	}

	return ret;
}

int host_cmds_send_recv_init_for_cmd() {
	int ret = 0;
	/* uart configuration for send recv commands with host */
		const uart_config_t uart_config = {
				.baud_rate = 921600, //460800, // 921600 //115200,
				.data_bits = UART_DATA_8_BITS,
				.parity = UART_PARITY_DISABLE,
				.stop_bits = UART_STOP_BITS_1,
	#if (HW_FLOWCTRL_DISABLE)
				.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	#elif (HW_FLOWCTRL_RTS)
				.flow_ctrl = UART_HW_FLOWCTRL_RTS,
	#elif (HW_FLOWCTRL_CTS)
				.flow_ctrl = UART_HW_FLOWCTRL_CTS,
	#elif (HW_FLOWCTRL_CTS_RTS)
				.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
	#else
				.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	#endif
				.source_clk = UART_SCLK_APB,
		};

		// We won't use a buffer for sending data.
		ret = uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "uart_driver_install Failed");
			return ret;
		}
		ret = uart_param_config(UART_NUM, &uart_config);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "uart_param_config Failed");
			return ret;
		}

	#if (HW_FLOWCTRL_DISABLE)
		ret = uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	#elif (HW_FLOWCTRL_RTS)
	    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, RTS_PIN, UART_PIN_NO_CHANGE);
	#elif (HW_FLOWCTRL_CTS)
	    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, CTS_PIN);
	#elif (HW_FLOWCTRL_CTS_RTS)
	    ret = uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, RTS_PIN, CTS_PIN);
	#else
	    uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	#endif

	    if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Failed to install uart driver");
			return ret;
		} else {
			ESP_LOGI(TAG, "UART init successful");
		}
	    return ret;
}

int host_cmds_send_recv_reinit_for_cmd() {
	int ret = 0;

	/* delete the receive task */
	if (m_recv_task != NULL) {
		vTaskDelete(m_recv_task);
		m_recv_task = NULL;
	}

	/* uninstall the uart driver */
	ret = uart_driver_delete(UART_NUM);
	if (ret == ESP_OK) {
		while (uart_is_driver_installed(UART_NUM)) {
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	} else {
		ESP_LOGE(TAG, "uart_driver_delete failed");
		return -1;
	}

	ret = host_cmds_send_recv_init_for_cmd();
	return ret;
}

//int host_cmds_send_recv_reinit_for_dfu() {
int host_cmds_send_recv_reinit_for_dfu(int baud_rate, int data_bits, int parity, int stop_bits, int flow_ctrl, int source_clk) {
	int ret = 0;

	/* delete the receive task */
	if (m_recv_task != NULL) {
		vTaskDelete(m_recv_task);
		m_recv_task = NULL;
	}

	/* uninstall the uart driver */
	ret = uart_driver_delete(UART_NUM);
	if (ret == ESP_OK) {
		while (uart_is_driver_installed(UART_NUM)) {
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
	} else {
		ESP_LOGE(TAG, "uart_driver_delete failed");
		return -1;
	}

	/* install the uart driver with new settings */
/*
	const uart_config_t uart_config = { .baud_rate = 115200, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_EVEN,
			.stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_APB, };
*/

	const uart_config_t uart_config = { .baud_rate = baud_rate, .data_bits = data_bits, .parity = parity,
			.stop_bits = stop_bits, .flow_ctrl = flow_ctrl, .source_clk = source_clk, };

	// We won't use a buffer for sending data.
	ret |= uart_driver_install(UART_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
	ret |= uart_param_config(UART_NUM, &uart_config);
	ret |= uart_set_pin(UART_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to re-install uart driver");
		return ret;
	} else {
		ESP_LOGI(TAG, "UART re-init successful");
	}

    vTaskDelay(1000 / portTICK_PERIOD_MS);

	return ret;
}

int host_cmd_send_rev_dfu_check() {
	uint8_t txdata = 0x7F;
	const uint8_t rxdata = 0x79;
	size_t len = sizeof(txdata);

	int ret = 0, rx_bytes = 0, tx_bytes = 0;
	uint8_t buf = 0x00;

	int retry=0;
	while (1) {
		if (retry > 10) {
			ESP_LOGE(TAG, "DFU communication failed");
			break;
		}

		tx_bytes = host_cmds_send_only((const char*) &txdata, len);
		if (tx_bytes == len) {
			rx_bytes = uart_read_bytes(UART_NUM, &buf, sizeof(buf), (TickType_t) ((500) / portTICK_PERIOD_MS));
			if (rx_bytes != sizeof(buf)) {
				++retry;
				ESP_LOGE(TAG, "uart_read_bytes failed, rx_bytes = %d", rx_bytes);
				ret = -1;
				vTaskDelay(500 / portTICK_PERIOD_MS);
			} else {

				if (buf == rxdata) {
					ESP_LOGI(TAG, "Connectivity to host for DFU is successful");
					ESP_LOG_BUFFER_HEXDUMP(TAG, &buf, sizeof(buf), ESP_LOG_WARN);

					host_cmds_send_recv_set_mode(SEND_RECV_MODE_DFU);

					/* create the receive task */
					BaseType_t xret = xTaskCreate(cmd_recv_task, "cmd_recv_task", HOST_CMDS_RECEIVE_TASK_STACK, NULL,
					HOST_CMDS_RECEIVE_TASK_PRIO, &m_recv_task);
					if (xret == pdPASS) {
						ESP_LOGI(TAG, "commands receive task created successfully");
						ret = 0;
					} else {
						ESP_LOGE(TAG, "commands receive task creation failed");
						ret = -1;
					}
					break;
				} else {
					++retry;
					ESP_LOGE(TAG, "Connectivity to host for DFU failed");
					ESP_LOG_BUFFER_HEXDUMP(TAG, &buf, sizeof(buf), ESP_LOG_WARN);
					ret = -1;
				}
			}
		} else {
			ESP_LOGE(TAG, "host_cmds_send_only failed, tx_bytes = %d", tx_bytes);
			ret = -1;
			break;
		}
	}

	return ret;
}

