/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_dfu_service.c
 * Created on: 23-Jun-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include <string.h>
#include <stdlib.h>

#include "host_cmds.h"
#include "host_cmds_callback.h"
#include "host_cmds_packet.h"
#include "host_cmds_send_recv.h"
#include "comm_ble_common.h"
//#include "comm_ble_blower_device_profile.h"
#include "comm_ble_dfu_service.h"
#include "app_events.h"
#if (CONFIG_BOARD_C201 || CONFIG_BOARD_C204)
	#if CONFIG_STM32_USART_BL_HOST_ENABLE
		#include "stm32_usart_bl_host.h"
	#endif
#endif
#include <c20x_m2m_cmds.h>

#define TAG	"comm_ble_dfu_svc"

#define SVC_INST_ID             	0

typedef enum {
	TASK_DFU_OPER=0,
//	TASK_DFU_NTF,
	TASK_NB
} DFU_TASK_IDX;
#define NUM_DFU_TASKS		(TASK_NB)

struct dfu_control {
	TaskHandle_t ntf_task;
	bool stay_alive;
};
static struct dfu_control m_dfu_ctrl[NUM_DFU_TASKS] = {
		[TASK_DFU_OPER] = {.ntf_task = NULL, .stay_alive = false},
};

esp_gatt_if_t m_gatts_if;
uint16_t m_conn_id;

static void task_dfu_operation( void * pvParameters );

/* Battery Service Attribute Indexes */
enum {
    DFU_IDX_SVC,

    DFU_IDX_START_CHAR,
    DFU_IDX_START_VAL,

    DFU_IDX_NB,
};

/* Array of handles to each element */
static uint16_t handle_table_dfu[DFU_IDX_NB];

/* DFU UUID and values */
struct dfu_values {
	uint8_t dfu_status;
};
static struct dfu_values m_dfu_val = {0};

/* Device Firmware Upgrade Service UUID */
static const uint16_t m_dfu_svc 			= 0x8010;
static const uint16_t m_dfu_status_uuid 	= 0x8020;

/* DFU Service Database Description */
static const esp_gatts_attr_db_t gatt_db_dfu_svc[DFU_IDX_NB] =
{
    // DFU Service Declaration
    [DFU_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
    		sizeof(uint16_t), sizeof(m_dfu_svc), (uint8_t *)&m_dfu_svc}},

    // DFU Status Characteristic Declaration
    [DFU_IDX_START_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    // DFU Status Characteristic Value
    [DFU_IDX_START_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_dfu_status_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		sizeof(uint8_t),sizeof(uint8_t), &m_dfu_val.dfu_status}},
};

/* Callback data and functions */
typedef enum {
	CB_DFU_STATUS=0,
	CB_NB
} READ_CB;
#define NUM_READ_CB		(CB_NB)
static struct host_cmd_callback cb_dfu_status;
static struct host_cmd_packet_t m_packet;					/* global packet object */
static SemaphoreHandle_t m_packet_mutex;					/* mutex to protect the global packet object */
struct cmd_read_cb {
	bool data_available;
	char cmd[HOST_CMD_COMMAND_SIZE_MAX];
	SemaphoreHandle_t cmd_mutex;
};
static struct cmd_read_cb m_cb_data[NUM_READ_CB] = {
		[CB_DFU_STATUS] = {
				.data_available = false,
				.cmd = CMD_DFU_START
		},
};

void cb_handler_dfu_status(struct host_cmd_callback *cb, uint32_t cmd, void *packet) {
	struct host_cmd_packet_t *pac = (struct host_cmd_packet_t *) packet;

	ESP_LOGI(TAG, "cb_handler_dfu_status");
/*
	ESP_LOGI(TAG, "pac.type = %d", pac->type);
	ESP_LOGI(TAG, "pac.sequence = %d", pac->sequence);
	ESP_LOGI(TAG, "pac.cmd_len = %d", pac->cmd_len);
	ESP_LOGI(TAG, "pac.cmd:");
	ESP_LOG_BUFFER_HEXDUMP(TAG, pac->cmd, pac->cmd_len, ESP_LOG_INFO);
	ESP_LOGI(TAG, "pac.status = %d", pac->status);
	ESP_LOGI(TAG, "pac.payload_len = %d", pac->payload_len);
	ESP_LOGI(TAG, "pac.payload:");
	ESP_LOG_BUFFER_HEXDUMP(TAG, pac->payload, pac->payload_len, ESP_LOG_INFO);
*/

	/* As there are multiple consumers of this data we copy the entire packet here
	 * and perform tasks in other threads */
	if (m_packet_mutex != NULL) {
		if (xSemaphoreTake(m_packet_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			memcpy(&m_packet, pac, sizeof(struct host_cmd_packet_t));
			xSemaphoreGive(m_packet_mutex);
		}
	}

	/* set the appropriate data available variable to true */
	for (int i = 0; i < NUM_READ_CB; i++) {
		if (!strcmp((char*) m_packet.cmd, m_cb_data[i].cmd)) {
			if (!strcmp((char*) m_packet.cmd, CMD_DFU_START)) {
				/* create the DFU operation task */
				if (m_dfu_ctrl[TASK_DFU_OPER].ntf_task == NULL) {
					BaseType_t xret = xTaskCreate(task_dfu_operation, "dfu_oper", COMM_BLE_TASK_STACK_DFU, NULL,
					COMM_BLE_TASK_PRIO_DFU, &m_dfu_ctrl[TASK_DFU_OPER].ntf_task);
					if (xret == pdPASS) {
						m_dfu_ctrl[TASK_DFU_OPER].stay_alive = true;
						ESP_LOGI(TAG, "DFU operation task created successfully");
					} else {
						ESP_LOGE(TAG, "DFU operation task creation failed");
					}
				}
			}

			if (xSemaphoreTake(m_cb_data[i].cmd_mutex, (TickType_t) 10 / portTICK_PERIOD_MS) == pdTRUE) {
				m_cb_data[i].data_available = true;
				xSemaphoreGive(m_cb_data[i].cmd_mutex);
			}
		}
	}
}

static void cmd_read_register_callback() {
	m_cb_data[CB_DFU_STATUS].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_dfu_status, cb_handler_dfu_status, C20X_M2M_CMD_ID_DFU);
}

static uint8_t find_char_and_desr_index(uint16_t handle, uint16_t *table, int num_elem) {
	uint8_t error = 0xff;

	for (int i = 0; i < num_elem; i++) {
		if (handle == table[i]) {
			return i;
		}
	}
	return error;
}

static int update_data_availability(SemaphoreHandle_t mutex, bool *pvar, bool val) {
	if (mutex != NULL) {
		if (xSemaphoreTake(mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			*pvar = val;
			xSemaphoreGive(mutex);
			return 0;
		} else {
			return -1;
		}
	}
	return -1;
}

static int wait_until_timeout(bool *pvar) {
//	bool wait_timeout = false;
	uint32_t delay = 0;
	while (!(*pvar)) {
		vTaskDelay(DATA_AVAILABLE_LOOP_DELAY);
		delay += DATA_AVAILABLE_LOOP_DELAY;
		if (delay > DATA_AVAILABLE_TIMEOUT) {
//			wait_timeout = true;
			return -1;
		}
	}
	return 0;
}

static int send_ble_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint32_t len, uint8_t *data) {
	esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = len;
    memcpy(rsp.attr_value.value, data, len);
    return esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

/* DFU gatt server event handler */
void gatts_dfu_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_dfu_svc, gatts_if, DFU_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_gatts_if = gatts_if;
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_dfu, DFU_IDX_NB);
		if (ret == DFU_IDX_START_VAL) {
			ESP_LOGI(TAG, "Read event for DFU_IDX_START_VAL, %d", ret);
#if 0
			if (update_data_availability(m_cb_data[CB_DFU_STATUS].cmd_mutex,
					&m_cb_data[CB_DFU_STATUS].data_available, false) < 0)
				break;

			ret = host_cmds_dfu_status_read();	/* Get the DFU status */
			if (wait_until_timeout(&m_cb_data[CB_DFU_STATUS].data_available) < 0)
				break;
			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
#endif
		}
	}
		break;
	case ESP_GATTS_WRITE_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_dfu, DFU_IDX_NB);

		switch (ret) {
		case DFU_IDX_START_VAL: {
			ESP_LOGI(TAG, "Write event for DFU_IDX_START_VAL, %d", ret);
			memcpy(&m_dfu_val.dfu_status, param->write.value, param->write.len);
			ret = host_cmds_dfu_status_write(m_dfu_val.dfu_status);
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
			break;
		}
		}
		break;
	case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
		break;
	case ESP_GATTS_CONF_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
		break;
	case ESP_GATTS_START_EVT:
		ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status,
				param->start.service_handle);
		break;
	case ESP_GATTS_CONNECT_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
		m_conn_id = param->connect.conn_id;

	}
		break;
	case ESP_GATTS_DISCONNECT_EVT: {
		if (m_dfu_ctrl[TASK_DFU_OPER].ntf_task != NULL) {
			/* call resume in case the notification task is suspended */
			vTaskResume(m_dfu_ctrl[TASK_DFU_OPER].ntf_task);
			/* tell the task to delete itself */
			m_dfu_ctrl[TASK_DFU_OPER].stay_alive = false;
		}
	}
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != DFU_IDX_NB) {
			ESP_LOGE(TAG,
					"create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, DFU_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			/* initialize callback data and register callback functions with host_cmds module */
			m_packet_mutex = xSemaphoreCreateMutex();
			if (m_packet_mutex != NULL) {
				cmd_read_register_callback();
			}

			memcpy(handle_table_dfu, param->add_attr_tab.handles, sizeof(handle_table_dfu));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_dfu[DFU_IDX_SVC]);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "battery service started successfully");
			}
		}
		break;
	}

	case ESP_GATTS_MTU_EVT:
	case ESP_GATTS_STOP_EVT:
	case ESP_GATTS_OPEN_EVT:
	case ESP_GATTS_CANCEL_OPEN_EVT:
	case ESP_GATTS_CLOSE_EVT:
	case ESP_GATTS_LISTEN_EVT:
	case ESP_GATTS_CONGEST_EVT:
	case ESP_GATTS_UNREG_EVT:
	case ESP_GATTS_DELETE_EVT:
	default:
		break;
	}
}

static void task_dfu_operation( void * pvParameters ) {
	/* Report co-processor firmware update event and wait for sometime */
	app_events_report_event(APP_EVENT_FW_TO_UPDATE_COPROC);
	vTaskDelay((1*1000) / portTICK_PERIOD_MS);
#if 0
	int ret = 0;
	while (1) {
		/* By this time the co-processor must be in bootloader mode, ready to accept new firmware,
		 * All other threads must be in suspend state
		 * Steps to update:
		 * 1. Reinitialize the UART driver with required settings
		 * 2. Connect with bootloader
		 * 3.
		 * */

		/* 1. Reinitialize the UART driver with required settings */
//		ret = host_cmds_send_recv_reinit_for_dfu();
#if CONFIG_STM32_USART_BL_HOST_ENABLE
		ret = stm32_ubl_usart_open();
#endif
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "stm32_ubl_usart_open failed!");
			break;
		}

//		ret = host_cmd_send_rev_dfu_check();
#if CONFIG_STM32_USART_BL_HOST_ENABLE
		ret = stm32_ubl_start_check();
#endif
		if (ret != 0) {
			ESP_LOGE(TAG, "stm32_ubl_start_check failed!");
			break;
		}

		/* 2. Connect with bootloader */


		break;
		/* TODO update rate from kconfig */
//		vTaskDelay((1*1000) / portTICK_PERIOD_MS);
	}

	/* if we reach here it means the DFU process has ended
	 * so we reinitialize the uart for command send recv
	 *  */
	ret = host_cmds_send_recv_reinit_for_cmd();
	ret = host_cmds_verify();
	ret = host_cmds_send_recv_start();

	/* Report co-processor firmware update complete event and wait for sometime */
	app_events_report_event(APP_EVENT_FW_UPDATED_COPROC);
#endif
	ESP_LOGW(TAG, "deleting dfu operation task ...");
	m_dfu_ctrl[TASK_DFU_OPER].ntf_task = NULL;
	vTaskDelete(NULL);
}
