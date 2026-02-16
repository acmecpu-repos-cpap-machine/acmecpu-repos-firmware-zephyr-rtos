/*
 * Copyright (c) 2022 Acme CPU
 *
 * comm_ble_sensor_service.h
 * Created on: 10-Aug-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_COMM_BLE_SENSOR_SERVICE_H_
#define COMPONENTS_COMM_BLE_COMM_BLE_SENSOR_SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_LIST_MAX_LEN				51		/* 25 sensors max (each sensor takes 2 bytes + 1 byte for sensor count) */
#define SENSOR_GET_VAL_MAX_LEN			8
#define SENSOR_GETALL_VAL_MAX_LEN		251		/* 25 sensors max (each sensor takes 10 bytes + 1 byte for sensor count) */

void gatts_sensor_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_COMM_BLE_COMM_BLE_SENSOR_SERVICE_H_ */
