/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/drivers/led.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_led);

#include "acpu_c201_modules.h"
#include "bsp_led/bsp_led_ntf.h"
//#include "bsp_leds.h"

int bsp_led_show_notification(uint8_t led_idx, struct bsp_led_config *led_config)
{
	int ret=0;
#if 0
	/* Extract the LED driver device and led index from the LED friendly name */
	const struct device *dev = device_get_binding(led_label);
	if (!dev) {
		LOG_ERR("LED device %s not found", led_label);
		return -1;
	}
	const struct bsp_leds_cfg *cfg = dev->config;
	const struct device *led_drv_dev = device_get_binding(cfg->led_drv_name);
	if (!led_drv_dev) {
		LOG_ERR("LED DRV device %s not found", cfg->led_drv_name);
		return -1;
	}
	uint32_t led_idx = cfg->led_idx;
#endif

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201 || CONFIG_BOARD_C205)
	const struct device *const led_drv_dev = DEVICE_DT_GET_ANY(nxp_pca9632);
#endif

	uint8_t ntf_type = led_config->ntf_type;
	uint8_t led_state = led_config->led_state;

	if ((ntf_type & BSP_LED_NTF_BRIGHTNESS) == BSP_LED_NTF_BRIGHTNESS) {
		ret = led_set_brightness(led_drv_dev, led_idx, led_config->brightness);
	}

	if ((ntf_type & BSP_LED_NTF_NORMAL) == BSP_LED_NTF_NORMAL) {
		if (led_state) {
			ret = led_on(led_drv_dev, led_idx);
		} else {
			ret = led_off(led_drv_dev, led_idx);
		}
	}

	if ((ntf_type & BSP_LED_NTF_BLINK) == BSP_LED_NTF_BLINK) {
		ret = led_blink(led_drv_dev, led_idx, led_config->blink_delay_on_ms, led_config->blink_delay_off_ms);
	}

	return ret;
}

