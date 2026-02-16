/*
 * Copyright (c) 2021 Acme CPU
 *
 * comm_ble_services.h
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_COMM_BLE_COMM_BLE_DEVICE_INFO_SERVICE_H_
#define COMPONENTS_COMM_BLE_COMM_BLE_DEVICE_INFO_SERVICE_H_

#include "esp_system.h"
#include "esp_gatts_api.h"

/*
 * Device Information Service
 * - This service is used to read the device information
 * 	 from the C201 device, like
 * 	 mfr name, model num, firmware version, software version, serial number
 * */

/* Device Information Service Attribute Indexes */
enum {
	IDX_DEVINF_SVC,

	/* characteristics to read manufacturer name */
	IDX_DEVINF_MFR_NAME_CHAR,
	IDX_DEVINF_MFR_NAME_VALUE,

	/* characteristics to read model number */
	IDX_DEVINF_MODEL_NUM_CHAR,
	IDX_DEVINF_MODEL_NUM_VALUE,

	/* characteristics to read serial number */
	IDX_DEVINF_SERIAL_NUM_CHAR,
	IDX_DEVINF_SERIAL_NUM_VALUE,

	/* characteristics to read firmware version */
	IDX_DEVINF_FW_VERSION_CHAR,
	IDX_DEVINF_FW_VERSION_VALUE,

	/* characteristics to read hardware version */
	IDX_DEVINF_HW_VERSION_CHAR,
	IDX_DEVINF_HW_VERSION_VALUE,

	/* characteristics to read software version */
	IDX_DEVINF_SW_VERSION_CHAR,
	IDX_DEVINF_SW_VERSION_VALUE,

	IDX_DEVINF_NB,
};
/* Service characteristics value lengths */
#define DEVICE_INFO_VALUE_MFR_NAME_LEN		18
#define DEVICE_INFO_VALUE_MODEL_NUM_LEN		18
#define DEVICE_INFO_VALUE_SERIAL_NUM_LEN	18
#define DEVICE_INFO_VALUE_FW_VERSION_LEN	18
#define DEVICE_INFO_VALUE_HW_VERSION_LEN	18
#define DEVICE_INFO_VALUE_SW_VERSION_LEN	18
#define DEVICE_INFO_VALUE_MAX_LEN			18

void gatts_devinf_service_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#endif /* COMPONENTS_COMM_BLE_COMM_BLE_DEVICE_INFO_SERVICE_H_ */
