/*
 * Copyright (c) 2022 Acme CPU
 *
 * comm_ble_settings_service.c
 * Created on: 03-Aug-2022
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
#include "comm_ble_common.h"
#include "comm_ble_blower_device_profile.h"
#include "comm_ble_settings_service.h"
#include "app_events.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"
#include "c20x_m2m_cmds.h"

#define TAG	"comm_ble_settings_svc"

#define SVC_INST_ID		0

/* App event related data */
static struct app_events_callback m_evntcb_toupdt_coproc;
static struct app_events_callback m_evntcb_updting_coproc;
static struct app_events_callback m_evntcb_updted_coproc;
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event);

static esp_gatt_if_t m_gatts_if;
static uint16_t m_conn_id;

/* Settings Service Attribute Indexes */
enum {
    SETTINGS_IDX_SVC,

    SETTINGS_IDX_CHAR,
    SETTINGS_IDX_VAL,

	SETTINGS_IDX_NB,
};

/* Array of handles to each element */
static uint16_t handle_table_settings[SETTINGS_IDX_NB];

/* Settings Information Service UUID */
static const uint16_t m_settings_svc 		= 0x8040;

static const uint16_t m_settings_uuid 		= 0x8041;
static uint8_t m_settings_value[SETTINGS_VALUE_MAX_LEN]		= {0x00};
static uint32_t m_settings_len = 0;

/* Settings Service Database Description */
static const esp_gatts_attr_db_t gatt_db_settings_svc[SETTINGS_IDX_NB] =
{
		// Settings Service Declaration
		[SETTINGS_IDX_SVC] =
		{ { ESP_GATT_AUTO_RSP }, { ESP_UUID_LEN_16, (uint8_t*) &primary_service_uuid, ESP_GATT_PERM_READ,
				sizeof(uint16_t), sizeof(m_settings_svc), (uint8_t*) &m_settings_svc } },

		// Settings Characteristic Declaration
		[SETTINGS_IDX_CHAR] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
		CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write_notify } },

		// Settings Characteristic Value
		[SETTINGS_IDX_VAL] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &m_settings_uuid, ESP_GATT_PERM_WRITE_ENCRYPTED|ESP_GATT_PERM_READ_ENCRYPTED,
		GATTS_CHAR_VAL_LEN_MAX, sizeof(m_settings_value), (uint8_t *)m_settings_value } },
};

/* Callback data and functions */
static struct host_cmd_callback cb_settings;
struct cmd_read_cb {
	bool data_available;
	SemaphoreHandle_t mutex;
};
static struct cmd_read_cb cb_data = {
		.data_available = false,
};

void cb_handler_settings(struct host_cmd_callback *cb, uint32_t cmd, void *frame) {
	ESP_LOGI(TAG, "cb_handler_settings");

	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (fr->type != UART_M2M_FRAME_SINGLE_RESP) {
		ESP_LOGE(TAG, "Invalid Frame type");
		return;
	}

	if (fr->payload_len <= 0) {
		ESP_LOGE(TAG, "Invalid payload length");
		return;
	}

	/* copy the payload */
	char pl_excl[10] = {0x00};
	uint8_t pl_excl_len = sprintf(pl_excl, "%d%s", C20X_M2M_CMD_ID_SETTINGS, M2M_CMD_PAYLOAD_DELIM);

	m_settings_len = fr->payload_len - pl_excl_len;
	memcpy(m_settings_value, fr->payload + pl_excl_len, fr->payload_len);

	/* set data available variable to true */
	if (cb_data.mutex != NULL) {
		if (xSemaphoreTake(cb_data.mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			cb_data.data_available = true;
			xSemaphoreGive(cb_data.mutex);
		}
	}
}

static void cmd_read_register_callback() {
	host_cmds_add_callback(&cb_settings, cb_handler_settings, C20X_M2M_CMD_ID_SETTINGS);
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

static int send_ble_write_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint32_t len, uint8_t *data) {
	esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.len = len;
    rsp.attr_value.handle = param->write.handle;
    rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
    memcpy(rsp.attr_value.value, data, len);
    return esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, &rsp);
}

void gatts_settings_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {

	case ESP_GATTS_REG_EVT:
	{
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_settings_svc, gatts_if, SETTINGS_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_gatts_if = gatts_if;
	}
		break;

	case ESP_GATTS_READ_EVT:
	{
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_settings, SETTINGS_IDX_NB);

		/* settings value */
		if (ret == SETTINGS_IDX_VAL)
		{
			ESP_LOGI(TAG, "Read event for SETTINGS_IDX_VAL, %d", ret);
#if 0
			if (update_data_availability(cb_data.mutex, &cb_data.data_available, false) < 0)
				break;

			/* TODO: send setting value to host processor */
			char temp[] = "1004,18,?\n";
			ret = host_cmds_settings_write(temp, strlen(temp));

			if (wait_until_timeout(&cb_data.data_available) < 0)
				break;
#endif
			if (send_ble_response(gatts_if, param, m_settings_len, m_settings_value) == ESP_OK) {
				ESP_LOGI(TAG, "Read event send_ble_response success");
			} else {
				ESP_LOGE(TAG, "Read event send_ble_response failed");
			}
		}
	}
		break;

	case ESP_GATTS_WRITE_EVT:
	{
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_settings, SETTINGS_IDX_NB);

		if (ret == SETTINGS_IDX_VAL) {
			ESP_LOGI(TAG, "Write event for SETTINGS_IDX_VAL, %d", ret);
//			memcpy(m_settings_value, param->write.value, param->write.len);

			if (update_data_availability(cb_data.mutex, &cb_data.data_available, false) < 0)
				break;

			/* send setting value to host processor and wait for response */
			ret = host_cmds_settings_write(param->write.value, param->write.len);

			if (wait_until_timeout(&cb_data.data_available) < 0)
				break;

			ESP_LOGW(TAG, "Write event sending response %s", m_settings_value);
			if (send_ble_write_response(gatts_if, param, m_settings_len, m_settings_value) == ESP_OK) {
				ESP_LOGI(TAG, "Write event send_ble_write_response success");
			} else {
				ESP_LOGE(TAG, "Write event send_ble_write_response failed");
			}
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
		ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
		break;

	case ESP_GATTS_CONNECT_EVT:
	{
		ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
		m_conn_id = param->connect.conn_id;

		/* register to event we want to listen */
		app_events_add_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_add_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_add_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;

	case ESP_GATTS_DISCONNECT_EVT:
	{
		/* remove event callbacks */
		app_events_remove_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_remove_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_remove_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;

	case ESP_GATTS_CREAT_ATTR_TAB_EVT:
	{
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != SETTINGS_IDX_NB) {
			ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, SETTINGS_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			/* initialize callback data and register callback functions with host_cmds module */
			cb_data.mutex = xSemaphoreCreateMutex();
			if (cb_data.mutex != NULL) {
				cmd_read_register_callback();
			}

			memcpy(handle_table_settings, param->add_attr_tab.handles, sizeof(handle_table_settings));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_settings[SETTINGS_IDX_SVC]);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "settings service started successfully");
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
		break;
	case APP_EVENT_FW_UPDATING_COPROC:
		break;
	case APP_EVENT_FW_UPDATED_COPROC:
		break;
	default:
		break;
	}
}
