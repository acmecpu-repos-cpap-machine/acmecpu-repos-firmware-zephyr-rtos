/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_common.h
 * Created on: 22-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_COMM_BLE_COMMON_H_
#define COMPONENTS_COMM_BLE_COMM_BLE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_system.h"

/* TODO get from Kconfig */
#define DATA_AVAILABLE_LOOP_DELAY	(30 / portTICK_PERIOD_MS)
#define DATA_AVAILABLE_TIMEOUT		(50 * DATA_AVAILABLE_LOOP_DELAY)

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_CHAR_VAL_LEN_MAX.
*/
#define GATTS_CHAR_VAL_LEN_MAX CONFIG_COMM_BLE_CHAR_VAL_LEN_MAX
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

/* UUID definition */
extern const uint16_t primary_service_uuid;
extern const uint16_t character_declaration_uuid;
extern const uint16_t character_client_config_uuid;
extern const uint16_t character_format_uuid;

/* Property definition */
extern const uint8_t char_prop_read;
extern const uint8_t char_prop_read_notify;
extern const uint8_t char_prop_write;
extern const uint8_t char_prop_read_write_notify;

/* BLE task stack sizes */
#define COMM_BLE_TASK_STACK_BAS				(1024*3)
#define COMM_BLE_TASK_STACK_BLOWER_VOLTAGE	(1024*2)
#define COMM_BLE_TASK_STACK_BLOWER_SPEED	(1024*2)
#define COMM_BLE_TASK_STACK_DFU				(1024*2)
#define COMM_BLE_TASK_STACK_SENSOR_GET		(1024*2)
#define COMM_BLE_TASK_STACK_SENSOR_GETALL	(1024*4)

/* BLE task priorities */
#define COMM_BLE_TASK_PRIO_BAS				(10)
#define COMM_BLE_TASK_PRIO_BLOWER_VOLTAGE	(8)
#define COMM_BLE_TASK_PRIO_BLOWER_SPEED		(8)
#define COMM_BLE_TASK_PRIO_DFU				(7)
#define COMM_BLE_TASK_PRIO_SENSOR_GET		(9)
#define COMM_BLE_TASK_PRIO_SENSOR_GETALL	(9)


#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_BLE_COMM_BLE_COMMON_H_ */
