/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 20-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#include "app_utils/app_utils.h"

#define HIGH	1
#define LOW		0

/* en_3v3 */
#define PWR_EN_3V3_DEV_NAME 	DT_PROP(DT_NODELABEL(io_1), label)
#define PWR_EN_3V3_PIN			DT_GPIO_PIN(DT_NODELABEL(en_3v3), gpios)
#define PWR_EN_3V3_FLAGS		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(en_3v3), gpios))

/* en_5v */
#define PWR_EN_5V_DEV_NAME 	DT_PROP(DT_NODELABEL(io_1), label)
#define PWR_EN_5V_PIN			DT_GPIO_PIN(DT_NODELABEL(en_5v), gpios)
#define PWR_EN_5V_FLAGS		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(en_5v), gpios))

/* en_1v8 */
#define PWR_EN_1V8_DEV_NAME 	DT_PROP(DT_NODELABEL(io_1), label)
#define PWR_EN_1V8_PIN			DT_GPIO_PIN(DT_NODELABEL(en_1v8), gpios)
#define PWR_EN_1V8_FLAGS		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(en_1v8), gpios))

/* usb_dsel */
#define USB_DSEL_DEV_NAME 	DT_GPIO_LABEL(DT_NODELABEL(usb_dsel), gpios)
#define USB_DSEL_PIN		DT_GPIO_PIN(DT_NODELABEL(usb_dsel), gpios)
#define USB_DSEL_FLAGS		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(usb_dsel), gpios))

#if 0
int app_utils_power_enable()
{
	int ret = 0;

	/* Enable 3V3 power */
	const struct device *dev_3v3 = device_get_binding(PWR_EN_3V3_DEV_NAME);
	if (dev_3v3 == NULL) {
		printk("Device not found: %s", PWR_EN_3V3_DEV_NAME);
		return -1;
	}
	ret = gpio_pin_configure(dev_3v3, PWR_EN_3V3_PIN, PWR_EN_3V3_FLAGS);
	if (ret < 0) {
		printk("gpio_pin_configure failed");
		return ret;
	}
	ret = gpio_pin_set(dev_3v3, PWR_EN_3V3_PIN, LOW);
	if (ret < 0) {
		printk("gpio_pin_set failed");
		return ret;
	}

	/* Enable 5V power */
	const struct device *dev_5v = device_get_binding(PWR_EN_5V_DEV_NAME);
	if (dev_5v == NULL) {
		printk("Device not found: %s", PWR_EN_5V_DEV_NAME);
		return -1;
	}
	ret = gpio_pin_configure(dev_5v, PWR_EN_5V_PIN, PWR_EN_5V_FLAGS);
	if (ret < 0) {
		printk("gpio_pin_configure failed");
		return ret;
	}
	ret = gpio_pin_set(dev_5v, PWR_EN_5V_PIN, LOW);
	if (ret < 0) {
		printk("gpio_pin_set failed");
		return ret;
	}

	/* Enable 1V8 power */
	const struct device *dev_1v8 = device_get_binding(PWR_EN_1V8_DEV_NAME);
	if (dev_1v8 == NULL) {
		printk("Device not found: %s", PWR_EN_1V8_DEV_NAME);
		return -1;
	}
	ret = gpio_pin_configure(dev_1v8, PWR_EN_1V8_PIN, PWR_EN_1V8_FLAGS);
	if (ret < 0) {
		printk("gpio_pin_configure failed");
		return ret;
	}
	ret = gpio_pin_set(dev_1v8, PWR_EN_1V8_PIN, HIGH);
	if (ret < 0) {
		printk("gpio_pin_set failed");
		return ret;
	}

	return ret;
}
#endif

int app_utils_usb_channel_select(USB_DATA_CHANNEL channel)
{
	int ret = 0;
	// const struct device *dev = device_get_binding(USB_DSEL_DEV_NAME);
	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(usb_dsel), gpios));
	if (dev == NULL) {
		// printk("Device not found: %s", USB_DSEL_DEV_NAME);
		printk("Device not found: %p", dev);
		return -1;
	}
	ret = gpio_pin_configure(dev, USB_DSEL_PIN, (GPIO_OUTPUT | USB_DSEL_FLAGS));
	if (ret < 0) {
		printk("gpio_pin_configure failed");
		return ret;
	}

    switch (channel)
	{
	case USB_DATA_CHANNEL_CHARGER:
		ret = gpio_pin_set(dev, USB_DSEL_PIN, LOW);
		if (ret < 0) {
			printk("gpio_pin_set failed");
			return ret;
		}
		break;
	
	case USB_DATA_CHANNEL_HOST:
		ret = gpio_pin_set(dev, USB_DSEL_PIN, HIGH);
		if (ret < 0) {
			printk("gpio_pin_set failed");
			return ret;
		}
		break;

	default:
		break;
	}

	return ret;
}