/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_display);

#include "acpu_c201_modules.h"
#if CONFIG_TPS61169
#include "tps61169.h"
#endif

static uint8_t m_bright_percent = 100;

int bsp_display_init() {
	int ret = 0;

	return ret;
}

int bsp_display_set_brightness(uint8_t percent) {
	int ret = 0;

	const struct device *disp_pwr = device_get_binding(ACPU_C201_MOD_NAME_DISPLAY_PWR);
	if (disp_pwr == NULL) {
		LOG_ERR("cannot find device %s", ACPU_C201_MOD_NAME_DISPLAY_PWR);
		return -1;
	}
#if CONFIG_TPS61169
	const struct tps61169_driver_api *api = disp_pwr->api;
	ret = api->tps61169_set_brightness(disp_pwr, percent);
#endif


	if (!ret)
		m_bright_percent = percent;

	return ret;
}

int bsp_display_on() {
#if CONFIG_TPS61169
	return bsp_display_set_brightness(m_bright_percent);
#else
	return 0;
#endif
}

int bsp_display_off() {
#if CONFIG_TPS61169
	return bsp_display_set_brightness(0);
#else
	return 0;
#endif
}

