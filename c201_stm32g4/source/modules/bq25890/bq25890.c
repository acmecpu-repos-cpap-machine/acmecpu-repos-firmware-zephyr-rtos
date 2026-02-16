/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 08-Sep-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 *
 *  References:
 *  	https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/power/supply/bq25890_charger.c
 *  	https://elixir.bootlin.com/linux/v4.2/source/Documentation/devicetree/bindings/power/bq25890.txt
 */


#define DT_DRV_COMPAT ti_bq25890

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_LTC294X_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bq25890d);

#include "bq25890.h"


/** Configuration data */
struct bq25890_config {
//	const char *i2c_master_name;
//	uint16_t i2c_addr;
	struct i2c_dt_spec i2c_bus;
};

struct bq25890_data {
//	const struct device *i2c;
//	uint8_t i2c_addr;
};

static int bq25890_read_regs(const struct bq25890_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
//	return i2c_burst_read(dev->i2c, dev->i2c_addr, reg, data, length);
	return i2c_burst_read_dt(&config->i2c_bus, reg, data, length);
}

static int bq25890_write_regs(const struct bq25890_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	// return i2c_burst_write(dev->i2c, dev->i2c_addr, reg, data, length);
	return i2c_burst_write_dt(&config->i2c_bus, reg, data, length);
}

static int bq25890_init(const struct device *dev) {
	int ret = -1;

	const struct bq25890_config *config = dev->config;
//	struct bq25890_data *data = dev->data;

	/* get the I2C master device */
//	data->i2c = device_get_binding(config->i2c_master_name);
//	if (data->i2c == NULL) {
//		LOG_ERR("Could not get pointer to %s device", config->i2c_master_name);
//		return -EINVAL;
//	}
//	data->i2c_addr = config->i2c_addr;

	if (!device_is_ready(config->i2c_bus.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* read chip version */
//	ret = bq25890_get_chip_version(bq);
	uint8_t reg14;
	ret = bq25890_read_regs(config, 0x14, &reg14, 1);
	if (ret) {
		LOG_ERR("Cannot read chip ID or unknown chip: %d", ret);
		return ret;
	}

	/* set input current limit
	 * 0 - disable HIZ
	 * 1 - enable ILIM
	 * 111111 - 3.25A
	 * [01111111] = 0x7F
	 * */
	uint8_t data = 0x7F;
	ret = bq25890_write_regs(config, 0x00, &data, 1);
	if (ret) {
		LOG_ERR("Cannot write to REG00: %d", ret);
		return ret;
	}

	return ret;
}

#define DEVICE_INSTANCE(inst) \
\
const static struct bq25890_config bq25890_##inst##_cfg = { \
	/*.i2c_master_name = DT_INST_BUS_LABEL(0), \
	.i2c_addr = DT_INST_REG_ADDR(0), */\
	.i2c_bus = I2C_DT_SPEC_INST_GET(inst),		       \
};\
static struct bq25890_data bq25890_##inst##_drvdata; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		bq25890_init,									\
		device_pm_control_nop,							\
		&bq25890_##inst##_drvdata,						\
		&bq25890_##inst##_cfg,							\
		APPLICATION, CONFIG_BQ25890_INIT_PRIORITY,		\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
