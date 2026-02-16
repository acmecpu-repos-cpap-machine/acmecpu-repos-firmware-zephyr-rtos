/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 30-Jun-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#define DT_DRV_COMPAT acpu_pwrctrldrv

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PWR_CTRL_DRV, CONFIG_PWR_CTRL_DRV_LOG_LEVEL);

#include "power_control_driver.h"

#define HIGH	1
#define LOW		0

struct power_control_driver_config {
#if CONFIG_PWR_CTRL_DRV_SUPPLY_1
	struct gpio_dt_spec ps1_gpio;
#endif
#if CONFIG_PWR_CTRL_DRV_SUPPLY_2
	struct gpio_dt_spec ps2_gpio;
#endif
#if CONFIG_PWR_CTRL_DRV_SUPPLY_3
	struct gpio_dt_spec ps3_gpio;
#endif
#if CONFIG_PWR_CTRL_DRV_SUPPLY_4
	struct gpio_dt_spec ps4_gpio;
#endif
#if CONFIG_PWR_CTRL_DRV_SUPPLY_5
	struct gpio_dt_spec ps5_gpio;
#endif
};

static int power_control_driver_init(const struct device *dev)
{
	const struct power_control_driver_config *config = dev->config;
	int ret = 0;

    /* Enable / disable power supply 1 */
#if (CONFIG_PWR_CTRL_DRV_SUPPLY_1)
   	if (!device_is_ready(config->ps1_gpio.port)) {
		LOG_ERR("ps1_gpio device is not ready");
		return -ENODEV;
    }
	ret = gpio_pin_configure(config->ps1_gpio.port, config->ps1_gpio.pin,
			(GPIO_OUTPUT | GPIO_PUSH_PULL | config->ps1_gpio.dt_flags));
	if (ret == 0) {
#if (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_1 == 1)
		ret = gpio_pin_set(config->ps1_gpio.port, config->ps1_gpio.pin, HIGH);
#else
		ret = gpio_pin_set(config->ps1_gpio.port, config->ps1_gpio.pin, LOW);
#endif	/* (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_1 == 1) */
	}
	else {
		LOG_ERR("ps1_gpio gpio_pin_configure failed");
		return ret;
    }
#endif /* (CONFIG_PWR_CTRL_DRV_SUPPLY_1) */

    /* Enable / disable power supply 2 */
#if (CONFIG_PWR_CTRL_DRV_SUPPLY_2)
   	if (!device_is_ready(config->ps2_gpio.port)) {
		LOG_ERR("ps2_gpio device is not ready");
		return -ENODEV;
    }
	ret = gpio_pin_configure(config->ps2_gpio.port, config->ps2_gpio.pin,
			(GPIO_OUTPUT | GPIO_PUSH_PULL | config->ps2_gpio.dt_flags));
	if (ret == 0) {
#if (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_2 == 1)
		ret = gpio_pin_set(config->ps2_gpio.port, config->ps2_gpio.pin, HIGH);
#else
		ret = gpio_pin_set(config->ps2_gpio.port, config->ps2_gpio.pin, LOW);
#endif	/* (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_2 == 1) */
	}
	else {
		LOG_ERR("ps2_gpio gpio_pin_configure failed");
		return ret;
    }
#endif /* (CONFIG_PWR_CTRL_DRV_SUPPLY_2) */

    /* Enable / disable power supply 3 */
#if (CONFIG_PWR_CTRL_DRV_SUPPLY_3)
   	if (!device_is_ready(config->ps3_gpio.port)) {
		LOG_ERR("ps3_gpio device is not ready");
		return -ENODEV;
    }
	ret = gpio_pin_configure(config->ps3_gpio.port, config->ps3_gpio.pin,
			(GPIO_OUTPUT | GPIO_PUSH_PULL | config->ps3_gpio.dt_flags));
	if (ret == 0) {
#if (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_3 == 1)
		ret = gpio_pin_set(config->ps3_gpio.port, config->ps3_gpio.pin, HIGH);
#else
		ret = gpio_pin_set(config->ps3_gpio.port, config->ps3_gpio.pin, LOW);
#endif	/* (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_3 == 1) */
	}
	else {
		LOG_ERR("ps3_gpio gpio_pin_configure failed");
		return ret;
    }
#endif /* (CONFIG_PWR_CTRL_DRV_SUPPLY_3) */

    /* Enable / disable power supply 4 */
#if (CONFIG_PWR_CTRL_DRV_SUPPLY_4)
   	if (!device_is_ready(config->ps4_gpio.port)) {
		LOG_ERR("ps4_gpio device is not ready");
		return -ENODEV;
    }
	ret = gpio_pin_configure(config->ps4_gpio.port, config->ps4_gpio.pin,
			(GPIO_OUTPUT | GPIO_PUSH_PULL | config->ps4_gpio.dt_flags));
	if (ret == 0) {
#if (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_4 == 1)
		ret = gpio_pin_set(config->ps4_gpio.port, config->ps4_gpio.pin, HIGH);
#else
		ret = gpio_pin_set(config->ps4_gpio.port, config->ps4_gpio.pin, LOW);
#endif	/* (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_4 == 1) */
	}
	else {
		LOG_ERR("ps4_gpio gpio_pin_configure failed");
		return ret;
    }
#endif /* (CONFIG_PWR_CTRL_DRV_SUPPLY_4) */

    /* Enable / disable power supply 5 */
#if (CONFIG_PWR_CTRL_DRV_SUPPLY_5)
   	if (!device_is_ready(config->ps5_gpio.port)) {
		LOG_ERR("ps5_gpio device is not ready");
		return -ENODEV;
    }
	ret = gpio_pin_configure(config->ps5_gpio.port, config->ps5_gpio.pin,
			(GPIO_OUTPUT | GPIO_PUSH_PULL | config->ps5_gpio.dt_flags));
	if (ret == 0) {
#if (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_5 == 1)
		ret = gpio_pin_set(config->ps5_gpio.port, config->ps5_gpio.pin, HIGH);
#else
		ret = gpio_pin_set(config->ps5_gpio.port, config->ps5_gpio.pin, LOW);
#endif	/* (CONFIG_PWR_CTRL_DRV_CONTROL_SUPPLY_5 == 1) */
	}
	else {
		LOG_ERR("ps5_gpio gpio_pin_configure failed");
		return ret;
    }
#endif /* (CONFIG_PWR_CTRL_DRV_SUPPLY_5) */

	k_sleep(K_MSEC(100));
    return ret;
}

static const struct power_control_driver_api driver_api = {
};

#define DEVICE_INSTANCE(inst) \
\
const static struct power_control_driver_config power_ctrl_##inst##_cfg = { \
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ps1_gpios), (		\
        .ps1_gpio = GPIO_DT_SPEC_INST_GET(inst, ps1_gpios), 	\
	))															\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ps2_gpios), (		\
        .ps2_gpio = GPIO_DT_SPEC_INST_GET(inst, ps2_gpios), 	\
    ))															\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ps3_gpios), (		\
        .ps3_gpio = GPIO_DT_SPEC_INST_GET(inst, ps3_gpios), 	\
    ))															\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ps4_gpios), (		\
        .ps4_gpio = GPIO_DT_SPEC_INST_GET(inst, ps4_gpios), 	\
    ))															\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ps5_gpios), (		\
        .ps5_gpio = GPIO_DT_SPEC_INST_GET(inst, ps3_gpios), 	\
    ))															\
};\
\
DEVICE_DT_INST_DEFINE(inst,								\
		power_control_driver_init,						\
		device_pm_control_nop,							\
        NULL, 											\
		&power_ctrl_##inst##_cfg,						\
		POST_KERNEL, CONFIG_PWR_CTRL_DRV_INIT_PRIORITY,	\
		&driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
