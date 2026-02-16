/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_bt_device.h"

#include <string.h>

#include "comm_ble_blower_device_profile.h"
#include "comm_ble_device_info_service.h"
#include "comm_ble_battery_service.h"
#include "comm_ble_blower_service.h"
#include "comm_ble_dfu_service.h"
#include "comm_ble_stepper_service.h"
#include "comm_ble_settings_service.h"
#include "comm_ble_sensor_service.h"

#define TAG	"comm_ble"

/* Indexes of the profile table array */
//#define PROFILE_IDX_DEVINF_SVC      0		/* device info service */
//#define PROFILE_IDX_BATTERY_SVC     1		/* battery service */
//#define PROFILE_IDX_SETTINGS_SVC    2		/* settings service */
//#define PROFILE_IDX_SENSOR_SVC    	3		/* sensor service */
//#define PROFILE_IDX_BLOWER_SVC		4		/* blower service */
//#define PROFILE_IDX_DFU_SVC			5		/* device firmware upgrade service */
//#define PROFILE_IDX_STEPPER_SVC		6		/* stepper motor service */

typedef enum {
	PROFILE_IDX_DEVINF_SVC = 0,				/* device info service */
	PROFILE_IDX_BATTERY_SVC,				/* battery service */
	PROFILE_IDX_SETTINGS_SVC,				/* settings service */
	PROFILE_IDX_SENSOR_SVC,					/* sensor service */
	PROFILE_IDX_BLOWER_SVC,					/* blower service */
	PROFILE_IDX_DFU_SVC,					/* device firmware upgrade service */
	PROFILE_IDX_STEPPER_SVC,				/* stepper motor service */

	PROFILE_IDX_MAX
} PROFILE_INDEX;

/* Extern variables */
/* UUID definition */
const uint16_t primary_service_uuid         	= ESP_GATT_UUID_PRI_SERVICE;
const uint16_t character_declaration_uuid   	= ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t character_client_config_uuid 	= ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
const uint16_t character_format_uuid			= ESP_GATT_UUID_CHAR_PRESENT_FORMAT;
/* Property definition */
const uint8_t char_prop_read                	=  ESP_GATT_CHAR_PROP_BIT_READ;
const uint8_t char_prop_read_notify 			= ESP_GATT_CHAR_PROP_BIT_READ|ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_write               	= ESP_GATT_CHAR_PROP_BIT_WRITE;
const uint8_t char_prop_read_write_notify   	= ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

/* Static variables */
/* Service UUID */
static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = ESP_BLE_APPEARANCE_GENERIC_PULSE_OXIMETER,//0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst blower_device_profile_table[PROFILE_NUM] = {
#if (PROFILE_NUM > 0)
    [PROFILE_IDX_DEVINF_SVC] = {
        .gatts_cb = gatts_devinf_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 1)
    [PROFILE_IDX_BATTERY_SVC] = {
        .gatts_cb = gatts_battery_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 2)
    [PROFILE_IDX_SETTINGS_SVC] = {
        .gatts_cb = gatts_settings_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 3)
    [PROFILE_IDX_SENSOR_SVC] = {
        .gatts_cb = gatts_sensor_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 4)
    [PROFILE_IDX_BLOWER_SVC] = {
        .gatts_cb = gatts_blower_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 5)
    [PROFILE_IDX_DFU_SVC] = {
        .gatts_cb = gatts_dfu_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
#if (PROFILE_NUM > 6)
    [PROFILE_IDX_STEPPER_SVC] = {
        .gatts_cb = gatts_stepper_service_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
#endif
};

static char *esp_key_type_to_str(esp_ble_key_type_t key_type)
{
   char *key_str = NULL;
   switch(key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;

   }

   return key_str;
}

static char *esp_auth_req_to_str(esp_ble_auth_req_t auth_req)
{
   char *auth_str = NULL;
   switch(auth_req) {
    case ESP_LE_AUTH_NO_BOND:
        auth_str = "ESP_LE_AUTH_NO_BOND";
        break;
    case ESP_LE_AUTH_BOND:
        auth_str = "ESP_LE_AUTH_BOND";
        break;
    case ESP_LE_AUTH_REQ_MITM:
        auth_str = "ESP_LE_AUTH_REQ_MITM";
        break;
    case ESP_LE_AUTH_REQ_BOND_MITM:
        auth_str = "ESP_LE_AUTH_REQ_BOND_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_ONLY:
        auth_str = "ESP_LE_AUTH_REQ_SC_ONLY";
        break;
    case ESP_LE_AUTH_REQ_SC_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_BOND";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM";
        break;
    case ESP_LE_AUTH_REQ_SC_MITM_BOND:
        auth_str = "ESP_LE_AUTH_REQ_SC_MITM_BOND";
        break;
    default:
        auth_str = "INVALID BLE AUTH REQ";
        break;
   }

   return auth_str;
}

static void show_bonded_devices(void) {
    int dev_num = esp_ble_get_bond_device_num();

    esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t *)malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
    esp_ble_get_bond_device_list(&dev_num, dev_list);
    ESP_LOGI(TAG, "Bonded devices number : %d\n", dev_num);

    ESP_LOGI(TAG, "Bonded devices list : %d\n", dev_num);
    for (int i = 0; i < dev_num; i++) {
        esp_log_buffer_hex(TAG, (void *)dev_list[i].bd_addr, sizeof(esp_bd_addr_t));
    }

    free(dev_list);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
	switch (event) {
	case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
			esp_ble_gap_start_advertising(&adv_params);
		break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        /* advertising start complete event to indicate advertising start successfully or failed */
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "advertising start failed");
        }else{
            ESP_LOGI(TAG, "advertising start successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising stop failed");
        }
        else {
            ESP_LOGI(TAG, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
              param->update_conn_params.status,
              param->update_conn_params.min_int,
              param->update_conn_params.max_int,
              param->update_conn_params.conn_int,
              param->update_conn_params.latency,
              param->update_conn_params.timeout);
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        /* send the positive(true) security response to the peer device to accept the security request.
        If not accept the security request, should send the security response with negative(false) accept value*/
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
    	/* the app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
           show the passkey number to the user to input it in the peer device. */
        ESP_LOGW(TAG, "The passkey Notify number:%06ld", param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_KEY_EVT:
        /* shows the ble key info share with peer device to the user. */
        ESP_LOGI(TAG, "key type = %s", esp_key_type_to_str(param->ble_security.ble_key.key_type));
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT: {
        esp_bd_addr_t bd_addr;
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",\
                (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGW(TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGI(TAG, "auth mode = %s",esp_auth_req_to_str(param->ble_security.auth_cmpl.auth_mode));
        }
        show_bonded_devices();
        break;
    }
	case ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT: {
		ESP_LOGD(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV_COMPLETE_EVT status = %d",
				param->remove_bond_dev_cmpl.status);
		ESP_LOGI(TAG, "ESP_GAP_BLE_REMOVE_BOND_DEV");
		ESP_LOGI(TAG, "-----ESP_GAP_BLE_REMOVE_BOND_DEV----");
		esp_log_buffer_hex(TAG, (void *)param->remove_bond_dev_cmpl.bd_addr, sizeof(esp_bd_addr_t));
		ESP_LOGI(TAG, "------------------------------------");
		break;
	}
	default:
		break;
	}
}

void blower_profile_callback_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
									esp_ble_gatts_cb_param_t *param)
{
    switch(event) {
        case ESP_GATTS_REG_EVT: {
            esp_ble_gap_config_local_icon (ESP_BLE_APPEARANCE_GENERIC_PULSE_OXIMETER);

            /* BT device name = <NAME>_<6 byte BT address>*/
            const uint8_t *bt_addr = esp_bt_dev_get_address();
            ESP_LOGD(TAG, "bt_addr: %02x:%02x:%02x:%02x:%02x:%02x", bt_addr[0], bt_addr[1], bt_addr[2], bt_addr[3], bt_addr[4], bt_addr[5]);
            ESP_LOG_BUFFER_HEXDUMP(TAG, bt_addr, 6, ESP_LOG_DEBUG);

            char bt_addr_str[12+1] = {0x00};
            int i, j=0;
            for (i=0; i<6; i++) {
            	sprintf(bt_addr_str+(i+j), "%02x", bt_addr[i]);
            	j++;
            }
            ESP_LOGD(TAG, "bt_addr_str = %s", bt_addr_str);

            char bt_adv_name[20] = SAMPLE_DEVICE_NAME;
            strncat(bt_adv_name, "_", 2);
            strncat(bt_adv_name, (const char*)bt_addr_str, 12);

            ESP_LOGI(TAG, "BT adv name = %s", bt_adv_name);

    		esp_err_t ret = esp_ble_gap_set_device_name(bt_adv_name);
    		if (ret) {
    			ESP_LOGE(TAG, "set device name failed, error code = %x", ret);
    		}
    		/* config adv data */
    		ret = esp_ble_gap_config_adv_data(&adv_data);
    		if (ret) {
    			ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
    		}
            break;
        }
        case ESP_GATTS_CONF_EVT: {
            break;
        }
        case ESP_GATTS_CREATE_EVT:
            break;
        case ESP_GATTS_CONNECT_EVT: {
    		ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
    		esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);

            /* start security connect with peer device when receive the connect event sent by the master */
            esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT_MITM);
#if 0
    		esp_ble_conn_update_params_t conn_params = { 0 };
    		memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    		/* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
    		conn_params.latency = 0;
    		conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
    		conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
    		conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
    		//start sent the update connection parameters to the peer device.
    		esp_ble_gap_update_conn_params(&conn_params);
#endif
            break;
        }
        case ESP_GATTS_DISCONNECT_EVT: {
    		esp_ble_gap_start_advertising(&adv_params);
            break;
        }
        case ESP_GATTS_CLOSE_EVT:
            break;
        case ESP_GATTS_WRITE_EVT: {
            break;
        }
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
            break;
         }

        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {

	/* If event is register event, store the gatts_if for each profile */
	if (event == ESP_GATTS_REG_EVT) {
		if (param->reg.status == ESP_GATT_OK) {
//			blower_device_profile_table[PROFILE_IDX_DEVINF_SVC].gatts_if = gatts_if;
			blower_device_profile_table[param->reg.app_id].gatts_if = gatts_if;
			blower_profile_callback_handler(event, gatts_if, param);
		} else {
			ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
			return;
		}
	}

	if ((event == ESP_GATTS_CONNECT_EVT) || (event == ESP_GATTS_DISCONNECT_EVT)) {
		blower_profile_callback_handler(event, gatts_if, param);
	}

	do {
		int idx;
		for (idx = 0; idx < PROFILE_NUM; idx++) {
			/* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
			if (gatts_if == ESP_GATT_IF_NONE || gatts_if == blower_device_profile_table[idx].gatts_if) {
				if (blower_device_profile_table[idx].gatts_cb) {
					blower_device_profile_table[idx].gatts_cb(event, gatts_if, param);
				}
			}
		}
	} while (0);
}

#if CONFIG_BT_PAIRING_WITH_PAIR_OPTION
#define BT_PAIRING_PASSKEY							CONFIG_BT_SECURE_PAIRING_PASSKEY_AUTO //123456

#elif CONFIG_BT_PAIRING_WITH_PASSKEY_INPUT
	#if CONFIG_BT_PAIRING_WITH_RANDOM_PASSKEY
	#define BT_PAIRING_PASSKEY						CONFIG_BT_SECURE_PAIRING_PASSKEY_RANDOM
	#elif CONFIG_BT_PAIRING_WITH_STATIC_PASSKEY
	#define BT_PAIRING_PASSKEY						CONFIG_BT_SECURE_PAIRING_PASSKEY_STATIC
	#endif /* CONFIG_BT_PAIRING_WITH_PASSKEY_INPUT */

#elif CONFIG_BT_PAIRING_WITH_YES_NO_OPTION
#define BT_PAIRING_PASSKEY							(123456)
#endif /* CONFIG_BT_PAIRING_WITH_PAIR_OPTION */

static int ble_security_enable() {
	int ret = 0;

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;     //bonding with peer device after authentication

#if CONFIG_BT_PAIRING_WITH_PAIR_OPTION
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           			//set the IO capability to No output No input
#elif CONFIG_BT_PAIRING_WITH_PASSKEY_INPUT
	esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT; 						//set the IO capability to Display only
#elif CONFIG_BT_PAIRING_WITH_YES_NO_OPTION
	esp_ble_io_cap_t iocap = ESP_IO_CAP_IO; 						//set the IO capability to Display YesNo
#endif

	uint8_t key_size = 16;      									//the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    //set passkey
    uint32_t passkey = BT_PAIRING_PASSKEY;

    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));

    /* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the master;
    If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

    return ret;
}

int comm_ble_init() {
	int ret = 0;

//    /* Initialize NVS. */
//    ret = nvs_flash_init();
//    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//        ESP_ERROR_CHECK(nvs_flash_erase());
//        ret = nvs_flash_init();
//    }
//    ESP_ERROR_CHECK( ret );

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT()
	;
	ret = esp_bt_controller_init(&bt_cfg);
	if (ret) {
		ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret) {
		ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_init();
	if (ret) {
		ESP_LOGE(TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	ret = esp_bluedroid_enable();
	if (ret) {
		ESP_LOGE(TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
		return ret;
	}

	/* register the gatt server callback function */
	ret = esp_ble_gatts_register_callback(gatts_event_handler);
	if (ret) {
		ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
		return ret;
	}

	/* register the gap callback function */
	ret = esp_ble_gap_register_callback(gap_event_handler);
	if (ret) {
		ESP_LOGE(TAG, "gap register error, error code = %x", ret);
		return ret;
	}

	/* Register the apps (services) under this profile */
#if (PROFILE_NUM > 0)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_DEVINF_SVC);	// Device info service
#endif
#if (PROFILE_NUM > 1)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_BATTERY_SVC);	// Battery service
#endif
#if (PROFILE_NUM > 2)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_SETTINGS_SVC);	// Settings service
#endif
#if (PROFILE_NUM > 3)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_SENSOR_SVC);	// Sensor service
#endif
#if (PROFILE_NUM > 4)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_BLOWER_SVC);	// Blower service
#endif
#if (PROFILE_NUM > 5)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_DFU_SVC);		// DFU service
#endif
#if (PROFILE_NUM > 6)
	ret = esp_ble_gatts_app_register(PROFILE_IDX_STEPPER_SVC);	// Stepper service
#endif
	if (ret) {
		ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
		return ret;
	}

	/* Set the MTU */
	esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(CONFIG_COMM_BLE_GATT_MTU_SIZE);
	if (local_mtu_ret) {
		ESP_LOGE(TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
	}

	/* Enable BLE security (pairing, bonding and encryption*/
	ret = ble_security_enable();

	return ret;
}
