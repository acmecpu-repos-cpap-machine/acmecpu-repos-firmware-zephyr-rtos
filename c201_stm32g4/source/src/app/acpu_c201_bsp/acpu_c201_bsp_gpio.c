/*
 * acpu_c201_bsp.h
 *
 * Created on: Feb 23, 2021
 *      Author: Rohan Dey
 */

#include <drivers/gpio.h>
#include <drivers/led.h>
#include <drivers/sensor.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bsp_gpios.h"
#include "acpu_c201_bsp_gpio.h"
//#include "acpu_c201_bsp_blower.h"
#include "acpu_c201_bsp_gpio_config.h"

typedef struct {
	const char *port_name;
	gpio_port_pins_t pin_mask;
	acpu_c201_bsp_gpio_callback_handler_t cb;
} bsp_gpio_data;

static bsp_gpio_data m_gpio_data[ACPU_GPIO_MAX_CB];
static uint8_t m_gpio_data_idx = 0;

static struct gpio_callback m_dio3_int_cb_data;
//static struct gpio_callback m_sensor_int_cb_data;
static struct gpio_callback m_tamp_int_cb_data;

struct k_sem ui_int_sync;
int counter;

#if 1
static void bsp_dio3_int_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {

	uint32_t port_val = 0;
	int ret = 0, idx;

//	const struct device *dev_dio = device_get_binding(ACPU_GPIO_LABEL_DIO_3);
//	ret |= gpio_port_get(dev_dio, &port_val);

	for (idx = 0; idx < ACPU_GPIO_MAX_CB; idx++) {

		if ((m_gpio_data[idx].port_name) == NULL || (m_gpio_data[idx].cb == NULL)) {
			continue;
		}

		/* Check if the port name matches */
		if (!strcmp(m_gpio_data[idx].port_name, ACPU_GPIO_LABEL_DIO_3)) {

			/* Check if any pin(s) from the pinmask is true */
//			if (port_val & m_gpio_data[idx].pin_mask) {

				/* Call the relevant callback and pass the pin mask */
				(m_gpio_data[idx].cb)(port_val);
//			}
			break;
		}
	}
}

#else
static void bsp_dio3_int_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	uint8_t pin = 0;
	switch (pins) {
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
	if (pin) {
		printk("\x1b[KIO_3 Pins#8-15 Int Handler triggered at %" PRIu32 "by pin #%d\n", k_cycle_get_32(), pin);
	} else {
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
	if ((current_time - ref_time) > 250) {
		k_sem_give(&ui_int_sync);
		ref_time = current_time;
	}
}
#endif
/*
 static void bsp_sense_int_cb(const struct device *dev,\
		struct gpio_callback *cb, uint32_t pins)
 {
 printk("Sensor interrupt triggered at %" PRIu32 "\n", k_cycle_get_32());
 }
 */

static void bsp_tamp_int_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
	printk("Tamper detection interrupt triggered at %" PRIu32 "\n", k_cycle_get_32());
}

static int acpu_c201_bsp_exti_cb_config(void) {
	int ret = 0;

	const struct device *dio3_int = device_get_binding(ACPU_GPIO_LABEL_UI_INT);
	const struct bsp_gpios_cfg *dio3_int_cfg = dio3_int->config;
	const struct bsp_gpios_data *dio3_int_data = dio3_int->data;
	const struct device *dio3_int_port = dio3_int_data->port;

	gpio_init_callback(&m_dio3_int_cb_data, bsp_dio3_int_cb, BIT(dio3_int_cfg->pin));
	ret |= gpio_add_callback(dio3_int_port, &m_dio3_int_cb_data);
	if (ret)
		printk("Failed to configure UI Interrupt");

	/*	const struct device *sensor_int = device_get_binding(ACPU_GPIO_LABEL_SENS_INT);
	 const struct bsp_gpios_cfg *sensor_int_cfg = sensor_int->config;
	 const struct bsp_gpios_data *sensor_int_data = sensor_int->data;
	 const struct device *sensor_int_port = sensor_int_data->port;

	 gpio_init_callback(&m_sensor_int_cb_data, bsp_sense_int_cb, BIT(sensor_int_cfg->pin));
	 ret |= gpio_add_callback(sensor_int_port, &m_sensor_int_cb_data);
	 if (ret)
	 printk("Failed to configure Sensor Interrupt");
	 */

	const struct device *tamp_int = device_get_binding(ACPU_GPIO_LABEL_TAMP_DET);
	const struct bsp_gpios_cfg *tamp_int_cfg = tamp_int->config;
	const struct bsp_gpios_data *tamp_int_data = tamp_int->data;
	const struct device *tamp_int_port = tamp_int_data->port;

	gpio_init_callback(&m_tamp_int_cb_data, bsp_tamp_int_cb, BIT(tamp_int_cfg->pin));
	ret |= gpio_add_callback(tamp_int_port, &m_tamp_int_cb_data);
	if (ret)
		printk("Failed to configure Tamper Detection Interrupt");

	return ret;
}

int acpu_c201_bsp_led_ind_control(uint16_t control) {
	return 0;
}

int acpu_c201_bsp_sys_alive(void) {
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

void acpu_c201_bsp_serial_debug_output(void) {
	//extern struct sensor_value temp_value;

	printk("\x1b[0;0H");
	printk("\x1b[KUptime %d\n", counter++);
	//printk("\x1b[KTemperature\x1b[32m %d.%d\x1b[0m C\n", temp_value.val1, temp_value.val2 / 10000);

	return;
}

static int acpu_c201_bsp_clear_dio_exti() {
	const struct device *dev_dio;
	uint32_t port_val = 0;
	int ret = 0;

	dev_dio = device_get_binding(ACPU_GPIO_LABEL_DIO_1);
	ret |= gpio_port_get(dev_dio, &port_val);

	dev_dio = device_get_binding(ACPU_GPIO_LABEL_DIO_2);
	ret |= gpio_port_get(dev_dio, &port_val);

	dev_dio = device_get_binding(ACPU_GPIO_LABEL_DIO_3);
	ret |= gpio_port_get(dev_dio, &port_val);

	return ret;
}

int acpu_c201_bsp_gpio_add_callback(const char *port_name, gpio_port_pins_t pins,
		acpu_c201_bsp_gpio_callback_handler_t cb) {
	int ret = 0;
	if (m_gpio_data_idx >= ACPU_GPIO_MAX_CB) {
		return -ENOSPC;
	}

	m_gpio_data[m_gpio_data_idx].port_name = port_name;
	m_gpio_data[m_gpio_data_idx].pin_mask = pins;
	m_gpio_data[m_gpio_data_idx].cb = cb;
	m_gpio_data_idx++;

	return ret;
}

int acpu_c201_bsp_gpio_init() {
	int ret = 0;

	/* Initialize the bsp gpio data to default value */
	for (int i = 0; i < ACPU_GPIO_MAX_CB; i++) {
		memset(&m_gpio_data[i], 0x00, sizeof(bsp_gpio_data));
	}
	m_gpio_data_idx = 0;

	ret |= acpu_c201_bsp_clear_dio_exti();
	ret |= acpu_c201_bsp_exti_cb_config();
	return ret;
}
