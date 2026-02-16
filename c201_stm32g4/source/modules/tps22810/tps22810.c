/*
 * Copyright (c) 2021 Acme CPU
 */
#define DT_DRV_COMPAT ti_tps22810

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TPS22810, CONFIG_TPS22810_LOG_LEVEL);

#include "tps22810.h"

struct tps22810_config {
	/* Enable pin definition */
//	const char *en_gpio_port;
//	gpio_pin_t en_gpio_pin;
//	gpio_flags_t en_gpio_flags;
	struct gpio_dt_spec en_gpio;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
	/* audio PS pin */
//	const char *aud_gpio_port;
//	gpio_pin_t aud_gpio_pin;
//	gpio_flags_t aud_gpio_flags;
	struct gpio_dt_spec aud_gpio;
#endif /*(CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)*/
};

struct tps22810_data {
	const struct device *switch_dev;
};

static int tps22810_enable(const struct device *dev) {
//	struct tps22810_data *data = dev->data;
	const struct tps22810_config *config = dev->config;
	int ret = 0;

	/* Configure GPIO output enable pin */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev != NULL) {
	if (device_is_ready(config->en_gpio.port)) {
		ret |= gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin,
				(config->en_gpio.dt_flags | GPIO_OUTPUT));

		/* Enable the device */
		ret |= gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->en_gpio.pin, ret);
			return ret;
		}
	} else {
		LOG_ERR("Could not find en_gpio device %d", ret);
		ret = -ENODEV;
	}
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
	/* Configure GPIO output audio PS enable pin */
//	const struct device *aud_gpio_dev = device_get_binding(config->aud_gpio_port);
//	if (aud_gpio_dev != NULL) {
	if (device_is_ready(config->aud_gpio.port)) {
		ret |= gpio_pin_configure(config->aud_gpio.port, config->aud_gpio.pin,
				(config->aud_gpio.dt_flags | GPIO_OUTPUT));

		/* Enable the device */
		ret |= gpio_pin_set(config->aud_gpio.port, config->aud_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->aud_gpio.pin, ret);
			return ret;
		}
	} else {
		LOG_ERR("Could not find en_gpio device %d", ret);
		ret = -ENODEV;
	}
#endif	/* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205) */
	return ret;
}

static int tps22810_disable(const struct device *dev) {
//	struct tps22810_data *data = dev->data;
	const struct tps22810_config *config = dev->config;
	int ret = 0;

	/* Configure GPIO output enable pin */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev != NULL) {
	if (device_is_ready(config->en_gpio.port)) {
		ret |= gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin,
				(config->en_gpio.dt_flags | GPIO_OUTPUT));

		/* Disable the device */
		ret |= gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->en_gpio.pin, ret);
			return ret;
		}
	}
	return ret;
}

static int tps22810_init(const struct device *dev) {
	int ret = 0;
	ret = tps22810_enable(dev);
	if (!ret) {
		/* we wait for the load power become available and stable */
		k_sleep(K_MSEC(1000));
	}
	return ret;
}

struct tps22810_driver_api tps22810_api = {
	.enable = tps22810_enable,
	.disable = tps22810_disable,
};
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
#define DEVICE_INSTANCE(inst)	\
const static struct tps22810_config tps22810_##inst##_config = {		\
		/*.en_gpio_port 	= DT_INST_GPIO_LABEL(inst, enable_gpios),		\
		.en_gpio_pin 	= DT_INST_GPIO_PIN(inst, enable_gpios),			\
		.en_gpio_flags	= DT_INST_GPIO_FLAGS(inst, enable_gpios),		*/\
		.en_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios), 			\
		/*.aud_gpio_port 	= DT_INST_GPIO_LABEL(inst, audio_gpios),		\
		.aud_gpio_pin 	= DT_INST_GPIO_PIN(inst, audio_gpios),			\
		.aud_gpio_flags	= DT_INST_GPIO_FLAGS(inst, audio_gpios)		*/\
		.aud_gpio = GPIO_DT_SPEC_INST_GET(inst, audio_gpios), 			\
};																		\
																		\
static struct tps22810_data tps22810_##inst##_data;						\
																		\
DEVICE_DT_INST_DEFINE(inst, tps22810_init, device_pm_control_nop, 		\
						&tps22810_##inst##_data,						\
						&tps22810_##inst##_config,						\
						POST_KERNEL, CONFIG_TPS22810_INIT_PRIORITY,		\
						&tps22810_api);
#else
#define DEVICE_INSTANCE(inst)	\
const static struct tps22810_config tps22810_##inst##_config = {		\
		/*.en_gpio_port 	= DT_INST_GPIO_LABEL(inst, enable_gpios),		\
		.en_gpio_pin 	= DT_INST_GPIO_PIN(inst, enable_gpios),			\
		.en_gpio_flags	= DT_INST_GPIO_FLAGS(inst, enable_gpios),		*/\
		.en_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios), 			\
};																		\
																		\
static struct tps22810_data tps22810_##inst##_data;						\
																		\
DEVICE_DT_INST_DEFINE(inst, tps22810_init, device_pm_control_nop, 		\
						&tps22810_##inst##_data,						\
						&tps22810_##inst##_config,						\
						POST_KERNEL, CONFIG_TPS22810_INIT_PRIORITY,		\
						&tps22810_api);
#endif /* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205) */

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
