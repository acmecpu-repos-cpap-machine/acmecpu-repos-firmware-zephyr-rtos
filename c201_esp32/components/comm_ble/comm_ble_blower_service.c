/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_blower_service.c
 * Created on: 27-Apr-2021
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
#include "comm_ble_blower_service.h"
#include "app_events.h"
#include "c20x_m2m_cmds.h"

//#include "m2m_frame.h"
#include "lib_m2m_frame.h"

#define TAG	"comm_ble_blower_svc"

#define SVC_INST_ID             	0

/* App event related data */
static struct app_events_callback m_evntcb_toupdt_coproc;
static struct app_events_callback m_evntcb_updting_coproc;
static struct app_events_callback m_evntcb_updted_coproc;
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event);

typedef enum {
	TASK_VOLTAGE=0,
	TASK_SPEED,
	TASK_NB
} BLOWER_TASK_IDX;
#define NUM_NOTIFY_TASKS		(TASK_NB)

struct blower_task_control {
	TaskHandle_t ntf_task;
	bool stay_alive;
};
static struct blower_task_control m_blw_task_ctrl[NUM_NOTIFY_TASKS] = {
	[TASK_VOLTAGE] = {.ntf_task = NULL, .stay_alive = false},
	[TASK_SPEED] = {.ntf_task = NULL, .stay_alive = false},
};

esp_gatt_if_t m_gatts_if;
uint16_t m_conn_id;

static void task_blower_voltage_notify( void * pvParameters );
static void task_blower_speed_notify( void * pvParameters );

/* Blower Service Attribute Indexes */
enum {
    BLW_IDX_SVC,

    BLW_IDX_STATUS_CHAR,
    BLW_IDX_STATUS_VAL,

    BLW_IDX_VOLTAGE_CHAR,
    BLW_IDX_VOLTAGE_VAL,
	BLW_IDX_VOLTAGE_NTF_CFG,

    BLW_IDX_DUTY_CHAR,
    BLW_IDX_DUTY_VAL,

	BLW_IDX_SPEED_CHAR,
    BLW_IDX_SPEED_VAL,
	BLW_IDX_SPEED_NTF_CFG,

    BLW_IDX_NB,
};

/* Array of handles to each element */
static uint16_t handle_table_blower[BLW_IDX_NB];

/* Blower Service UUID and values */
struct blower_values {
	uint8_t blower_status;
	uint32_t blower_mvolts;
	uint8_t blower_duty;
	uint32_t blower_speed;
};
static struct blower_values m_blw_val = {0, 0, 0, 0};

static const uint16_t m_blower_svc 						= 0x8001;

static const uint16_t m_blower_status_uuid 				= 0x8101;

static const uint16_t m_blower_voltage_uuid				= 0x8102;
static uint8_t m_blower_voltage_ccc[2]					= { 0x00, 0x00}; /* keep notifications disabled by default */

static const uint16_t m_blower_duty_uuid				= 0x8103;

static const uint16_t m_blower_speed_uuid				= 0x8104;
static uint8_t m_blower_speed_ccc[2]					= { 0x00, 0x00}; /* keep notifications disabled by default */

/* Blower Service Database Description */
static const esp_gatts_attr_db_t gatt_db_blower_svc[BLW_IDX_NB] =
{
    // Blower Service Declaration
    [BLW_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
    		sizeof(uint16_t), sizeof(m_blower_svc), (uint8_t *)&m_blower_svc}},

    // Blower ON/OFF Characteristic Declaration
    [BLW_IDX_STATUS_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    // Blower ON/OFF Characteristic Value
    [BLW_IDX_STATUS_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_blower_status_uuid, ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM,
    		sizeof(uint8_t),sizeof(uint8_t), &m_blw_val.blower_status}},

	// Blower Voltage Characteristic Declaration
	[BLW_IDX_VOLTAGE_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

	// Blower Voltage Characteristic Value
	[BLW_IDX_VOLTAGE_VAL] =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_blower_voltage_uuid, ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM,
			sizeof(uint32_t),sizeof(uint32_t), (uint8_t *)&m_blw_val.blower_mvolts}},

    // Blower Voltage Characteristic - Client Characteristic Configuration Descriptor
    [BLW_IDX_VOLTAGE_NTF_CFG] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		sizeof(uint16_t),sizeof(m_blower_voltage_ccc), (uint8_t *)m_blower_voltage_ccc}},

	// Blower Duty Characteristic Declaration
	[BLW_IDX_DUTY_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_WRITE,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

	// Blower Duty Characteristic Value
	[BLW_IDX_DUTY_VAL] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&m_blower_duty_uuid, ESP_GATT_PERM_WRITE_ENC_MITM,
			sizeof(uint8_t),sizeof(uint8_t), &m_blw_val.blower_duty}},

	// Blower Speed Characteristic Declaration
	[BLW_IDX_SPEED_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

	// Blower Speed Characteristic Value
	[BLW_IDX_SPEED_VAL] =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_blower_speed_uuid, ESP_GATT_PERM_READ_ENC_MITM,
			sizeof(uint32_t),sizeof(uint32_t), (uint8_t *)&m_blw_val.blower_speed}},

	// Blower Speed Characteristic - Client Characteristic Configuration Descriptor
	[BLW_IDX_SPEED_NTF_CFG] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
			sizeof(uint16_t),sizeof(m_blower_speed_ccc), (uint8_t *)m_blower_speed_ccc}},
};

/* Callback data and functions */
typedef enum {
	CB_STATUS=0,
	CB_VOLTS_MV,
	CB_SPEED,
	CB_NB
} READ_CB;
#define NUM_READ_CB		(CB_NB)

static struct host_cmd_callback cb_blower_get_status;		/* blower status get callback object */
static struct host_cmd_callback cb_blower_get_volts_mv;		/* blower voltage get callback object */
static struct host_cmd_callback cb_blower_get_speed;		/* blower speed get callback object */
//static struct host_cmd_packet_t m_packet;					/* global packet object */
//static SemaphoreHandle_t m_packet_mutex;					/* mutex to protect the global packet object */
struct cmd_read_cb {
	bool data_available;
//	char cmd[HOST_CMD_COMMAND_SIZE_MAX];
	SemaphoreHandle_t cmd_mutex;
};
static struct cmd_read_cb m_cb_data[NUM_READ_CB] = {
		[CB_STATUS] = {
				.data_available = false,
//				.cmd = CMD_BLOWER_STATUS_GET
		},
		[CB_VOLTS_MV] = {
				.data_available = false,
//				.cmd = CMD_BLOWER_GET_VOLTS_MV
		},
		[CB_SPEED] = {
				.data_available = false,
//				.cmd = CMD_BLOWER_GET_SPEED_HZ
		},
};

void cb_handler_blower_cmds(struct host_cmd_callback *cb,
		uint32_t cmd,
		void *frame) {
//	struct host_cmd_packet_t *pac = (struct host_cmd_packet_t *) packet;

	ESP_LOGI(TAG, "cb_handler_blower_cmds");
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
#if 0
	/* As there are multiple consumers of this data we copy the entire packet here
	 * and perform tasks in other threads */
	if (m_packet_mutex != NULL) {
		if (xSemaphoreTake(m_packet_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			memcpy(&m_packet, pac, sizeof(struct host_cmd_packet_t));
			xSemaphoreGive(m_packet_mutex);
		}
	}

	/* set the appropriate data available variable to true */
	for (int i=0; i<NUM_READ_CB; i++) {
		if (!strcmp((char*)m_packet.cmd, m_cb_data[i].cmd)) {
			if (xSemaphoreTake(m_cb_data[i].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
				m_cb_data[i].data_available = true;
				xSemaphoreGive(m_cb_data[i].cmd_mutex);
			}
		}
	}

/*
	if (m_cb_data.cmd_mutex != NULL) {
		if (xSemaphoreTake(m_cb_data.cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			memcpy(m_cb_data.cmd, pac->cmd, pac->cmd_len);
			m_cb_data.data_available = true;
			xSemaphoreGive(m_cb_data.cmd_mutex);
		}
	}
*/
#endif

	struct m2m_frame_t *fr = (struct m2m_frame_t *) frame;

	if (fr->type != UART_M2M_FRAME_SINGLE_RESP) {
		ESP_LOGE(TAG, "Invalid Frame type");
		return;
	}

	/* extract the command id */
	char *tok = strtok((char*)fr->payload, ",\n");
	if (tok == NULL)	return;

	uint16_t cmd_id = atoi(tok);
	tok = strtok(NULL, ",\n");

	switch (cmd_id) {
	case C20X_M2M_CMD_BLOWER_STATE:
		m_blw_val.blower_status = atoi(tok);
		if (xSemaphoreTake(m_cb_data[CB_STATUS].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_STATUS].data_available = true;
			xSemaphoreGive(m_cb_data[CB_STATUS].cmd_mutex);
		}
		break;
	case C20X_M2M_CMD_BLOWER_VOLT_MV:
		m_blw_val.blower_mvolts = atoi(tok);
		if (xSemaphoreTake(m_cb_data[CB_VOLTS_MV].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_VOLTS_MV].data_available = true;
			xSemaphoreGive(m_cb_data[CB_VOLTS_MV].cmd_mutex);
		}
		break;
	case C20X_M2M_CMD_BLOWER_SPEED_RPM:
		m_blw_val.blower_speed = atoi(tok);
		if (xSemaphoreTake(m_cb_data[CB_SPEED].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
			m_cb_data[CB_SPEED].data_available = true;
			xSemaphoreGive(m_cb_data[CB_SPEED].cmd_mutex);
		}
		break;
	}
}

static void cmd_read_register_callback() {
	m_cb_data[CB_STATUS].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_blower_get_status, cb_handler_blower_cmds, C20X_M2M_CMD_BLOWER_STATE);

	m_cb_data[CB_VOLTS_MV].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_blower_get_volts_mv, cb_handler_blower_cmds, C20X_M2M_CMD_BLOWER_VOLT_MV);

	m_cb_data[CB_SPEED].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_blower_get_speed, cb_handler_blower_cmds, C20X_M2M_CMD_BLOWER_SPEED_RPM);
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

static int send_ble_response(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param, uint32_t len, void *data) {
	esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = len;
    memcpy(rsp.attr_value.value, data, len);
    return esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
}

void gatts_blower_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_blower_svc, gatts_if, BLW_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_gatts_if = gatts_if;
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_blower, BLW_IDX_NB);

		/* blower state */
		if (ret == BLW_IDX_STATUS_VAL)
		{
			ESP_LOGI(TAG, "Read event for BLW_IDX_STATUS_VAL, %d", ret);

			if (update_data_availability(m_cb_data[CB_STATUS].cmd_mutex,
					&m_cb_data[CB_STATUS].data_available, false) < 0)
				break;
			ret = host_cmds_blower_status_read();	/* Get the blower status from host processor */
			if (wait_until_timeout(&m_cb_data[CB_STATUS].data_available) < 0)
				break;
//			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
			send_ble_response(gatts_if, param, sizeof(m_blw_val.blower_status), &m_blw_val.blower_status);
		}
		/* blower voltage */
		else if (ret == BLW_IDX_VOLTAGE_VAL)
		{
			ESP_LOGI(TAG, "Read event for BLW_IDX_VOLTAGE_VAL, %d", ret);

			if (update_data_availability(m_cb_data[CB_VOLTS_MV].cmd_mutex,
					&m_cb_data[CB_VOLTS_MV].data_available, false) < 0)
				break;
			ret = host_cmds_blower_voltage_read();	/* Get the blower voltage from host processor */
			if (wait_until_timeout(&m_cb_data[CB_VOLTS_MV].data_available) < 0)
				break;
//			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
			send_ble_response(gatts_if, param, sizeof(m_blw_val.blower_mvolts), &m_blw_val.blower_mvolts);
		}
		/* blower speed */
		else if (ret == BLW_IDX_SPEED_VAL)
		{
			ESP_LOGI(TAG, "Read event for BLW_IDX_SPEED_VAL, %d", ret);
			if (update_data_availability(m_cb_data[CB_SPEED].cmd_mutex,
					&m_cb_data[CB_SPEED].data_available, false) < 0)
				break;
			ret = host_cmds_blower_speed_hz_read();	/* Get the blower speed from host processor */
			if (wait_until_timeout(&m_cb_data[CB_SPEED].data_available) < 0)
				break;
//			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
			send_ble_response(gatts_if, param, sizeof(m_blw_val.blower_speed), &m_blw_val.blower_speed);
		}
	}
		break;
	case ESP_GATTS_WRITE_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_blower, BLW_IDX_NB);

		switch (ret) {
		case BLW_IDX_STATUS_VAL: {
			ESP_LOGI(TAG, "Write event for BLW_IDX_STATUS_VAL, %d", ret);
			memcpy(&m_blw_val.blower_status, param->write.value, param->write.len);
			ret = host_cmds_blower_status_write(m_blw_val.blower_status);
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case BLW_IDX_VOLTAGE_VAL: {
			ESP_LOGI(TAG, "Write event for BLW_IDX_VOLTAGE_VAL, %d", ret);
			memcpy(&m_blw_val.blower_mvolts, param->write.value, param->write.len);
			ret = host_cmds_blower_voltage_write(m_blw_val.blower_mvolts);
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case BLW_IDX_VOLTAGE_NTF_CFG: {
			ESP_LOGI(TAG, "Write event for BLW_IDX_VOLTAGE_NTF_CFG, %d", ret);
			m_blower_voltage_ccc[0] = param->write.value[0];
			m_blower_voltage_ccc[1] = param->write.value[1];
			ESP_LOGW(TAG, "len=%d,ccc[0]=0x%x ccc[1]=0x%x", param->write.len, m_blower_voltage_ccc[0],
					m_blower_voltage_ccc[1]);

			/* if notifications are enabled (ccc_val[0][1]=0x1,0x0), then resume the notification task.
			 * Suspend happens inside the notification task by checking if both
			 *  m_blower_voltage_ccc and m_blower_speed_ccc are disabled value */
			if ((m_blower_voltage_ccc[0] == 0x01) && (m_blower_voltage_ccc[1] == 0x00)) {
				vTaskResume(m_blw_task_ctrl[TASK_VOLTAGE].ntf_task);
			}
//			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case BLW_IDX_DUTY_VAL: {
			ESP_LOGI(TAG, "Write event for BLW_IDX_DUTY_VAL, %d", ret);
			memcpy(&m_blw_val.blower_duty, param->write.value, param->write.len);
			ret = host_cmds_blower_duty_write(m_blw_val.blower_duty);
//			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case BLW_IDX_SPEED_NTF_CFG: {
			ESP_LOGI(TAG, "Write event for BLW_IDX_SPEED_NTF_CFG, %d", ret);
			m_blower_speed_ccc[0] = param->write.value[0];
			m_blower_speed_ccc[1] = param->write.value[1];
			ESP_LOGW(TAG, "len=%d,ccc[0]=0x%x ccc[1]=0x%x", param->write.len, m_blower_speed_ccc[0],
					m_blower_speed_ccc[1]);

			/* if notifications are enabled (ccc_val[0][1]=0x1,0x0), then resume the notification task.
			 * Suspend happens inside the notification task by checking if both
			 *  m_blower_voltage_ccc and m_blower_speed_ccc are disabled value */
			if ((m_blower_speed_ccc[0] == 0x01) && (m_blower_speed_ccc[1] == 0x00)) {
				vTaskResume(m_blw_task_ctrl[TASK_SPEED].ntf_task);
			}
//			send_ble_response(gatts_if, param, param->write.len, param->write.value);
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

		/* create a blower voltage notification task */
		BaseType_t xret = xTaskCreate(task_blower_voltage_notify, "blower_voltage", COMM_BLE_TASK_STACK_BLOWER_VOLTAGE, NULL,
									COMM_BLE_TASK_PRIO_BLOWER_VOLTAGE, &m_blw_task_ctrl[TASK_VOLTAGE].ntf_task);
		if (xret == pdPASS) {
			m_blw_task_ctrl[TASK_VOLTAGE].stay_alive = true;
			ESP_LOGI(TAG, "blower voltage notification task created successfully");
		} else {
			ESP_LOGE(TAG, "blower voltage notification task creation failed");
		}

		/* create a blower voltage notification task */
		xret = xTaskCreate(task_blower_speed_notify, "blower_speed", COMM_BLE_TASK_STACK_BLOWER_SPEED, NULL,
							COMM_BLE_TASK_PRIO_BLOWER_SPEED, &m_blw_task_ctrl[TASK_SPEED].ntf_task);
		if (xret == pdPASS) {
			m_blw_task_ctrl[TASK_SPEED].stay_alive = true;
			ESP_LOGI(TAG, "blower speed notification task created successfully");
		} else {
			ESP_LOGE(TAG, "blower speed notification task creation failed");
		}

		/* register to event we want to listen */
		app_events_add_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_add_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_add_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_DISCONNECT_EVT: {
		/* call resume in case the notification task is suspended */
		vTaskResume(m_blw_task_ctrl[TASK_VOLTAGE].ntf_task);
		vTaskResume(m_blw_task_ctrl[TASK_SPEED].ntf_task);

		/* tell the task to delete itself */
		m_blw_task_ctrl[TASK_VOLTAGE].stay_alive = false;
		m_blw_task_ctrl[TASK_SPEED].stay_alive = false;

		/* remove event callbacks */
		app_events_remove_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_remove_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_remove_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != BLW_IDX_NB) {
			ESP_LOGE(TAG,
					"create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, BLW_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			/* initialize callback data and register callback functions with host_cmds module */
//			m_packet_mutex = xSemaphoreCreateMutex();
//			if (m_packet_mutex != NULL) {
				cmd_read_register_callback();
//			}

			memcpy(handle_table_blower, param->add_attr_tab.handles, sizeof(handle_table_blower));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_blower[BLW_IDX_SVC]);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "blower service started successfully");
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
		vTaskSuspend(m_blw_task_ctrl[TASK_VOLTAGE].ntf_task);
		vTaskSuspend(m_blw_task_ctrl[TASK_SPEED].ntf_task);
		break;
	case APP_EVENT_FW_UPDATING_COPROC:
		break;
	case APP_EVENT_FW_UPDATED_COPROC:
		break;
	default:
		break;
	}
}

static void task_blower_voltage_notify( void * pvParameters ) {
//	struct blower_values *p_blw_val = (struct blower_values *)pvParameters;
	esp_err_t ret = 0;

	while (1) {

		/* if the connection is not alive then kill this task */
		if (!m_blw_task_ctrl[TASK_VOLTAGE].stay_alive) {
			break;
		}

		/* if all notifications are disabled, then suspend this task */
		if ((m_blower_voltage_ccc[0] == 0x00) && (m_blower_voltage_ccc[1] == 0x00)) {
			ESP_LOGW(TAG, "suspending blower voltage notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from blower voltage notification task");
		}

		/* Check if blower voltage notification is enabled */
		if ((m_blower_voltage_ccc[0] == 0x01) && (m_blower_voltage_ccc[1] == 0x00)) {
			/* Read the blower voltage from host processor */

			if (update_data_availability(m_cb_data[CB_VOLTS_MV].cmd_mutex,
					&m_cb_data[CB_VOLTS_MV].data_available, false) < 0)
				continue;

			ret = host_cmds_blower_voltage_read();	/* Get the blower voltage from host processor */
			ret = wait_until_timeout(&m_cb_data[CB_VOLTS_MV].data_available);
			if (!ret) {
//				ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id,
//						handle_table_blower[BLW_IDX_VOLTAGE_VAL], m_packet.payload_len,
//						(uint8_t*) m_packet.payload, false);
				ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id,
						handle_table_blower[BLW_IDX_VOLTAGE_VAL], sizeof(m_blw_val.blower_mvolts),
						(uint8_t*) &m_blw_val.blower_mvolts, false);
				if (ret == ESP_OK) {
					ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
				} else {
					ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
				}
			}

#if 0
				char cmd[HOST_CMD_COMMAND_SIZE_MAX] = { 0x00 };
				memcpy(cmd, m_packet.cmd, m_packet.cmd_len);

				if (!strcmp(cmd, CMD_BLOWER_GET_VOLTS_MV)) {
					memcpy(&p_blw_val->blower_mvolts, m_packet.payload, sizeof(m_packet.payload_len));
					/* Notify to connected app */
					ret = esp_ble_gatts_send_indicate(m_blw_task_ctrl.gatts_if, m_blw_task_ctrl.conn_id,
							handle_table_blower[BLW_IDX_VOLTAGE_VAL], sizeof(p_blw_val->blower_mvolts),
							(uint8_t*) &p_blw_val->blower_mvolts, false);
					if (ret == ESP_OK) {
						ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
					} else {
						ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
					}
				}
			}

			if (m_cb_data.cmd_mutex != NULL) {
				if (xSemaphoreTake(m_cb_data.cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
					m_cb_data.data_available = false;
					xSemaphoreGive(m_cb_data.cmd_mutex);
				} else {
					break;
				}
			}

			ret = host_cmds_blower_voltage_read();

			bool wait_timeout = false;
			uint32_t delay = 0;
			while (!m_cb_data.data_available) {
				vTaskDelay(DATA_AVAILABLE_LOOP_DELAY);
				delay += DATA_AVAILABLE_LOOP_DELAY;
				if (delay > DATA_AVAILABLE_TIMEOUT) {
					wait_timeout = true;
					ESP_LOGE(TAG, "task_blower_notify wait timeout!");
					break;
				}
			}

			if (!wait_timeout) {
				char cmd[HOST_CMD_COMMAND_SIZE_MAX] = { 0x00 };
				memcpy(cmd, m_packet.cmd, m_packet.cmd_len);

				if (!strcmp(cmd, CMD_BLOWER_GET_VOLTS_MV)) {
					memcpy(&p_blw_val->blower_mvolts, m_packet.payload, sizeof(m_packet.payload_len));
					/* Notify to connected app */
					ret = esp_ble_gatts_send_indicate(m_blw_task_ctrl.gatts_if, m_blw_task_ctrl.conn_id,
							handle_table_blower[BLW_IDX_VOLTAGE_VAL], sizeof(p_blw_val->blower_mvolts),
							(uint8_t*) &p_blw_val->blower_mvolts, false);
					if (ret == ESP_OK) {
						ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
					} else {
						ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
					}
				}
			}
#endif
		}

#if 0
		/* Check if blower speed notification is enabled */
		if ((m_blower_speed_ccc[0] == 0x01) && (m_blower_speed_ccc[1] == 0x00)) {
			/* Read the blower speed from host processor */
			char blower_speed[10] = "5250";
//			ret = host_cmds_battery_level_read(blower_voltage, sizeof(blower_voltage));
			p_blw_val->blower_speed = atoi(blower_speed);

			if (m_cb_data.cmd_mutex != NULL) {
				if (xSemaphoreTake(m_cb_data.cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
					m_cb_data.data_available = false;
					xSemaphoreGive(m_cb_data.cmd_mutex);
				} else {
					break;
				}
			}
			while (!m_cb_data.data_available);

			if (!strcmp(m_packet.cmd, CMD_BLOWER_GET_SPEED_HZ)) {

			}

			/* Notify to connected app */
			ret = esp_ble_gatts_send_indicate(m_blw_task_ctrl.gatts_if, m_blw_task_ctrl.conn_id, handle_table_blower[BLW_IDX_SPEED_VAL],
					sizeof(p_blw_val->blower_speed), (uint8_t*)&p_blw_val->blower_speed, false);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
			} else {
				ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
			}
		}
#endif

		/* TODO update rate from kconfig */
		vTaskDelay((1*5000) / portTICK_PERIOD_MS);
	}

	ESP_LOGW(TAG, "deleting blower voltage notification task ...");
	vTaskDelete(NULL);
}

static void task_blower_speed_notify( void * pvParameters ) {
	esp_err_t ret = 0;

	while (1) {

		/* if the connection is not alive then kill this task */
		if (!m_blw_task_ctrl[TASK_SPEED].stay_alive) {
			break;
		}

		/* if all notifications are disabled, then suspend this task */
		if ((m_blower_speed_ccc[0] == 0x00) && (m_blower_speed_ccc[1] == 0x00)) {
			ESP_LOGW(TAG, "suspending blower speed notification task!");
			vTaskSuspend(NULL);
			ESP_LOGW(TAG, "resumed from blower speed notification task");
		}

		/* Check if blower speed notification is enabled */
		if ((m_blower_speed_ccc[0] == 0x01) && (m_blower_speed_ccc[1] == 0x00)) {
			/* Read the blower speed from host processor */

			if (update_data_availability(m_cb_data[CB_SPEED].cmd_mutex,
					&m_cb_data[CB_SPEED].data_available, false) < 0)
				continue;

			ret = host_cmds_blower_speed_hz_read();	/* Get the blower speed from host processor */
			ret = wait_until_timeout(&m_cb_data[CB_SPEED].data_available);
			if (!ret) {
//				ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id,
//						handle_table_blower[BLW_IDX_SPEED_VAL], m_packet.payload_len,
//						(uint8_t*) m_packet.payload, false);
				ret = esp_ble_gatts_send_indicate(m_gatts_if, m_conn_id,
						handle_table_blower[BLW_IDX_SPEED_VAL], sizeof(m_blw_val.blower_speed),
						(uint8_t*) &m_blw_val.blower_speed, false);
				if (ret == ESP_OK) {
					ESP_LOGI(TAG, "esp_ble_gatts_send_indicate successfully");
				} else {
					ESP_LOGE(TAG, "esp_ble_gatts_send_indicate failed");
				}
			}
		}

		/* TODO update rate from kconfig */
		vTaskDelay((1*1000) / portTICK_PERIOD_MS);
	}

	ESP_LOGW(TAG, "deleting blower speed notification task ...");
	vTaskDelete(NULL);
}

