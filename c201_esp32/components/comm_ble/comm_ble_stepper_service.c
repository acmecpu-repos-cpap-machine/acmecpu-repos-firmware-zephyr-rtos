/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_stepper_service.c
 * Created on: 07-Jul-2021
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
#include <stdlib.h>

#include "host_cmds.h"
#include "host_cmds_callback.h"
#include "host_cmds_packet.h"
#include "comm_ble_common.h"
#include "comm_ble_blower_device_profile.h"
#include "comm_ble_stepper_service.h"
#include "app_events.h"


#define TAG	"comm_ble_stepper_svc"

#define SVC_INST_ID             	0

/* App event related data */
static struct app_events_callback m_evntcb_toupdt_coproc;
static struct app_events_callback m_evntcb_updting_coproc;
static struct app_events_callback m_evntcb_updted_coproc;
static void app_event_handler(struct app_events_callback *cb, APP_EVENT_TYPE event);

esp_gatt_if_t m_gatts_if;
uint16_t m_conn_id;

/* Stepper Service Attribute Indexes */
enum {
    STP_IDX_SVC,

    STP_IDX_DIR_CHAR,
    STP_IDX_DIR_VAL,

    STP_IDX_SPEED_CHAR,
    STP_IDX_SPEED_VAL,

    STP_IDX_NUMROT_CHAR,
    STP_IDX_NUMROT_VAL,

	STP_IDX_POSREL_CHAR,
    STP_IDX_POSREL_VAL,

	STP_IDX_POSABS_CHAR,
    STP_IDX_POSABS_VAL,

	STP_IDX_POSCURR_CHAR,
    STP_IDX_POSCURR_VAL,

	STP_IDX_ZEROSET_CHAR,
    STP_IDX_ZEROSET_VAL,

    STP_IDX_NB,
};

/* Array of handles to each element */
static uint16_t handle_table_stepper[STP_IDX_NB];

/* Stepper Service UUID and values */
struct stepper_values {
	uint8_t dir;
	uint32_t speed_hz;
	uint32_t num_rot;
	uint16_t pos_rel;
	uint16_t pos_abs;
	uint16_t pos_curr;
	uint8_t zero_set;
};
static struct stepper_values m_stp_val = {0, 0, 0, 0, 0, 0, 0};

static const uint16_t m_stepper_svc 			= 0x8030;

static const uint16_t m_stepper_dir_uuid 		= 0x8031;
static const uint16_t m_stepper_speed_uuid		= 0x8032;
static const uint16_t m_num_rot_uuid			= 0x8033;
static const uint16_t m_pos_rel_uuid			= 0x8034;
static const uint16_t m_pos_abs_uuid			= 0x8035;
static const uint16_t m_pos_curr_uuid			= 0x8036;
static const uint16_t m_zeroset_uuid			= 0x8037;

/* Stepper Service Database Description */
static const esp_gatts_attr_db_t gatt_db_stepper_svc[STP_IDX_NB] =
{
    // Stepper Service Declaration
    [STP_IDX_SVC] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
    		sizeof(uint16_t), sizeof(m_stepper_svc), (uint8_t *)&m_stepper_svc}},

    // Stepper direction Characteristic Declaration
    [STP_IDX_DIR_CHAR] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

    // Stepper direction Characteristic Value
    [STP_IDX_DIR_VAL] =
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_stepper_dir_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    		sizeof(uint8_t),sizeof(uint8_t), &m_stp_val.dir}},

	// Stepper speed Characteristic Declaration
	[STP_IDX_SPEED_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

	// Stepper speed Characteristic Value
	[STP_IDX_SPEED_VAL] =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_stepper_speed_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
			sizeof(uint32_t),sizeof(uint32_t), (uint8_t *)&m_stp_val.speed_hz}},

	// Stepper number of rotations Characteristic Declaration
	[STP_IDX_NUMROT_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},

	// Stepper number of rotations Characteristic Value
	[STP_IDX_NUMROT_VAL] =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_num_rot_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
			sizeof(uint32_t),sizeof(uint32_t), (uint8_t *)&m_stp_val.num_rot}},

	// Stepper position relative Characteristic Declaration
	[STP_IDX_POSREL_CHAR] =
	{{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_WRITE,
			CHAR_DECLARATION_SIZE,CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

	// Stepper position relative Characteristic Value
	[STP_IDX_POSREL_VAL] =
	{{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&m_pos_rel_uuid, ESP_GATT_PERM_WRITE,
			sizeof(uint16_t),sizeof(uint16_t), (uint8_t *)&m_stp_val.pos_rel}},

	// Stepper position absolute Characteristic Declaration
	[STP_IDX_POSABS_CHAR] =
	{{ ESP_GATT_AUTO_RSP }, {ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_WRITE,
	CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_write }},

	// Stepper position absolute Characteristic Value
	[STP_IDX_POSABS_VAL] =
	{{ ESP_GATT_RSP_BY_APP }, {ESP_UUID_LEN_16, (uint8_t*) &m_pos_abs_uuid, ESP_GATT_PERM_WRITE,
			sizeof(uint16_t), sizeof(uint16_t), (uint8_t*) &m_stp_val.pos_abs }},

	// Stepper position current Characteristic Declaration
	[STP_IDX_POSCURR_CHAR] =
	{{ ESP_GATT_AUTO_RSP }, {ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_READ,
	CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_read }},

	// Stepper position current Characteristic Value
	[STP_IDX_POSCURR_VAL] =
	{{ ESP_GATT_RSP_BY_APP }, {ESP_UUID_LEN_16, (uint8_t*) &m_pos_curr_uuid, ESP_GATT_PERM_READ,
			sizeof(uint16_t), sizeof(uint16_t), (uint8_t*) &m_stp_val.pos_curr }},

	// Stepper zero set Characteristic Declaration
	[STP_IDX_ZEROSET_CHAR] =
	{{ ESP_GATT_AUTO_RSP }, {ESP_UUID_LEN_16, (uint8_t*) &character_declaration_uuid, ESP_GATT_PERM_WRITE,
	CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t*) &char_prop_write }},

	// Stepper zero set Characteristic Value
	[STP_IDX_ZEROSET_VAL] =
	{{ ESP_GATT_RSP_BY_APP }, {ESP_UUID_LEN_16, (uint8_t*) &m_zeroset_uuid, ESP_GATT_PERM_WRITE,
			sizeof(uint8_t), sizeof(uint8_t), (uint8_t*) &m_stp_val.zero_set }},
};

/* Callback data and functions */
typedef enum {
	CB_DIR=0,
	CB_SPEED_HZ,
	CB_NUM_ROT,
	CB_POS_CURR,
	CB_NB
} READ_CB;
#define NUM_READ_CB		(CB_NB)

static struct host_cmd_callback cb_stepper_dir_get;			/* stepper direction get callback object */
static struct host_cmd_callback cb_stepper_speed_hz_get;	/* stepper speed get callback object */
static struct host_cmd_callback cb_stepper_num_rot_get;		/* stepper configured number of rotations get callback object */
static struct host_cmd_callback cb_stepper_pos_curr_get;	/* stepper current position get callback object */
static struct host_cmd_packet_t m_packet;					/* global packet object */
static SemaphoreHandle_t m_packet_mutex;					/* mutex to protect the global packet object */
struct cmd_read_cb {
	bool data_available;
	char cmd[HOST_CMD_COMMAND_SIZE_MAX];
	SemaphoreHandle_t cmd_mutex;
};
static struct cmd_read_cb m_cb_data[NUM_READ_CB] = {
		[CB_DIR] = {
				.data_available = false,
				.cmd = CMD_STEPPER_DIR_GET
		},
		[CB_SPEED_HZ] = {
				.data_available = false,
				.cmd = CMD_STEPPER_SPEED_HZ_GET
		},
		[CB_NUM_ROT] = {
				.data_available = false,
				.cmd = CMD_STEPPER_NUM_ROT_GET
		},
		[CB_POS_CURR] = {
				.data_available = false,
				.cmd = CMD_STEPPER_POS_CUR_GET
		},
};

void cb_handler_stepper_cmds(struct host_cmd_callback *cb,
		uint32_t cmd,
		void *packet) {
	struct host_cmd_packet_t *pac = (struct host_cmd_packet_t *) packet;

	ESP_LOGI(TAG, "cb_handler_stepper_cmds");
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
	for (int i=0; i<NUM_READ_CB; i++) {
		if (!strcmp((char*)m_packet.cmd, m_cb_data[i].cmd)) {
			if (xSemaphoreTake(m_cb_data[i].cmd_mutex, (TickType_t) 10/portTICK_PERIOD_MS ) == pdTRUE) {
				m_cb_data[i].data_available = true;
				xSemaphoreGive(m_cb_data[i].cmd_mutex);
			}
		}
	}
}

static void cmd_read_register_callback() {
	m_cb_data[CB_DIR].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_stepper_dir_get, cb_handler_stepper_cmds, C20X_M2M_CMD_STEPPER_RESET_DIR);

	m_cb_data[CB_SPEED_HZ].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_stepper_speed_hz_get, cb_handler_stepper_cmds, C20X_M2M_CMD_STEPPER_STEP_SPEED_HZ);

	m_cb_data[CB_NUM_ROT].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_stepper_num_rot_get, cb_handler_stepper_cmds, C20X_M2M_CMD_STEPPER_RESET_ROT_CNT);

	m_cb_data[CB_POS_CURR].cmd_mutex = xSemaphoreCreateMutex();
	host_cmds_add_callback(&cb_stepper_pos_curr_get, cb_handler_stepper_cmds, C20X_M2M_CMD_STEPPER_RESET_POS);
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

void gatts_stepper_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	switch (event) {
	case ESP_GATTS_REG_EVT: {
		esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db_stepper_svc, gatts_if, STP_IDX_NB, SVC_INST_ID);
		if (create_attr_ret) {
			ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
		}
		m_gatts_if = gatts_if;
	}
		break;
	case ESP_GATTS_READ_EVT: {
		ESP_LOGI(TAG, "ESP_GATTS_READ_EVT conn_id=0x%x, trans_id=0x%x", (int)param->read.conn_id, (int)param->read.trans_id);

		esp_err_t ret = find_char_and_desr_index(param->read.handle, handle_table_stepper, STP_IDX_NB);

		if (ret == STP_IDX_DIR_VAL) {
			ESP_LOGI(TAG, "Read event for STP_IDX_DIR_VAL, %d", ret);

			if (update_data_availability(m_cb_data[CB_DIR].cmd_mutex, &m_cb_data[CB_DIR].data_available, false) < 0)
				break;
			ret = host_cmds_stepper_dir_read(); /* Get the stepper direction of rotation from host processor */
			if (wait_until_timeout(&m_cb_data[CB_DIR].data_available) < 0)
				break;
			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);

		} else if (ret == STP_IDX_SPEED_VAL) {
			ESP_LOGI(TAG, "Read event for STP_IDX_SPEED_VAL, %d", ret);

			if (update_data_availability(m_cb_data[CB_SPEED_HZ].cmd_mutex, &m_cb_data[CB_SPEED_HZ].data_available,
			false) < 0)
				break;
			ret = host_cmds_stepper_speed_hz_read(); /* Get the stepper motor's speed in hertz from the host processor */
			if (wait_until_timeout(&m_cb_data[CB_SPEED_HZ].data_available) < 0)
				break;
			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);

		} else if (ret == STP_IDX_NUMROT_VAL) {
			ESP_LOGI(TAG, "Read event for STP_IDX_NUMROT_VAL, %d", ret);
			if (update_data_availability(m_cb_data[CB_NUM_ROT].cmd_mutex, &m_cb_data[CB_NUM_ROT].data_available, false)
					< 0)
				break;
			ret = host_cmds_stepper_num_rot_read(); /* Get the configured number of rotation from host processor */
			if (wait_until_timeout(&m_cb_data[CB_NUM_ROT].data_available) < 0)
				break;
			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
		} else if (ret == STP_IDX_POSCURR_VAL) {
			ESP_LOGI(TAG, "Read event for STP_IDX_POSCURR_VAL, %d", ret);
			if (update_data_availability(m_cb_data[CB_POS_CURR].cmd_mutex, &m_cb_data[CB_POS_CURR].data_available,
					false) < 0)
				break;
			ret = host_cmds_stepper_pos_cur_read(); /* Get the current position of the stepper motor from the host processor */
			if (wait_until_timeout(&m_cb_data[CB_POS_CURR].data_available) < 0)
				break;
			send_ble_response(gatts_if, param, m_packet.payload_len, m_packet.payload);
		}
	}
		break;
	case ESP_GATTS_WRITE_EVT:
		ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT");
		esp_err_t ret = find_char_and_desr_index(param->write.handle, handle_table_stepper, STP_IDX_NB);

		switch (ret) {
		case STP_IDX_DIR_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_DIR_VAL, %d", ret);
			memcpy(&m_stp_val.dir, param->write.value, param->write.len);
			ret = host_cmds_stepper_dir_write(m_stp_val.dir);		/* Set the stepper motor's direction by sending command to the host processor */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case STP_IDX_SPEED_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_SPEED_VAL, %d", ret);
			memcpy(&m_stp_val.speed_hz, param->write.value, param->write.len);
			ret = host_cmds_stepper_speed_hz_write(m_stp_val.speed_hz);	/* Set the stepper motor's clock speed in hertz by sending command to the host processor */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case STP_IDX_NUMROT_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_NUMROT_VAL, %d", ret);
			memcpy(&m_stp_val.num_rot, param->write.value, param->write.len);
			ret = host_cmds_stepper_num_rot_write(m_stp_val.num_rot);	/* Set the number of rotations by sending command to the host processor */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case STP_IDX_POSREL_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_POSREL_VAL, %d", ret);
			memcpy(&m_stp_val.pos_rel, param->write.value, param->write.len);
			ret = host_cmds_stepper_pos_rel_write(m_stp_val.pos_rel);	/* Set the relative position by sending command to the host processor */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case STP_IDX_POSABS_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_POSABS_VAL, %d", ret);
			memcpy(&m_stp_val.pos_abs, param->write.value, param->write.len);
			ret = host_cmds_stepper_pos_abs_write(m_stp_val.pos_abs);	/* Set the absolute position by sending command to the host processor */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
		case STP_IDX_ZEROSET_VAL: {
			ESP_LOGI(TAG, "Write event for STP_IDX_ZEROSET_VAL, %d", ret);
			memcpy(&m_stp_val.zero_set, param->write.value, param->write.len);
			ret = host_cmds_stepper_zeroset_write(m_stp_val.zero_set);	/* Send command to the host processor to set the current position as absolute zero */
			send_ble_response(gatts_if, param, param->write.len, param->write.value);
		}
		break;
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

		/* register to event we want to listen */
		app_events_add_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_add_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_add_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_DISCONNECT_EVT: {
		/* remove event callbacks */
		app_events_remove_callback(&m_evntcb_toupdt_coproc, app_event_handler, APP_EVENT_FW_TO_UPDATE_COPROC);
		app_events_remove_callback(&m_evntcb_updting_coproc, app_event_handler, APP_EVENT_FW_UPDATING_COPROC);
		app_events_remove_callback(&m_evntcb_updted_coproc, app_event_handler, APP_EVENT_FW_UPDATED_COPROC);
	}
		break;
	case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
		if (param->add_attr_tab.status != ESP_GATT_OK) {
			ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
		} else if (param->add_attr_tab.num_handle != STP_IDX_NB) {
			ESP_LOGE(TAG,
					"create attribute table abnormally, num_handle (%d) \
                        doesn't equal to IDX_NB(%d)",
					param->add_attr_tab.num_handle, STP_IDX_NB);
		} else {
			ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d",
					param->add_attr_tab.num_handle);

			/* register callback functions with host_cmds module */
			/* initialize callback data and register callback functions with host_cmds module */
			m_packet_mutex = xSemaphoreCreateMutex();
			if (m_packet_mutex != NULL) {
				cmd_read_register_callback();
			}

			memcpy(handle_table_stepper, param->add_attr_tab.handles, sizeof(handle_table_stepper));
			esp_err_t ret = esp_ble_gatts_start_service(handle_table_stepper[STP_IDX_SVC]);
			if (ret == ESP_OK) {
				ESP_LOGI(TAG, "stepper service started successfully");
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

