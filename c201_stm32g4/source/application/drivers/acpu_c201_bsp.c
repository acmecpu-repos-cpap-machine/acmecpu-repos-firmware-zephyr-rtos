/*
 * Copyright (c) 2021 Acme CPU
 */

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/sensor.h>
#include <stdint.h>
#include <stdbool.h>

#include "acpu_c201_bsp.h"
#include "bsp_gpios.h"
#include "acpu_c201_bsp_blower.h"

struct bsp_data
{
	uint8_t dummy;
} data;

struct bsp_cfg
{
	uint8_t dummy;
} cfg;

static struct gpio_callback ui_int_cb_data;
//static struct gpio_callback sensor_int_cb_data;
static struct gpio_callback tamp_int_cb_data;

struct k_sem ui_int_sync;
int counter;

static void bsp_ui_int_cb(const struct device *dev,\
		struct gpio_callback *cb, uint32_t pins)
{
	uint8_t pin = 0;
	switch(pins)
	{
		case BIT(8):
				pin = 8;
			break;
		case BIT(9):
				pin = 9;
			break;
		case BIT(10):
				pin = 10;
			break;
		case BIT(11):
				pin = 11;
			break;
		case BIT(12):
				pin = 12;
			break;
		case BIT(13):
				pin = 13;
			break;
		case BIT(14):
				pin = 14;
			break;
		case BIT(15):
				pin = 15;
			break;
		default:
				pin = 0;
			break;
	}
	if(pin)
	{
		printk("\x1b[KIO_3 Pins#8-15 Int Handler triggered at %" PRIu32 "by pin #%d\n", k_cycle_get_32(), pin);
	}
	else
	{
		printk("\x1b[KIO_3 Pins#8-15 Int Handler triggered by an unknown source at %" PRIu32 "\n", k_cycle_get_32());
	}
	/*
	 * 	We implement a membrane switch event handler from here.
	 * 	There are also other options to get an exact pin which triggered an interrupt
	 * 	like associating different callback functions with different pins.
	 * 	Then, we can either release a specific semaphore, send data into a stream, etc.
	 */

	static uint64_t ref_time = 0;
	uint64_t current_time;
#ifdef CONFIG_DEBUG
	printk("\x1b[KUI Interrupt occurred at %" PRIu64 "\n", (current_time = k_uptime_get()));
#endif
	if ((current_time - ref_time) > 250)
	{
		k_sem_give(&ui_int_sync);
		ref_time = current_time;
	}
}

/*
static void bsp_sense_int_cb(const struct device *dev,\
		struct gpio_callback *cb, uint32_t pins)
{
	printk("Sensor interrupt triggered at %" PRIu32 "\n", k_cycle_get_32());
}
 */

static void bsp_tamp_int_cb(const struct device *dev,\
		struct gpio_callback *cb, uint32_t pins)
{
	printk("Tamper detection interrupt triggered at %" PRIu32 "\n", k_cycle_get_32());
}

static int bsp_gpio_pca95xx_int_cb_config(char *port_name, gpio_port_pins_t pin_mask)
{
	int ret = 0;
	const struct device *pe_dev = device_get_binding(port_name);
	const struct gpio_driver_api *pe_dev_api = pe_dev->api;

	gpio_init_callback(&ui_int_cb_data, bsp_ui_int_cb, pin_mask);
	ret = pe_dev_api->manage_callback(pe_dev, &ui_int_cb_data, 1);
	return ret;
}

static int bsp_exti_cb_config(void)
{
	int ret = 0;
	/*
	 *	Will replace these "magic" numbers and the string with defines
	 *	from the device tree
	 */
	ret |= bsp_gpio_pca95xx_int_cb_config("IO_3",\
			(BIT(8) | BIT(9) | BIT(10) | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15)));
	if (ret)
		printk("Failed to configure UI Interrupt");

	/*	const struct device *sensor_int = device_get_binding("SENSOR INT");
	const struct bsp_gpios_cfg *sensor_int_cfg = sensor_int->config;
	const struct bsp_gpios_data *sensor_int_data = sensor_int->data;
	const struct device *sensor_int_port = sensor_int_data->port;

	gpio_init_callback(&sensor_int_cb_data, bsp_sense_int_cb, BIT(sensor_int_cfg->pin));
	ret |= gpio_add_callback(sensor_int_port, &sensor_int_cb_data);
	if (ret)
		printk("Failed to configure Sensor Interrupt");
	 */

	const struct device *tamp_int = device_get_binding("TAMP DET");
	const struct bsp_gpios_cfg *tamp_int_cfg = tamp_int->config;
	const struct bsp_gpios_data *tamp_int_data = tamp_int->data;
	const struct device *tamp_int_port = tamp_int_data->port;

	gpio_init_callback(&tamp_int_cb_data, bsp_tamp_int_cb, BIT(tamp_int_cfg->pin));
	ret |= gpio_add_callback(tamp_int_port, &tamp_int_cb_data);
	if (ret)
		printk("Failed to configure Tamper Detection Interrupt");

	return ret;
}

static int bsp_led_ind_control(const struct device *dev, uint16_t control)
{
	return 0;
}

static int bsp_sys_alive(void)
{
	static bool state = false;
	const struct device *pwm_led = device_get_binding("UI_LEDS");
	const struct led_driver_api *pwm_led_api = pwm_led->api;
	state = (state ? false : true);
	if (state)
		pwm_led_api->on(pwm_led, 1);
	else
		pwm_led_api->off(pwm_led, 1);
	return 0;
}

static void bsp_serial_debug_output(void)
{
	//extern struct sensor_value temp_value;

	printk("\x1b[0;0H");
	printk("\x1b[KUptime %d\n", counter++);
	//printk("\x1b[KTemperature\x1b[32m %d.%d\x1b[0m C\n", temp_value.val1, temp_value.val2 / 10000);

	return;
}

static int bsp_init(const struct device *dev)
{
	int ret = 0;
	ret |= bsp_exti_cb_config();
	return ret;
}

const struct bsp_driver_api bsp_drv = {
		.led_ind_ctrl = bsp_led_ind_control,
		.sys_alive = bsp_sys_alive,
		.gpio_int_cb_cfg = bsp_exti_cb_config,
		.debug_output = bsp_serial_debug_output,
};


DEVICE_DEFINE(acpu_bsp, "acpu_c201_bsp_driver", bsp_init, NULL,\
		&data, &cfg,\
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,\
		&bsp_drv);
