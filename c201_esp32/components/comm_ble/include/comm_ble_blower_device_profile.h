/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble.h
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_INCLUDE_COMM_BLE_BLOWER_DEVICE_PROFILE_H_
#define COMPONENTS_COMM_BLE_INCLUDE_COMM_BLE_BLOWER_DEVICE_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_system.h"

#define PROFILE_NUM             4//CONFIG_COMM_BLE_PROFILE_NUM
#define APP_ID                  CONFIG_COMM_BLE_APP_ID
#define SAMPLE_DEVICE_NAME      CONFIG_COMM_BLE_DEVICE_NAME

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

int comm_ble_init();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_BLE_INCLUDE_COMM_BLE_BLOWER_DEVICE_PROFILE_H_ */
