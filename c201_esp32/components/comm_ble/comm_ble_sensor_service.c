/*
 * Copyright (c) 2022 Acme CPU
 *
 * comm_ble_sensor_service.c
 * Created on: 10-Aug-2022
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
#include "comm_ble_sensor_service.h"
#include "app_events.h"
#include "c20x_m2m_cmds.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"
#define TAG	"comm_ble_sensor_svc"

#define SVC_INST_ID             	0

/* App event related data */
static struct app_events_callback m_evntcb_toupdt_coproc;
static struct app_events_callback m_evntcb_updting_coproc;
static struct app_events_callback m_evntcb_updted_coproc;
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event);


typedef enum {
	TASK_SENSOR_GET=0,
	TASK_SENSOR_GETALL,
	TASK_NB
} SENSOR_TASK_IDX;
#define NUM_NOTIFY_TASKS		(TASK_NB)

struct sensor_task_control {
	TaskHandle_t ntf_task;
	bool stay_alive;
};

static struct sensor_task_control m_sen_task_ctrl[NUM_NOTIFY_TASKS] = {
	[TASK_SENSOR_GET] = {.ntf_task = NULL, .stay_alive = false},
};
static esp_gatt_if_t m_gatts_if;
static uint16_t m_conn_id;

static void task_sensor_get_notify( void * pvParameters );
static void task_sensor_getall_notify( void * pvParameters );

/* Sensor Service Attribute Indexes */
enum {
    SEN_IDX_SVC,

    SEN_IDX_LIST_CHAR,
    SEN_IDX_LIST_VAL,

    SEN_IDX_GET_CHAR,
    SEN_IDX_GET_VAL,
	SEN_IDX_GET_NTF_CFG,

	SEN_IDX_GETALL_CHAR,
    SEN_IDX_GETALL_VAL,
	SEN_IDX_GETALL_NTF_CFG,

	SEN_IDX_GETALL_INTV_CHAR,
    SEN_IDX_GETALL_INTV_VAL,

	SEN_IDX_NB,
};

/* Array of handles to each element */
static uint16_t handle_table_sensor[SEN_IDX_NB];

/* Sensor Service UUID */
static const uint16_t m_sensor_svc 				= 0x8050;
static const uint16_t m_sensor_list_uuid 		= 0x8051;
static const uint16_t m_sensor_get_uuid 		= 0x8052;
static uint8_t m_sensor_get_ccc[2]				= { 0x00, 0x00}; /* keep notifications disabled by default */
static const uint16_t m_sensor_getall_uuid 		= 0x8053;
static const uint16_t m_getall_intv_uuid 		= 0x8054;
static uint8_t m_sensor_getall_ccc[2]			= { 0x00, 0x00}; /* keep notifications disabled by default */
static uint32_t m_getall_interval				= 50;	/* interval in ms */

static uint8_t m_sensor_list[SENSOR_LIST_MAX_LEN]				= {0x00};
static uint8_t m_sensor_get_value[SENSOR_GET_VAL_MAX_LEN]		= {0x00};
static uint8_t m_sensor_getall_value[SENSOR_GETALL_VAL_MAX_LEN]	= {0x00};

static uint32_t m_sensor_list_len = 0;
static uint32_t m_sensor_get_len = 0;
static uint32_t m_sensor_getall_len = 0;


/* Sensor Service Database Description */
static const esp_gatts_attr_db_t gatt_db_sensor_svc[SEN_IDX_NB] =
{
		// Sensor Service Declaration
		[SEN_IDX_SVC] =
		{ { ESP_GATT_AUTO_RSP }, { ESP_UUID_LEN_16, (uint8_t*) &primary_service_uuid, ESP_GATT_PERM_READ,
		sizeof(uint16_t), sizeof(m_sensor_svc), (uint8_t*) &m_sensor_svc } },

		// Sensor List Characteristic Declaration
		[SEN_IDX_LIST_CHAR] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
		CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read } },

		// Sensor List Characteristic Value
		[SEN_IDX_LIST_VAL] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &m_sensor_list_uuid, ESP_GATT_PERM_READ_ENCRYPTED,
		GATTS_CHAR_VAL_LEN_MAX, sizeof(m_sensor_list), (uint8_t*) m_sensor_list } },

		// Sensor Get Characteristic Declaration
		[SEN_IDX_GET_CHAR] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
		CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write_notify } },

		// Sensor Get Characteristic Value
		[SEN_IDX_GET_VAL] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &m_sensor_get_uuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
		GATTS_CHAR_VAL_LEN_MAX, sizeof(m_sensor_get_value), (uint8_t*) m_sensor_get_value } },

		// Sensor Get Characteristic - Client Characteristic Configuration Descriptor
		[SEN_IDX_GET_NTF_CFG] =
		{ { ESP_GATT_AUTO_RSP }, { ESP_UUID_LEN_16, (uint8_t*) &character_client_config_uuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
		sizeof(uint16_t), sizeof(m_sensor_get_ccc), (uint8_t*) m_sensor_get_ccc } },

		// Sensor Get All Characteristic Declaration
		[SEN_IDX_GETALL_CHAR] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
		CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write_notify } },

		// Sensor Get All Characteristic Value
		[SEN_IDX_GETALL_VAL] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &m_sensor_getall_uuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
		GATTS_CHAR_VAL_LEN_MAX, sizeof(m_sensor_getall_value), (uint8_t*) m_sensor_getall_value } },

		// Sensor Get All Characteristic - Client Characteristic Configuration Descriptor
		[SEN_IDX_GETALL_NTF_CFG] =
		{ { ESP_GATT_AUTO_RSP }, { ESP_UUID_LEN_16, (uint8_t*) &character_client_config_uuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
		sizeof(uint16_t), sizeof(m_sensor_getall_ccc), (uint8_t*) m_sensor_getall_ccc } },

		// Sensor Getall Interval Characteristic Declaration
		[SEN_IDX_GETALL_INTV_CHAR] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ|ESP_GATT_PERM_WRITE,
		CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read_write_notify } },

		// Sensor Get All Characteristic Value
		[SEN_IDX_GETALL_INTV_VAL] =
		{ { ESP_GATT_RSP_BY_APP }, { ESP_UUID_LEN_16, (uint8_t*) &m_getall_intv_uuid, ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED,
		GATTS_CHAR_VAL_LEN_MAX, sizeof(m_getall_interval), (uint8_t*) &m_getall_interval } },
};

/* Callback data and functions */
typedef enum {
	CB_LIST=0,
	CB_GET,
	CB_GET_ALL,
	CB_NB
} READ_CB;
#define NUM_READ_CB		(CB_NB)

static struct host_cmd_callback cb_sensor_get_list;			/* sensor get list callback object */
static struct host_cmd_callback cb_sensor_get_value;		/* sensor get value callback object */
static struct host_cmd_callback cb_sensor_getall_value;		/* sensor getall value callback object */

struct cmd_read_cb {
	bool data_available;
	SemaphoreHandle_t cmd_mutex;
};
static struct cmd_read_cb m_cb_data[NUM_READ_CB];

void cb_handler_sensor_list(struct host_cmd_callback *cb, uint32_t cmd, void *frame) {
	ESP_LOGI(TAG, "cb_handler_sensor_list");

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
	uint8_t pl_excl_len = sprintf(pl_excl, "%d%s", C20X_M2M_CMD_ID_SENSOR_LIST, M2M_CMD_PAYLOAD_DELIM);

	m_sensor_list_len = fr->payload_len - pl_excl_len;
	memcpy(m_sensor_list, fr->payload + pl_excl_len, fr->payload_len-1 /* exclude the terminating character */);

	/* set data available variable to true */
	if (m_cb_data[CB_LIST].cmd_mutex != NULL) {
		if (xSemaphoreTake(m_cb_data[CB_LIST].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_LIST].data_available = true;
			xSemaphoreGive(m_cb_data[CB_LIST].cmd_mutex);
		}
	}
}

void cb_handler_sensor_get(struct host_cmd_callback *cb, uint32_t cmd, void *frame) {
	ESP_LOGI(TAG, "cb_handler_sensor_get");

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
	uint8_t pl_excl_len = sprintf(pl_excl, "%d%s", C20X_M2M_CMD_ID_SENSOR_GET, M2M_CMD_PAYLOAD_DELIM);

	m_sensor_get_len = fr->payload_len - pl_excl_len;
	memcpy(m_sensor_get_value, fr->payload + pl_excl_len, fr->payload_len-1 /* exclude the terminating character */);

	/* set data available variable to true */
	if (m_cb_data[CB_GET].cmd_mutex != NULL) {
		if (xSemaphoreTake(m_cb_data[CB_GET].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_GET].data_available = true;
			xSemaphoreGive(m_cb_data[CB_GET].cmd_mutex);
		}
	}
}

void cb_handler_sensor_getall(struct host_cmd_callback *cb, uint32_t cmd, void *frame) {
	ESP_LOGI(TAG, "cb_handler_sensor_getall");

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
	uint8_t pl_excl_len = sprintf(pl_excl, "%d%s", C20X_M2M_CMD_ID_SENSOR_GET, M2M_CMD_PAYLOAD_DELIM);

	m_sensor_getall_len = fr->payload_len - pl_excl_len;
	memcpy(m_sensor_getall_value, fr->payload + pl_excl_len, fr->payload_len-1 /* exclude the terminating character */);

	/* set data available variable to true */
	if (m_cb_data[CB_GET_ALL].cmd_mutex != NULL) {
		if (xSemaphoreTake(m_cb_data[CB_GET_ALL].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_GET_ALL].data_available = true;
			xSemaphoreGive(m_cb_data[CB_GET_ALL].cmd_mutex);
		}
	}
}

static void cmd_read_register_callback() {
	m_cb_data[CB_LIST].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_sensor_get_list, cb_handler_sensor_list, C20X_M2M_CMD_ID_SENSOR_LIST);

	m_cb_data[CB_GET].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_sensor_get_value, cb_handler_sensor_get, C20X_M2M_CMD_ID_SENSOR_GET);

	m_cb_data[CB_GET_ALL].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_sensor_getall_value, cb_handler_sensor_getall, C20X_M2M_CMD_ID_SENSOR_GETALL);
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

#define DATA_AVAILABLE_LOOP_DELAY_SENSOR	(1 / portTICK_PERIOD_MS)
static int wait_until_timeout(bool *pvar) {
//	bool wait_timeout = false;
	uint32_t delay = 0;
	while (!(*pvar)) {
		vTaskDelay(DATA_AVAILABLE_LOOP_DELAY_SENSOR);
		delay += DATA_AVAILABLE_LOOP_DELAY_SENSOR;
		if (delay > DATA_AVAILABLE_TIMEOUT) {
//			wait_timeout = true;
			return -1;
		}
	}
	return 0;
}

static int send_ble_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint32_t len, void *data) {
	esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = len;
    memcpy(rsp.attr_value.value, data, len);
    return esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

static int send_ble_write_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint32_t len, void *data) {
	esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.len = len;
    rsp.attr_value.handle = param->write.handle;
    rsp.attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
    memcpy(rsp.attr_value.value, data, len);
    return esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, &rsp);
}

void gatts_sensor_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_sensor_svc, gatts_if, SEN_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_gatts_if = gatts_if;
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_sensor, SEN_IDX_NB);

		/* sensor list */
		if (ret == SEN_IDX_LIST_VAL)
		{
			ESP_LOGI(TAG, "Read event for SEN_IDX_LIST_VAL, %d", ret);

			if (update_data_availability(m_cb_data[CB_LIST].cmd_mutex, &m_cb_data[CB_LIST].data_available, false) < 0)
				break;
			ret = host_cmds_sensor_list_get();	/* Get the sensor list from host processor */
			if (wait_until_timeout(&m_cb_data[CB_LIST].data_available) < 0)
				break;
			if (send_ble_response(gatts_if, param, m_sensor_list_len, m_sensor_list) == ESP_OK) {
				ESP_LOGI(TAG, "Read event send_ble_response success");
			} else {
				ESP_LOGE(TAG, "Read event send_ble_response failed");
			}
		}
		/* sensor get */
		else if (ret == SEN_IDX_GET_VAL)
		{
			ESP_LOGI(TAG, "Read event for SEN_IDX_GET_VAL, %d", ret);
#if 0
			if (update_data_availability(m_cb_data[CB_GET].cmd_mutex, &m_cb_data[CB_GET].data_available, false) < 0)
				break;
//			ret = host_cmds_blower_voltage_read();	/* Get the sensor voltage from host processor */
			if (wait_until_timeout(&m_cb_data[CB_GET].data_available) < 0)
				break;
#endif
			if (send_ble_response(gatts_if, param, m_sensor_get_len, m_sensor_get_value) == ESP_OK) {
				ESP_LOGI(TAG, "Read event send_ble_response success");
			} else {
				ESP_LOGE(TAG, "Read event send_ble_response failed");
			}
		}
		/* sensor get all */
		else if (ret == SEN_IDX_GETALL_VAL)
		{
			ESP_LOGI(TAG, "Read event for SEN_IDX_GETALL_VAL, %d", ret);
			if (update_data_availability(m_cb_data[CB_GET_ALL].cmd_mutex, &m_cb_data[CB_GET_ALL].data_available, false) < 0)
				break;
			ret = host_cmds_sensor_value_getall();	/* Get the sensor values from host processor */
			if (wait_until_timeout(&m_cb_data[CB_GET_ALL].data_available) < 0)
				break;
			if (send_ble_response(gatts_if, param, m_sensor_getall_len, m_sensor_getall_value) == ESP_OK) {
				ESP_LOGI(TAG, "Read event send_ble_response success");
			} else {
				ESP_LOGE(TAG, "Read event send_ble_response failed");
			}
		}
		/* sensor get all interval */
		else if (SEN_IDX_GETALL_INTV_VAL)
		{
			ESP_LOGI(TAG, "Read event for SEN_IDX_GETALL_INTV_VAL, %d", ret);
			if (send_ble_response(gatts_if, param, sizeof(m_getall_interval), &m_getall_interval) == ESP_OK) {
				ESP_LOGI(TAG, "Read event send_ble_response success");
			} else {
				ESP_LOGE(TAG, "Read event send_ble_response failed");
			}
		}
		break;
	}
	case ESP_GATTS_WRITE_EVT:
	{
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_sensor, SEN_IDX_NB);

		switch (ret) {
		case SEN_IDX_GET_VAL:
		{
			ESP_LOGI(TAG, "Write event for SEN_IDX_GET_VAL, %d", ret);
			if (update_data_availability(m_cb_data[CB_GET].cmd_mutex, &m_cb_data[CB_GET].data_available, false) < 0)
				break;

			/* send sensor channel and id to host processor and wait for response */
			uint8_t sens_info[2] = {param->write.value[0], param->write.value[1]};
			ret = host_cmds_sensor_value_getone(sens_info);

			if (wait_until_timeout(&m_cb_data[CB_GET].data_available) < 0)
				break;

			ESP_LOGW(TAG, "Write event sending response:");
			ESP_LOG_BUFFER_HEXDUMP(TAG, m_sensor_get_value, sizeof(m_sensor_get_value), ESP_LOG_WARN);

			if (send_ble_write_response(gatts_if, param, sizeof(m_sensor_get_value), m_sensor_get_value) == ESP_OK) {
				ESP_LOGI(TAG, "Write event send_ble_write_response success");
			} else {
				ESP_LOGE(TAG, "Write event send_ble_write_response failed");
			}
			break;
		}
		case SEN_IDX_GETALL_INTV_VAL:
		{
			ESP_LOGI(TAG, "Write event for SEN_IDX_GETALL_INTV_VAL, %d", ret);
			memcpy(&m_getall_interval, param->write.value, param->write.len);
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
			break;
		}
		case SEN_IDX_GET_NTF_CFG:
		{
			ESP_LOGI(TAG, "Write event for SEN_IDX_GET_NTF_CFG, %d", ret);
			m_sensor_get_ccc[0] = param->write.value[0];
			m_sensor_get_ccc[1] = param->write.value[1];
			ESP_LOGW(TAG, "len=%d,ccc[0]=0x%x ccc[1]=0x%x", param->write.len, m_sensor_get_ccc[0],m_sensor_get_ccc[1]);

			/* if notifications are enabled (ccc_val[0][1]=0x1,0x0), then resume the notification task.
			 * Suspend happens inside the notification task by checking if m_sensor_get_ccc is disabled */
			if ((m_sensor_get_ccc[0] == 0x01) && (m_sensor_get_ccc[1] == 0x00)) {
				vTaskResume(m_sen_task_ctrl[TASK_SENSOR_GET].ntf_task);
			}
//			send_ble_response(gatts_if, param, param->write.len, param->write.value);
			break;
		}
		case SEN_IDX_GETALL_NTF_CFG:
		{
			ESP_LOGI(TAG, "Write event for SEN_IDX_GETALL_NTF_CFG, %d", ret);
			m_sensor_getall_ccc[0] = param->write.value[0];
			m_sensor_getall_ccc[1] = param->write.value[1];
			ESP_LOGW(TAG, "len=%d,ccc[0]=0x%x ccc[1]=0x%x", param->write.len, m_sensor_getall_ccc[0], m_sensor_getall_ccc[1]);

			/* if notifications are enabled (ccc_val[0][1]=0x1,0x0), then resume the notification task.
			 * Suspend happens inside the notification task by checking if m_sensor_get_ccc is disabled */
			if ((m_sensor_getall_ccc[0] == 0x01) && (m_sensor_getall_ccc[1] == 0x00)) {
				vTaskResume(m_sen_task_ctrl[TASK_SENSOR_GETALL].ntf_task);
			}
//			send_ble_response(gatts_if, param, param->write.len, param->write.value);
			break;
		}
		}	/* switch (ret) */
		break;
	}
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

		/* create a sensor get notification task */
		BaseType_t xret = xTaskCreate(task_sensor_get_notify, "sensor_get", COMM_BLE_TASK_STACK_SENSOR_GET, NULL,
				COMM_BLE_TASK_PRIO_SENSOR_GET, &m_sen_task_ctrl[TASK_SENSOR_GET].ntf_task);
		if (xret == pdPASS) {
			m_sen_task_ctrl[TASK_SENSOR_GET].stay_alive = true;
			ESP_LOGI(TAG, "sensor get notification task created successfully");
		} else {
			ESP_LOGE(TAG, "sensor get notification task creation failed");
		}

		/* create a sensor get all notification task */
		xret = xTaskCreate(task_sensor_getall_notify, "sensor_getall", COMM_BLE_TASK_STACK_SENSOR_GETALL, NULL,
				COMM_BLE_TASK_PRIO_SENSOR_GETALL, &m_sen_task_ctrl[TASK_SENSOR_GETALL].ntf_task);
		if (xret == pdPASS) {
			m_sen_task_ctrl[TASK_SENSOR_GETALL].stay_alive = true;
			ESP_LOGI(TAG, "sensor getall notification task created successfully");
		} else {
			ESP_LOGE(TAG, "sensor getall notification task creation failed");
		}

		/* register to event we want to listen */
		app_events_add_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_add_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_add_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
		break;
	}
	case ESP_GATTS_DISCONNECT_EVT:
	{
		/* call resume in case the notification task is suspended */
		vTaskResume(m_sen_task_ctrl[TASK_SENSOR_GET].ntf_task);
		vTaskResume(m_sen_task_ctrl[TASK_SENSOR_GETALL].ntf_task);

		/* tell the task to delete itself */
		m_sen_task_ctrl[TASK_SENSOR_GET].stay_alive = false;
		m_sen_task_ctrl[TASK_SENSOR_GETALL].stay_alive = false;

		/* remove event callbacks */
		app_events_remove_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_remove_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_remove_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
		break;
	}
	case ESP_GATTS_CREAT_ATTR_TAB_EVT:
	{
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != SEN_IDX_NB) {
			ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) doesn't equal to IDX_NB(%d)", param->add_attr_tab.num_handle, SEN_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d", param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			cmd_read_register_callback();

			memcpy(handle_table_sensor, param->add_attr_tab.handles, sizeof(handle_table_sensor));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_sensor[SEN_IDX_SVC]);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "sensor service started successfully");
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
		vTaskSuspend(m_sen_task_ctrl[TASK_SENSOR_GET].ntf_task);
		vTaskSuspend(m_sen_task_ctrl[TASK_SENSOR_GETALL].ntf_task);
		break;
	case APP_EVENT_FW_UPDATING_COPROC:
		break;
	case APP_EVENT_FW_UPDATED_COPROC:
		break;
	default:
		break;
	}
}

static void task_sensor_get_notify( void * pvParameters ) {
	esp_err_t ret = 0;

	while (1) {

		/* if the connection is not alive then kill this task */
		if (!m_sen_task_ctrl[TASK_SENSOR_GET].stay_alive) {
			break;
		}

		/* if all notifications are disabled, then suspend this task */
		if ((m_sensor_get_ccc[0] == 0x00) && (m_sensor_get_ccc[1] == 0x00)) {
			ESP_LOGW(TAG, "suspending sensor get notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from sensor get notification task");
		}

		/* Check if sensor get notification is enabled */
		if (m_cb_data[CB_GET].data_available && (m_sensor_get_ccc[0] == 0x01) && (m_sensor_get_ccc[1] == 0x00)) {
			ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id, handle_table_sensor[SEN_IDX_GET_VAL],
					m_sensor_get_len, (uint8_t*) m_sensor_get_value, false);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
				m_cb_data[CB_GET].data_available = false;
			} else {
				ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
			}
		} else {
			ESP_LOGW(TAG, "suspending sensor get notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from sensor get notification task");
		}
	}

	ESP_LOGW(TAG, "deleting sensor get notification task ...");
	vTaskDelete(NULL);
}

static void task_sensor_getall_notify( void * pvParameters ) {
	esp_err_t ret = 0;

	while (1) {
		/* if the connection is not alive then kill this task */
		if (!m_sen_task_ctrl[TASK_SENSOR_GETALL].stay_alive) {
			break;
		}

		/* if all notifications are disabled, then suspend this task */
		if ((m_sensor_getall_ccc[0] == 0x00) && (m_sensor_getall_ccc[1] == 0x00)) {
			ESP_LOGW(TAG, "suspending sensor getall notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from sensor getall notification task");
		}

		/* Check if notifications are enabled */
		if ((m_sensor_getall_ccc[0] == 0x01) && (m_sensor_getall_ccc[1] == 0x00)) {
			/* Read the sensor value from host processor */

			if (update_data_availability(m_cb_data[CB_GET_ALL].cmd_mutex, &m_cb_data[CB_GET_ALL].data_available, false) < 0)
				continue;
			ret = host_cmds_sensor_value_getall();	/* Get the sensor values from host processor */
			ret = wait_until_timeout(&m_cb_data[CB_GET_ALL].data_available);
			if (!ret) {
				ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id, handle_table_sensor[SEN_IDX_GETALL_VAL],
						m_sensor_getall_len, (uint8_t*) m_sensor_getall_value, false);
				if (ret == ESP_OK) {
					ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
				} else {
					ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
				}
			} else {
				ESP_LOGE(TAG, "ERROR !!!");
			}
		}

		vTaskDelay(m_getall_interval / portTICK_PERIOD_MS);
	}
	ESP_LOGW(TAG, "deleting sensor getall notification task ...");
	vTaskDelete(NULL);
}
