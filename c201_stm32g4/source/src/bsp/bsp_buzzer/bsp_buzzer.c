/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsp_buzzer);

#include "acpu_c201_modules.h"
#if (CONFIG_BUZZER_GENERIC)
#include "buzzer_generic.h"
#endif

void bsp_buzzer_play_switch_pressed() {
	/* TODO check the keypad/buzzer_beep settings */
#if (CONFIG_BUZZER_GENERIC)
	const struct device *buzzer = device_get_binding(ACPU_C201_MOD_NAME_BUZZER);
	if (buzzer == NULL) {
		LOG_ERR("cannot find device %s", ACPU_C201_MOD_NAME_BUZZER);
		return;
	}
	const struct buzzer_driver_api *api = buzzer->api;

	api->buzzer_on(buzzer);
	k_sleep(K_MSEC(30));
	api->buzzer_off(buzzer);
#endif
}

void bsp_buzzer_on() {
#if (CONFIG_BUZZER_GENERIC)
	const struct device *buzzer = device_get_binding(ACPU_C201_MOD_NAME_BUZZER);
	if (buzzer == NULL) {
		LOG_ERR("cannot find device %s", ACPU_C201_MOD_NAME_BUZZER);
		return;
	}
	const struct buzzer_driver_api *api = buzzer->api;

	api->buzzer_on(buzzer);
#endif
}

void bsp_buzzer_off() {
#if (CONFIG_BUZZER_GENERIC)
	const struct device *buzzer = device_get_binding(ACPU_C201_MOD_NAME_BUZZER);
	if (buzzer == NULL) {
		LOG_ERR("cannot find device %s", ACPU_C201_MOD_NAME_BUZZER);
		return;
	}
	const struct buzzer_driver_api *api = buzzer->api;

	api->buzzer_off(buzzer);
#endif
}
