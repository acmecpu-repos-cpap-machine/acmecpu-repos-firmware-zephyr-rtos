/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <drivers/gpio.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bsp_load_switch_tps22810);

#define HIGH	1
#define LOW		0

#define LABEL_IO_2					"IO_2"

/* Output pin and flags */
#define PIN_LOAD_SWITCH_EN			15
#define FLAGS_LOAD_SWITCH_EN		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL)

static const struct device* initialize_device(char *label) {
	const struct device *dev;
	dev = device_get_binding(label);
	if (!dev) {
		LOG_ERR("Device driver not found");
		return NULL;
	}
	return dev;
}

static int configure_pin(const struct device *dev, int pin, int flags) {
	int ret = gpio_pin_configure(dev, pin, flags);
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return -1;
	}
	return 0;
}

int tps22810_ps_enable() {
	const struct device *dev = initialize_device(LABEL_IO_2);
	int ret = configure_pin(dev, PIN_LOAD_SWITCH_EN, FLAGS_LOAD_SWITCH_EN);
	if (ret != 0) {
		LOG_ERR("configure_pin, PIN_LOAD_SWITCH_EN failed");
		return -1;
	}
	ret = gpio_pin_set(dev, PIN_LOAD_SWITCH_EN, HIGH);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, PIN_LOAD_SWITCH_EN failed");
		return -1;
	}
	return ret;
}

int tps22810_ps_disable() {
	const struct device *dev = initialize_device(LABEL_IO_2);
	int ret = configure_pin(dev, PIN_LOAD_SWITCH_EN, FLAGS_LOAD_SWITCH_EN);
	if (ret != 0) {
		LOG_ERR("configure_pin, PIN_LOAD_SWITCH_EN failed");
		return -1;
	}
	ret = gpio_pin_set(dev, PIN_LOAD_SWITCH_EN, LOW);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, PIN_LOAD_SWITCH_EN failed");
		return -1;
	}
	return ret;
}
