/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_services.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <c20x_m2m_cmds.h>
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

#include "host_cmds.h"
#include "host_cmds_callback.h"
#include "host_cmds_packet.h"
#include "comm_ble_common.h"
#include "comm_ble_blower_device_profile.h"
#include "comm_ble_device_info_service.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"comm_ble_devinf_svc"

#define PREPARE_BUF_MAX_SIZE        1024
#define SVC_INST_ID             	0

/* Array of handles to each element */
static uint16_t handle_table_devinf[IDX_DEVINF_NB];

/* Device Information Service UUID */
static const uint16_t m_device_info_svc = ESP_GATT_UUID_DEVICE_INFO_SVC;

/* Device Info UUIDs */
static const uint16_t m_device_info_manufacturer_uuid = ESP_GATT_UUID_MANU_NAME;
static const uint16_t m_device_info_model_num_uuid = ESP_GATT_UUID_MODEL_NUMBER_STR;
static const uint16_t m_device_info_serial_num_uuid = ESP_GATT_UUID_SERIAL_NUMBER_STR;
static const uint16_t m_device_info_fw_ver_uuid = ESP_GATT_UUID_FW_VERSION_STR;
static const uint16_t m_device_info_hw_ver_uuid = ESP_GATT_UUID_HW_VERSION_STR;
static const uint16_t m_device_info_sw_ver_uuid = ESP_GATT_UUID_SW_VERSION_STR;

/* Device Information Service Characteristics value arrays to be populated in the gatt server attribute database */
static uint8_t m_device_info_manufacturer_value[DEVICE_INFO_VALUE_MFR_NAME_LEN] 	= {0x00};
static uint8_t m_device_info_model_num_value[DEVICE_INFO_VALUE_MODEL_NUM_LEN]		= {0x00};
static uint8_t m_device_info_serial_num_value[DEVICE_INFO_VALUE_SERIAL_NUM_LEN]	= {0x00};
static uint8_t m_device_info_fw_ver_value[DEVICE_INFO_VALUE_FW_VERSION_LEN]		= {0x00};
static uint8_t m_device_info_hw_ver_value[DEVICE_INFO_VALUE_HW_VERSION_LEN]		= {0x00};
static uint8_t m_device_info_sw_ver_value[DEVICE_INFO_VALUE_SW_VERSION_LEN]		= {0x00};

/* Device Information Service Database Description */
static const esp_gatts_attr_db_t gatt_db_devinf_svc[IDX_DEVINF_NB] =
{
    // Service Declaration
    [IDX_DEVINF_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(m_device_info_svc), (uint8_t *)&m_device_info_svc}},

    /* Characteristic Declaration */
    [IDX_DEVINF_MFR_NAME_CHAR]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    /* Characteristic Value */
    [IDX_DEVINF_MFR_NAME_VALUE] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_manufacturer_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_manufacturer_value), (uint8_t *)m_device_info_manufacturer_value}},

    /* Characteristic Declaration */
    [IDX_DEVINF_MODEL_NUM_CHAR]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    /* Characteristic Value */
    [IDX_DEVINF_MODEL_NUM_VALUE]  =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_model_num_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_model_num_value), (uint8_t *)m_device_info_model_num_value}},

    /* Characteristic Declaration */
    [IDX_DEVINF_SERIAL_NUM_CHAR]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
    /* Characteristic Value */
    [IDX_DEVINF_SERIAL_NUM_VALUE]  =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_serial_num_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
      GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_serial_num_value), (uint8_t *)m_device_info_serial_num_value}},

	/* Characteristic Declaration */
	[IDX_DEVINF_FW_VERSION_CHAR]      =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
	  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
	/* Characteristic Value */
	[IDX_DEVINF_FW_VERSION_VALUE]  =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_fw_ver_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
	  GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_fw_ver_value), (uint8_t *)m_device_info_fw_ver_value}},

	/* Characteristic Declaration */
	[IDX_DEVINF_HW_VERSION_CHAR]      =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
	  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
	/* Characteristic Value */
	[IDX_DEVINF_HW_VERSION_VALUE]  =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_hw_ver_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
	  GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_hw_ver_value), (uint8_t *)m_device_info_hw_ver_value}},

	/* Characteristic Declaration */
	[IDX_DEVINF_SW_VERSION_CHAR]      =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
	  CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read}},
	/* Characteristic Value */
	[IDX_DEVINF_SW_VERSION_VALUE]  =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_device_info_sw_ver_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
	  GATTS_CHAR_VAL_LEN_MAX, sizeof(m_device_info_sw_ver_value), (uint8_t *)m_device_info_sw_ver_value}},
};

/* Callback data and functions */
static struct host_cmd_callback cb_fw_ver;
struct cmd_read_cb {
	bool data_available;
	SemaphoreHandle_t mutex;
};
static struct cmd_read_cb cb_data = {
		.data_available = false,
};

void cb_handler_fw_ver(struct host_cmd_callback *cb,
		uint32_t cmd,
		void *frame) {
/*
	struct host_cmd_packet_t *pac = (struct host_cmd_packet_t *) packet;

	ESP_LOGI(TAG, "cb_handler_fw_ver");
	ESP_LOGI(TAG, "pac.type = %d", pac->type);
	ESP_LOGI(TAG, "pac.sequence = %d", pac->sequence);
	ESP_LOGI(TAG, "pac.cmd_len = %d", pac->cmd_len);
	ESP_LOGI(TAG, "pac.cmd:");
	ESP_LOG_BUFFER_HEXDUMP(TAG, pac->cmd, pac->cmd_len, ESP_LOG_INFO);
	ESP_LOGI(TAG, "pac.status = %d", pac->status);
	ESP_LOGI(TAG, "pac.payload_len = %d", pac->payload_len);
	ESP_LOGI(TAG, "pac.payload:");
	ESP_LOG_BUFFER_HEXDUMP(TAG, pac->payload, pac->payload_len, ESP_LOG_INFO);

	if (pac->status == HOST_CMD_STATUS_OK) {
		memcpy(m_device_info_fw_ver_value, pac->payload, pac->payload_len);
	}
*/

	ESP_LOGI(TAG, "cb_handler_fw_ver");

	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	/* extract the command id */
	char *tok = strtok((char*)fr->payload, ",\n");
	if (tok == NULL)	return;

	uint16_t cmd_id = atoi(tok);
	if (cmd_id == C20X_M2M_CMD_DEVINFO_FWVER) {
		/* extract the value */
		tok = strtok(NULL, ",\n");
		if (tok == NULL)	return;

		int len = strlen(tok);
		memcpy(m_device_info_fw_ver_value, tok, len);
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
	host_cmds_add_callback(&cb_fw_ver, cb_handler_fw_ver, C20X_M2M_CMD_DEVINFO_FWVER);
}

static uint8_t find_char_and_desr_index(uint16_t handle) {
	uint8_t error = 0xff;

	for (int i = 0; i < IDX_DEVINF_NB; i++) {
		if (handle == handle_table_devinf[i]) {
			return i;
		}
	}
	return error;
}

void gatts_devinf_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_devinf_svc, gatts_if, IDX_DEVINF_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
		ESP_LOGI(TAG, "conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		char buf[DEVICE_INFO_VALUE_MAX_LEN] = { 0x00 };
		bool wait_timeout = false;

		esp_err_t ret = find_char_and_desr_index(param->read.handle);
		if (ret == IDX_DEVINF_MFR_NAME_VALUE) {
			ret = host_cmds_devinf_mfr_name_read(buf, DEVICE_INFO_VALUE_MFR_NAME_LEN);
		} else if (ret == IDX_DEVINF_MODEL_NUM_VALUE) {
			ret = host_cmds_devinf_model_num_read(buf, DEVICE_INFO_VALUE_MODEL_NUM_LEN);
		} else if (ret == IDX_DEVINF_SERIAL_NUM_VALUE) {
			ret = host_cmds_devinf_serial_num_read(buf, DEVICE_INFO_VALUE_SERIAL_NUM_LEN);
		} else if (ret == IDX_DEVINF_FW_VERSION_VALUE) {
			if (cb_data.mutex != NULL) {
				if (xSemaphoreTake(cb_data.mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
					cb_data.data_available = false;
					xSemaphoreGive(cb_data.mutex);
				} else {
					break;
				}
			}
			ret = host_cmds_devinf_fw_version_read(buf, DEVICE_INFO_VALUE_FW_VERSION_LEN);

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
			if (!wait_timeout)
				memcpy(buf, m_device_info_fw_ver_value, sizeof(m_device_info_fw_ver_value));

		} else if (ret == IDX_DEVINF_HW_VERSION_VALUE) {
			ret = host_cmds_devinf_hw_version_read(buf, DEVICE_INFO_VALUE_HW_VERSION_LEN);
		} else if (ret == IDX_DEVINF_SW_VERSION_VALUE) {
			ret = host_cmds_devinf_sw_version_read(buf, DEVICE_INFO_VALUE_SW_VERSION_LEN);
		}

		if (!wait_timeout) {
			esp_gatt_rsp_t rsp;
	        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
	        rsp.attr_value.handle = param->read.handle;
	        rsp.attr_value.len = strlen(buf);
	        memcpy(rsp.attr_value.value, buf, strlen(buf));
	        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
	                                    ESP_GATT_OK, &rsp);
		}
	}
		break;
	case ESP_GATTS_WRITE_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		break;
	case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
		break;
	case ESP_GATTS_MTU_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
		break;
	case ESP_GATTS_CONF_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
		break;
	case ESP_GATTS_START_EVT:
		ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status,
				param->start.service_handle);
		break;
	case ESP_GATTS_CONNECT_EVT:
		break;
	case ESP_GATTS_DISCONNECT_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != IDX_DEVINF_NB) {
			ESP_LOGE(TAG,
					"create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, IDX_DEVINF_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* initialize callback data and register callback functions with host_cmds module */
			cb_data.mutex = xSemaphoreCreateMutex();
			if (cb_data.mutex != NULL) {
				cmd_read_register_callback();
			}

			memcpy(handle_table_devinf, param->add_attr_tab.handles, sizeof(handle_table_devinf));
			esp_ble_gatts_start_service(handle_table_devinf[IDX_DEVINF_SVC]);
		}
		break;
	}
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
