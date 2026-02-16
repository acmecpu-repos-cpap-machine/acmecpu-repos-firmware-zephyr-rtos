/*
 * Copyright (c) 2021 Acme CPU
 *
 */

#ifndef MODULES_BSP_LEDS_BSP_LEDS_H_
#define MODULES_BSP_LEDS_BSP_LEDS_H_

#include <zephyr.h>
#include <device.h>
#include <stdint.h>

struct bsp_leds_cfg {
	const char *led_drv_name;
	uint32_t led_idx;
};

struct bsp_leds_data {
	const struct device *port;
};

typedef int (*bsp_leds_config_t)(const struct device *);

struct bsp_leds_driver_api {
	bsp_leds_config_t config;
};

#endif /* MODULES_BSP_LEDS_BSP_LEDS_H_ */
