/*
 * Copyright (c) 2022 Acme CPU
 *
 * comm_ble_settings_service.h
 * Created on: 03-Aug-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_COMM_BLE_SETTINGS_SERVICE_H_
#define COMPONENTS_COMM_BLE_COMM_BLE_SETTINGS_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_VALUE_MAX_LEN	32

void gatts_settings_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_BLE_COMM_BLE_SETTINGS_SERVICE_H_ */
