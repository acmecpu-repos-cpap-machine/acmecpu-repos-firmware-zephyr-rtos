/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 21-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#define DT_DRV_COMPAT acpu_h205cpower

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_CMX655D_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(h205c_power);

#include "h205c_power.h"

#define HIGH	1
#define LOW		0

/** Configuration data */
struct h205c_power_config {
	/* 3v3 enable pin definition */
	// const char *gpio_port_3v3;
	// gpio_pin_t gpio_pin_3v3;
	// gpio_flags_t gpio_flags_3v3;
    struct gpio_dt_spec gpio_3v3;

	/* 5v enable pin definition */
	// const char *gpio_port_5v;
	// gpio_pin_t gpio_pin_5v;
	// gpio_flags_t gpio_flags_5v;
    struct gpio_dt_spec gpio_5v;

	/* 1v8 enable pin definition */
	// const char *gpio_port_1v8;
	// gpio_pin_t gpio_pin_1v8;
	// gpio_flags_t gpio_flags_1v8;
    struct gpio_dt_spec gpio_1v8;
};

// struct h205c_power_data {
// 	const struct device *dev_3v3;
//     const struct device *dev_5v;
//     const struct device *dev_1v8;
// };

static int h205c_power_init(const struct device *dev)
{
	const struct h205c_power_config *config = dev->config;
	// struct h205c_power_data *drv_data = dev->data;
	int ret = 0;

    // k_sleep(K_MSEC(1000));
    /* Enable 3V3 power */
   	if (!device_is_ready(config->gpio_3v3.port)) {
		LOG_ERR("gpio_3v3 device is not ready");
		return -ENODEV;
    }
	// drv_data->dev_3v3 = device_get_binding(config->gpio_port_3v3);
	// if (drv_data->dev_3v3 != NULL) {
	    ret = gpio_pin_configure(config->gpio_3v3.port, config->gpio_3v3.pin, 
                (GPIO_OUTPUT | GPIO_PUSH_PULL | config->gpio_3v3.dt_flags));
        if (ret == 0)
	        ret = gpio_pin_set(config->gpio_3v3.port, config->gpio_3v3.pin, LOW);
        else
            return ret;
    // }
    // k_sleep(K_MSEC(1000));
    /* Enable 5V power */
   	if (!device_is_ready(config->gpio_5v.port)) {
		LOG_ERR("gpio_5v device is not ready");
		return -ENODEV;
    }
	// drv_data->dev_5v = device_get_binding(config->gpio_port_5v);
	// if (drv_data->dev_5v != NULL) {
	    ret = gpio_pin_configure(config->gpio_5v.port, config->gpio_5v.pin, 
                (GPIO_OUTPUT | GPIO_PUSH_PULL | config->gpio_5v.dt_flags));
        if (ret == 0)
	        ret = gpio_pin_set(config->gpio_5v.port, config->gpio_5v.pin, HIGH);
        else
            return ret;
    // }
    // k_sleep(K_MSEC(1000));
    /* Enable 1V8 power */
   	if (!device_is_ready(config->gpio_1v8.port)) {
		LOG_ERR("gpio_1v8 device is not ready");
		return -ENODEV;
    }
	// drv_data->dev_1v8 = device_get_binding(config->gpio_port_1v8);
	// if (drv_data->dev_1v8 != NULL) {
	    ret = gpio_pin_configure(config->gpio_1v8.port, config->gpio_1v8.pin, 
                (GPIO_OUTPUT | GPIO_PUSH_PULL | config->gpio_1v8.dt_flags));
        if (ret == 0)
	        ret = gpio_pin_set(config->gpio_1v8.port, config->gpio_1v8.pin, HIGH);
        else
            return ret;
    // }

    return ret;
}

static const struct h205c_power_driver_api driver_api = {
};

#define DEVICE_INSTANCE(inst) \
\
const static struct h205c_power_config h205c_power_##inst##_cfg = { \
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, en3v3_gpios), (	\
        /*.gpio_port_3v3 = DT_INST_GPIO_LABEL(inst, en3v3_gpios),	\
        .gpio_pin_3v3 = DT_INST_GPIO_PIN(inst, en3v3_gpios),	\
        .gpio_flags_3v3 = DT_INST_GPIO_FLAGS(inst, en3v3_gpios),*/	\
        .gpio_3v3 = GPIO_DT_SPEC_INST_GET(inst, en3v3_gpios), \
	))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, en5v_gpios), (	\
        /*.gpio_port_5v = DT_INST_GPIO_LABEL(inst, en5v_gpios),	\
        .gpio_pin_5v = DT_INST_GPIO_PIN(inst, en5v_gpios),	\
        .gpio_flags_5v = DT_INST_GPIO_FLAGS(inst, en5v_gpios),*/	\
        .gpio_5v = GPIO_DT_SPEC_INST_GET(inst, en5v_gpios), \
    ))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, en1v8_gpios), (	\
        /*.gpio_port_1v8 = DT_INST_GPIO_LABEL(inst, en1v8_gpios),	\
        .gpio_pin_1v8 = DT_INST_GPIO_PIN(inst, en1v8_gpios),	\
        .gpio_flags_1v8 = DT_INST_GPIO_FLAGS(inst, en1v8_gpios),*/	\
        .gpio_1v8 = GPIO_DT_SPEC_INST_GET(inst, en1v8_gpios), \
    ))								\
};\
\
/*static struct h205c_power_data h205c_power_##inst##_drvdata = { \
};*/ \
\
DEVICE_DT_INST_DEFINE(inst,								\
		h205c_power_init,									\
		device_pm_control_nop,							\
		/*&h205c_power_##inst##_drvdata,*/						\
        NULL, \
		&h205c_power_##inst##_cfg,							\
		APPLICATION, CONFIG_H205C_POWER_INIT_PRIORITY,		\
		&driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
