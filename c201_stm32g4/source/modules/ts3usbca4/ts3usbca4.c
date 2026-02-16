/*
 * Copyright (c) 2021 Acme CPU
 */
#define DT_DRV_COMPAT ti_ts3usbca4

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(TS3USBCA4, CONFIG_TS3USBCA4_LOG_LEVEL);

#include "ts3usbca4.h"

#define REG_T3USBCA4_REVISION_ID	(0x09)
#define REG_T3USBCA4_GENERAL_1		(0x0A)
#define REG_T3USBCA4_GENERAL_2		(0x0B)

struct ts3usbca4_config {
	/** The master I2C device's name */
//	const char * const i2c_master_dev_name;

	struct i2c_dt_spec i2c_bus;

	/* The slave address of the chip */
//	uint16_t i2c_slave_addr;

	/* Output Enable pin definition */
//	const char *oe_gpio_port;
//	gpio_pin_t oe_gpio_pin;
//	gpio_flags_t oe_gpio_flags;
	struct gpio_dt_spec oe_gpio;
};

struct ts3usbca4_data {
	/* Master I2C device */
//	const struct device *i2c_master;
	struct k_sem lock;
};

static int read_regs_i2c(const struct device *dev, const uint8_t reg, uint8_t *buf) {
	const struct ts3usbca4_config *const config = dev->config;
//	struct ts3usbca4_data *const drv_data = (struct ts3usbca4_data* const ) dev->data;
//	const struct device *i2c_master = drv_data->i2c_master;
//	uint16_t i2c_addr = config->i2c_slave_addr;
//	int ret;
//
//	ret = i2c_burst_read(i2c_master, i2c_addr, reg, (uint8_t*) buf, 1);
//	if (ret != 0) {
//		LOG_ERR("[0x%X]: error reading register 0x%X (%d)", i2c_addr, reg, ret);
//		return ret;
//	}
//
//	LOG_DBG("[0x%X]: Read: REG[0x%X] = 0x%X", i2c_addr, reg, *buf);
//
//	return 0;

	return i2c_burst_read_dt(&config->i2c_bus, reg, buf, 1);
}

static int write_regs_i2c(const struct device *dev, const uint8_t reg, const uint8_t value) {
	const struct ts3usbca4_config *const config = dev->config;
//	struct ts3usbca4_data *const drv_data = (struct ts3usbca4_data* const ) dev->data;
//	const struct device *i2c_master = drv_data->i2c_master;
//	uint16_t i2c_addr = config->i2c_slave_addr;
//	int ret;
//
//	LOG_DBG("[0x%X]: Write: REG[0x%X] = 0x%X", i2c_addr, reg, value);
//
//	ret = i2c_burst_write(i2c_master, i2c_addr, reg, (uint8_t*) &value, sizeof(value));
//	if (ret != 0) {
//		LOG_ERR("[0x%X]: error writing to register 0x%X (%d)", i2c_addr, reg, ret);
//	}
//
//	return ret;

	return i2c_burst_write_dt(&config->i2c_bus, reg, &value, sizeof(value));
}


static int ts3usbca4_init(const struct device *dev)
{
//	struct ts3usbca4_data *data = dev->data;
	const struct ts3usbca4_config *config = dev->config;
	int ret = 0;
	uint8_t revision=0;

//	const struct device *i2c_master;
//	/* Find out the device struct of the bus */
//	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
//	if (!i2c_master) {
//		return -EINVAL;
//	}
//	data->i2c_master = i2c_master;

	if (!device_is_ready(config->i2c_bus.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	if (device_is_ready(config->oe_gpio.port)) {
//	/* Configure GPIO output enable pin */
//	const struct device *en_gpio_dev = device_get_binding(config->oe_gpio_port);
//	if (en_gpio_dev != NULL) {
		ret |= gpio_pin_configure(config->oe_gpio.port, config->oe_gpio.pin, (config->oe_gpio.dt_flags | GPIO_OUTPUT));
		/* Change output enable pin to normal operation */
		ret |= gpio_pin_set(config->oe_gpio.port, config->oe_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)", config->oe_gpio.pin, ret);
			return ret;
		}
//	}
	}

	k_sleep(K_MSEC(250));

	/* Read the status */
	ret = read_regs_i2c(dev, REG_T3USBCA4_REVISION_ID, &revision);
	if (ret != 0) {
		LOG_ERR("read failed for register 0x%x, %d", REG_T3USBCA4_REVISION_ID, ret);
		return ret;
	}

	return ret;
}

struct ts3usbca4_driver_api ts3usbca4_api = {
	.select_channel = NULL
};

#define DEVICE_INSTANCE(inst) \
	\
static const struct ts3usbca4_config ts3usbca4_##inst##_config = { \
	/*.i2c_master_dev_name = DT_INST_BUS_LABEL(0), \
	.i2c_slave_addr = DT_INST_REG_ADDR(0),*/		\
	.i2c_bus = I2C_DT_SPEC_INST_GET(inst),		       \
\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, oe_gpios), ( 	\
		/*.oe_gpio_port = DT_INST_GPIO_LABEL(0, oe_gpios), \
		.oe_gpio_pin = DT_INST_GPIO_PIN(0, oe_gpios), \
		.oe_gpio_flags = DT_INST_GPIO_FLAGS(0, oe_gpios), */\
		.oe_gpio = GPIO_DT_SPEC_INST_GET(inst, oe_gpios), \
	)) \
}; \
\
static struct ts3usbca4_data ts3usbca4_##inst##_data; \
 \
DEVICE_DT_INST_DEFINE(inst, \
		ts3usbca4_init, \
		device_pm_control_nop, \
		&ts3usbca4_##inst##_data, \
		&ts3usbca4_##inst##_config, \
		APPLICATION, \
		CONFIG_TS3USBCA4_INIT_PRIORITY, \
		&ts3usbca4_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
