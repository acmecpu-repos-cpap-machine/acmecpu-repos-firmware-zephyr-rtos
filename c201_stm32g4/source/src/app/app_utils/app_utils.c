/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 20-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_utils);

#include "app_utils/app_utils.h"

#define HIGH	1
#define LOW		0

#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
#define TCPC_USB_MUX_DEVNAME		DT_GPIO_LABEL(DT_NODELABEL(usb_mux_sel0), gpios)
#define TCPC_USB_MUX_SEL0			DT_GPIO_PIN(DT_NODELABEL(usb_mux_sel0), gpios)
#define TCPC_USB_MUX_SEL1			DT_GPIO_PIN(DT_NODELABEL(usb_mux_sel1), gpios)
#define FLAGS_USB_MUX_SEL			(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(usb_mux_sel0), gpios))
#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)	// dummy
#define TCPC_USB_MUX_DEVNAME		"dummy"
#define TCPC_USB_MUX_SEL0			1
#define TCPC_USB_MUX_SEL1			2
#define FLAGS_USB_MUX_SEL			0
#else
//#define TCPC_USB_MUX_DEVNAME		"IO_3"
#define TCPC_USB_MUX_SEL0			4
#define TCPC_USB_MUX_SEL1			3
#define FLAGS_USB_MUX_SEL			(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(usb_mux_sel0), gpios))
#endif

int app_utils_usb_channel_select(USB_DATA_CHANNEL channel)
{
	int ret = 0;
#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(io_3));
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(usb_mux_sel0), gpios));
#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	const struct device *dev = NULL;
#endif
	if (dev == NULL) {
		printk("Device not found: %p", dev);
		return -1;
	}
	ret = gpio_pin_configure(dev, TCPC_USB_MUX_SEL0, (GPIO_OUTPUT | FLAGS_USB_MUX_SEL));
	ret |= gpio_pin_configure(dev, TCPC_USB_MUX_SEL1, (GPIO_OUTPUT | FLAGS_USB_MUX_SEL));
	if (ret < 0) {
		printk("gpio_pin_configure failed");
		return ret;
	}

    switch (channel)
	{
	case USB_DATA_CHANNEL_STM32:
		ret = gpio_pin_set(dev, TCPC_USB_MUX_SEL0, LOW);
		ret |= gpio_pin_set(dev, TCPC_USB_MUX_SEL1, LOW);
		break;

	case USB_DATA_CHANNEL_ESP32:
		ret = gpio_pin_set(dev, TCPC_USB_MUX_SEL0, LOW);
		ret |= gpio_pin_set(dev, TCPC_USB_MUX_SEL1, HIGH);
		break;
	
	case USB_DATA_CHANNEL_OTHER:
		ret = gpio_pin_set(dev, TCPC_USB_MUX_SEL0, HIGH);
		ret |= gpio_pin_set(dev, TCPC_USB_MUX_SEL1, LOW);
		break;
	default:
		break;
	}

	if (ret < 0) {
		printk("gpio_pin_set failed");
		return ret;
	}

	return ret;
}

int app_utils_ucpd_i2c_mux_control(APP_UTILS_DEVICE_CONTROL en_dis)
{
	int ret = 0;
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W || CONFIG_BOARD_C208T)
	const struct gpio_dt_spec sel = GPIO_DT_SPEC_GET(DT_NODELABEL(ts3usb221_sel), gpios);
	const struct gpio_dt_spec oe = GPIO_DT_SPEC_GET(DT_NODELABEL(ts3usb221_oe), gpios);

	if (!gpio_is_ready_dt(&sel)) {
		LOG_ERR("gpio %d not ready", sel.pin);
		return -1;
	}
	if (!gpio_is_ready_dt(&oe)) {
		LOG_ERR("gpio %d not ready", oe.pin);
		return -1;
	}

	ret = gpio_pin_configure_dt(&sel, (GPIO_OUTPUT | sel.dt_flags));
	if (ret < 0) {
		LOG_ERR("gpio %d configure failed", sel.pin);
		return -1;
	}
	ret = gpio_pin_configure_dt(&oe, (GPIO_OUTPUT | oe.dt_flags));
	if (ret < 0) {
		LOG_ERR("gpio %d configure failed", oe.pin);
		return -1;
	}

	if (en_dis == APP_UTILS_DEVICE_ENABLE) {
		ret = gpio_pin_set_dt(&sel, LOW);
		ret = gpio_pin_set_dt(&oe, LOW);
	} else if (en_dis == APP_UTILS_DEVICE_DISABLE) {
		ret = gpio_pin_set_dt(&sel, LOW);
		ret = gpio_pin_set_dt(&oe, HIGH);
	}
#endif
	return ret;
}
