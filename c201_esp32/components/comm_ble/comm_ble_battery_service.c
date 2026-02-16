/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_service_battery.c
 * Created on: 22-Apr-2021
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
#include "comm_ble_common.h"
#include "comm_ble_blower_device_profile.h"
#include "comm_ble_battery_service.h"
#include "app_events.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"
#include "c20x_m2m_cmds.h"

#define TAG	"comm_ble_battery_svc"

#define SVC_INST_ID             	0

/* App event related data */
static struct app_events_callback m_evntcb_toupdt_coproc;
static struct app_events_callback m_evntcb_updting_coproc;
static struct app_events_callback m_evntcb_updted_coproc;
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event);

struct bas_control {
	TaskHandle_t ntf_task;
	bool stay_alive;
	esp_gatt_if_t gatts_if;
	uint16_t conn_id;
};
static struct bas_control m_bas_ctrl = {
		.ntf_task = NULL,
		.stay_alive = false
};

static void task_batt_notify( void * pvParameters );

/* Battery Service Attribute Indexes */
enum {
    BAS_IDX_SVC,

    BAS_IDX_BATT_LVL_CHAR,
    BAS_IDX_BATT_LVL_VAL,
    BAS_IDX_BATT_LVL_NTF_CFG,
//    BAS_IDX_BATT_LVL_PRES_FMT,

    BAS_IDX_NB,
};

/// characteristic presentation information
struct prf_char_pres_fmt
{
    /// Unit (The Unit is a UUID)
    uint16_t unit;
    /// Description
    uint16_t description;
    /// Format
    uint8_t format;
    /// Exponent
    uint8_t exponent;
    /// Name space
    uint8_t name_space;
};

/* Array of handles to each element */
static uint16_t handle_table_battery[BAS_IDX_NB];

/* Battery Information Service UUID */
static const uint16_t m_battery_svc 		= ESP_GATT_UUID_BATTERY_SERVICE_SVC;

static const uint16_t m_battery_level_uuid 	= ESP_GATT_UUID_BATTERY_LEVEL;
static uint8_t m_battery_level_ccc[2]		= { 0x00, 0x00}; /* keep notifications disabled by default */
static uint8_t m_battery_lev = 50;

/* Battery Service Database Description */
static const esp_gatts_attr_db_t gatt_db_battery_svc[BAS_IDX_NB] =
{
    // Battery Service Declaration
    [BAS_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
    		sizeof(uint16_t), sizeof(m_battery_svc), (uint8_t *)&m_battery_svc}},

    // Battery level Characteristic Declaration
    [BAS_IDX_BATT_LVL_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
    		CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    // Battery level Characteristic Value
    [BAS_IDX_BATT_LVL_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_battery_level_uuid, ESP_GATT_PERM_READ_ENC_MITM,
    		sizeof(uint8_t),sizeof(uint8_t), &m_battery_lev}},

    // Battery level Characteristic - Client Characteristic Configuration Descriptor
    [BAS_IDX_BATT_LVL_NTF_CFG] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
    		sizeof(uint16_t),sizeof(m_battery_level_ccc), (uint8_t *)m_battery_level_ccc}},

    // Battery level report Characteristic Declaration
//    [BAS_IDX_BATT_LVL_PRES_FMT] =
//    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_format_uuid, ESP_GATT_PERM_READ,
//    		sizeof(struct prf_char_pres_fmt), 0, NULL}},
};

/* Callback data and functions */
static struct host_cmd_callback cb_batt_level;
struct cmd_read_cb {
	bool data_available;
	SemaphoreHandle_t mutex;
};
static struct cmd_read_cb cb_data = {
		.data_available = false,
};

void cb_handler_batt_level(struct host_cmd_callback *cb,
		uint32_t cmd,
		void *frame) {
//	struct host_cmd_packet_t *pac = (struct host_cmd_packet_t *) packet;
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
//	if (pac->status == HOST_CMD_STATUS_OK) {
//		memcpy(&m_battery_lev, pac->payload, pac->payload_len);
//	}

	ESP_LOGI(TAG, "cb_handler_batt_level");

	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	/* extract the command id */
	char *tok = strtok((char*)fr->payload, ",\n");
	if (tok == NULL)	return;

	uint16_t cmd_id = atoi(tok);
	if (cmd_id == C20X_M2M_CMD_ID_BATT_LEVEL) {
		/* extract the value */
		tok = strtok(NULL, ",\n");
		if (tok == NULL)	return;

		m_battery_lev = atoi(tok);
	}

	/* set data available variable to true */
	if (cb_data.mutex != NULL) {
		if (xSemaphoreTake(cb_data.mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			cb_data.data_available = true;
			xSemaphoreGive(cb_data.mutex);
		}
	}
}

static void cmd_read_register_callback() {
	host_cmds_add_callback(&cb_batt_level, cb_handler_batt_level, /*CMD_BATTERY_LEVEL_GET*/C20X_M2M_CMD_ID_BATT_LEVEL);
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

void gatts_battery_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_battery_svc, gatts_if, BAS_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_bas_ctrl.gatts_if = gatts_if;
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_battery, BAS_IDX_NB);

		if (ret == BAS_IDX_BATT_LVL_VAL) {
			ESP_LOGI(TAG, "Read event for BAS_IDX_BATT_LVL_VAL, %d", ret);
			/* Get the battery level from host processor */
			char buf[5] = {0x00};

			if (cb_data.mutex != NULL) {
				if (xSemaphoreTake(cb_data.mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
					cb_data.data_available = false;
					xSemaphoreGive(cb_data.mutex);
				} else {
					break;
				}
			}

			ret = host_cmds_battery_level_read(buf, sizeof(buf));

			bool wait_timeout = false;
			uint32_t delay = 0;
			while (!cb_data.data_available) {
				vTaskDelay(DATA_AVAILABLE_LOOP_DELAY);
				delay += DATA_AVAILABLE_LOOP_DELAY;
				if (delay > DATA_AVAILABLE_TIMEOUT) {
					wait_timeout = true;
					ESP_LOGE(TAG, "BAS_IDX_BATT_LVL_VAL wait timeout!");
					break;
				}
			}
			if (!wait_timeout) {
				esp_gatt_rsp_t rsp;
		        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
		        rsp.attr_value.handle = param->read.handle;
		        rsp.attr_value.len = sizeof(m_battery_lev);
		        rsp.attr_value.value[0] = m_battery_lev;
		        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
		                                    ESP_GATT_OK, &rsp);
			}
		} else if (ret == BAS_IDX_BATT_LVL_NTF_CFG) {
			ESP_LOGI(TAG, "Read event for BAS_IDX_BATT_LVL_NTF_CFG, %d", ret);
			ESP_LOGW(TAG, "ccc[0]=0x%x ccc[1]=0x%x", m_battery_level_ccc[0], m_battery_level_ccc[1]);
		}
	}
		break;
	case ESP_GATTS_WRITE_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_battery, BAS_IDX_NB);
		if (ret == BAS_IDX_BATT_LVL_NTF_CFG) {
			ESP_LOGI(TAG, "Write event for BAS_IDX_BATT_LVL_NTF_CFG, %d", ret);
			m_battery_level_ccc[0] = param->write.value[0];
			m_battery_level_ccc[1] = param->write.value[1];
			ESP_LOGW(TAG, "len=%d,ccc[0]=0x%x ccc[1]=0x%x", param->write.len, m_battery_level_ccc[0], m_battery_level_ccc[1]);

			/* if notifications are enabled (ccc_val[0][1]=0x1,0x0), then resume the notification task.
			 * Suspend happens inside the notification task by checking the m_battery_level_ccc value */
			if ((m_battery_level_ccc[0] == 0x01) && (m_battery_level_ccc[1] == 0x00)) {
				vTaskResume(m_bas_ctrl.ntf_task);
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
		m_bas_ctrl.conn_id = param->connect.conn_id;

		/* create a battery level notification task */
		BaseType_t xret = xTaskCreate(task_batt_notify, "batt_notify", COMM_BLE_TASK_STACK_BAS, &m_battery_lev,
				COMM_BLE_TASK_PRIO_BAS, &m_bas_ctrl.ntf_task);
		if (xret == pdPASS) {
			m_bas_ctrl.stay_alive = true;
			ESP_LOGI(TAG, "battery notification task created successfully");
		} else {
			ESP_LOGE(TAG, "battery notification task creation failed");
		}

		/* register to event we want to listen */
		app_events_add_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_add_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_add_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_DISCONNECT_EVT: {
		/* call resume in case the notification task is suspended */
		vTaskResume(m_bas_ctrl.ntf_task);

		/* tell the task to delete itself */
		m_bas_ctrl.stay_alive = false;

		/* remove event callbacks */
		app_events_remove_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_remove_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_remove_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != BAS_IDX_NB) {
			ESP_LOGE(TAG,
					"create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, BAS_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			/* initialize callback data and register callback functions with host_cmds module */
			cb_data.mutex = xSemaphoreCreateMutex();
			if (cb_data.mutex != NULL) {
				cmd_read_register_callback();
			}

			memcpy(handle_table_battery, param->add_attr_tab.handles, sizeof(handle_table_battery));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_battery[BAS_IDX_SVC]);
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

/* App Events handler */
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event) {
	switch (event) {
	case APP_EVENT_FW_TO_UPDATE_COPROC:
		ESP_LOGI(TAG, "APP_EVENT_FW_TO_UPDATE_COPROC");
		vTaskSuspend(m_bas_ctrl.ntf_task);
		break;
	case APP_EVENT_FW_UPDATING_COPROC:
		break;
	case APP_EVENT_FW_UPDATED_COPROC:
		break;
	default:
		break;
	}
}

/* Tasks */
static void task_batt_notify( void * pvParameters ) {
//	uint8_t *p_battery_level = (uint8_t *)pvParameters;
	char batt_level[5] = {0x00};
	esp_err_t ret = 0;

	while (1) {

		/* if the connection is not alive then kill this task */
		if (!m_bas_ctrl.stay_alive) {
			break;
		}

		/* if notifications are disabled, then suspend this task */
		if ((m_battery_level_ccc[0] == 0x00) && (m_battery_level_ccc[1] == 0x00)) {
			ESP_LOGW(TAG, "suspending battery notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from battery notification task");
		}

		/* Get the battery level from host processor */
		if (cb_data.mutex != NULL) {
			if (xSemaphoreTake(cb_data.mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
				cb_data.data_available = false;
				xSemaphoreGive(cb_data.mutex);
			} else {
				break;
			}
		}

		ret = host_cmds_battery_level_read(batt_level, sizeof(batt_level));

		bool wait_timeout = false;
		uint32_t delay = 0;
		while (!cb_data.data_available) {
			vTaskDelay(DATA_AVAILABLE_LOOP_DELAY);
			delay += DATA_AVAILABLE_LOOP_DELAY;
			if (delay > DATA_AVAILABLE_TIMEOUT) {
				wait_timeout = true;
				ESP_LOGE(TAG, "task_batt_notify wait timeout!");
				break;
			}
		}

		/* Notify to connected app */
		if (!wait_timeout) {
			ret = esp_ble_gatts_send_indicate(m_bas_ctrl.gatts_if, m_bas_ctrl.conn_id,
					handle_table_battery[BAS_IDX_BATT_LVL_VAL], sizeof(uint8_t), &m_battery_lev, false);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
			} else {
				ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
			}
		}

		/* TODO update rate from kconfig */
		vTaskDelay((30*1000) / portTICK_PERIOD_MS);
	}

	ESP_LOGW(TAG, "deleting battery notification task ...");
	vTaskDelete(NULL);
}

