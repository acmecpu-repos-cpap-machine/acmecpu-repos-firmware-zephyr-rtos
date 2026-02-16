/*
 * Copyright (c) 2021 Acme CPU
 *
 * c201_esp32_application.c
 * Created on: 20-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "host_cmds.h"
#include "host_cmds_send_recv.h"
#include "host_cmds_req.h"
#include "app_net_file_download.h"


#include "comm_wifi.h"

#if (CONFIG_BOARD_C201 || CONFIG_BOARD_C204 || CONFIG_BOARD_E206)
	#include "comm_ble_blower_device_profile.h"
#endif

#include "app_events.h"

#if (CONFIG_BOARD_C201 || CONFIG_BOARD_C204)
	#if CONFIG_STM32_USART_BL_HOST_ENABLE
		#include "stm32_usart_bl_host.h"
	#endif
#endif

#if CONFIG_CONSOLE_CMDS_APP
#include "console_cmds.h"
#endif

#define TAG "c201_esp32_app"

int c201_esp32_application_init() {
	int ret = 0;

	/* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

	/* TODO: start the system activity logger */

	/* initialize and verify communication with the host processor */
	ret = host_cmds_init_verify_start();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "host_cmds_init_verify failed");
		return ret;
	}

	/* initialize all the request commands that are supported
	 * we are expecting these commands from the host (main) processor */
	ret = host_cmds_req_init();
	ret = host_cmds_net_init();

	/* get the system settings from the host */

	ret = app_events_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "app_events_init failed");
		return ret;
	}

	/* init wifi - this only calls the needed one time functions,
	 * it does not allocates resources or starts the wifi */
	ret = comm_wifi_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "comm_wifi_init failed");
		return ret;
	}

#if 0//(CONFIG_BOARD_C201 || CONFIG_BOARD_C204)
	/* initialize the ble component and start advertising */
	ret = comm_ble_init();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "comm_ble_init failed");
		return ret;
	}

#if CONFIG_STM32_USART_BL_HOST_ENABLE
	/* initialize the stm32 usart bootloader host */
	struct stm32_ubl_funcs funcs;
	funcs.usart_open = host_cmds_send_recv_reinit_for_dfu;
	funcs.usart_send = host_cmds_send_only;
	funcs.usart_recv = host_cmds_recv_bytes;
	funcs.usart_close = NULL;
	funcs.ms_delay = host_cmds_delay;

	ret = stm32_ubl_init(&funcs);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "stm32_ubl_init failed");
		return ret;
	}
#endif
#endif	/*(CONFIG_BOARD_C201 || CONFIG_BOARD_C204)*/

#if 0//CONFIG_CONSOLE_CMDS_APP
	/* initialize and loop for usart based command console */
	ret = console_cmds_init_and_loop();
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "console_cmds_init_and_loop failed");
		return ret;
	}
#endif

//	host_cmds_sensor_list_get();
//
//	vTaskDelay(500 / portTICK_PERIOD_MS);
//
//	uint8_t sens_info[2] = {0x0e, 0x01};
//	host_cmds_sensor_value_getone(sens_info);
//
//	vTaskDelay(1000 / portTICK_PERIOD_MS);
//	host_cmds_sensor_value_getall();

	/* host commands init */
	host_cmds_settings_init();
	host_cmds_wifi_init();

	/* initialize networking apps */
	app_net_http_file_download_init();

	/* initialize firmware update module */
	host_cmds_fw_update_init();

	return ret;
}
