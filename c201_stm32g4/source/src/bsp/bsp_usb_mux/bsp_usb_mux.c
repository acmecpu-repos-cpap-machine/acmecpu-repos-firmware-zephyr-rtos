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
LOG_MODULE_REGISTER(bsp_usb_mux);

#define HIGH	1
#define LOW		0

#if (CONFIG_BOARD_C204_CORE)
#define TCPC_USB_MUX_DEVNAME		DT_GPIO_LABEL(DT_NODELABEL(usb_mux_sel0), gpios)
#define TCPC_USB_MUX_SEL0			DT_GPIO_PIN(DT_NODELABEL(usb_mux_sel0), gpios)
#define TCPC_USB_MUX_SEL1			DT_GPIO_PIN(DT_NODELABEL(usb_mux_sel1), gpios)
#define FLAGS_USB_MUX_SEL			(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(usb_mux_sel0), gpios))
#else
#define TCPC_USB_MUX_DEVNAME					"IO_3"
#define TCPC_USB_MUX_SEL0			4
#define TCPC_USB_MUX_SEL1			3
#define FLAGS_USB_MUX_SEL			(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL)
#endif

//static const struct device* initialize_device(char *label) {
//	const struct device *dev;
//	dev = device_get_binding(label);
//	if (!dev) {
//		LOG_ERR("Device driver not found");
//		return NULL;
//	}
//	return dev;
//}

static int configure_pin(const struct device *dev, int pin, int flags) {
	int ret = gpio_pin_configure(dev, pin, flags);
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return -1;
	}
	return 0;
}

/*
 * This function selects the USB1 port of T3USB3031 chip.
 * On the C201 board the USB1 port is connected to the
 * D+/D- lines of STM32
 * */
int bsp_usb_mux_select_usb1() {
//	const struct device *dev_dio_1 = initialize_device(TCPC_USB_MUX_DEVNAME);
	const struct device *dev_dio_1 = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(usb_mux_sel0), gpios));
	int ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL0, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL1, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL1, LOW);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL0, LOW);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	return ret;
}

/*
 * This function selects the USB2 port of T3USB3031 chip.
 * On the C201 board the USB1 port is connected to the
 * D+/D- lines of BG95
 * */
int bsp_usb_mux_select_usb2() {
//	const struct device *dev_dio_1 = initialize_device(TCPC_USB_MUX_DEVNAME);
	const struct device *dev_dio_1 = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(usb_mux_sel0), gpios));
	int ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL0, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL1, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL1, LOW);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL0, HIGH);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	return ret;
}

/*
 * This function selects the MHL port of T3USB3031 chip.
 * On the C201 board the MHL port is connected to the
 * CP2102 D+/D- lines which bridges the ESP32 UART pins
 * */
int bsp_usb_mux_select_mhl() {
//	const struct device *dev_dio_1 = initialize_device(TCPC_USB_MUX_DEVNAME);
	const struct device *dev_dio_1 = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(usb_mux_sel0), gpios));
	int ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL0, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	ret = configure_pin(dev_dio_1, TCPC_USB_MUX_SEL1, FLAGS_USB_MUX_SEL);
	if (ret != 0) {
		LOG_ERR("configure_pin, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL1, HIGH);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL1 failed");
		return -1;
	}
	ret = gpio_pin_set(dev_dio_1, TCPC_USB_MUX_SEL0, LOW);
	if (ret != 0) {
		LOG_ERR("gpio_pin_set, TCPC_USB_MUX_SEL0 failed");
		return -1;
	}
	return ret;
}
