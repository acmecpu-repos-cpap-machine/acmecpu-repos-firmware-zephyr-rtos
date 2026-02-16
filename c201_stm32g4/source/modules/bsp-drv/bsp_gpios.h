/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_BSP_DRV_BSP_GPIOS_H_
#define MODULES_BSP_DRV_BSP_GPIOS_H_

#include <device.h>
#include <drivers/gpio.h>

struct bsp_gpios_cfg {
	const char *port_name;
	gpio_pin_t pin;
	gpio_flags_t flags;
	bool gpio_pca95xx_compat;
};

struct bsp_gpios_data {
	const struct device *port;
};

typedef int (*bsp_gpio_config_t)(const struct device *);
typedef int (*bsp_gpio_set_t)(const struct device *);
typedef int (*bsp_gpio_reset_t)(const struct device *);
typedef int (*bsp_gpio_toggle_t)(const struct device *);

struct bsp_gpio_driver_api {
	bsp_gpio_config_t config;
	bsp_gpio_set_t set;
	bsp_gpio_reset_t reset;
	bsp_gpio_toggle_t toggle;
};


#endif /* MODULES_BSP_DRV_BSP_GPIOS_H_ */
