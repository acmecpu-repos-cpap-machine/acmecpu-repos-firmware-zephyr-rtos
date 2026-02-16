/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_blower_service.h
 * Created on: 27-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_COMM_BLE_BLOWER_SERVICE_H_
#define COMPONENTS_COMM_BLE_COMM_BLE_BLOWER_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

void gatts_blower_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_BLE_COMM_BLE_BLOWER_SERVICE_H_ */
