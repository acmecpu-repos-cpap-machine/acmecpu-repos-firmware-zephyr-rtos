/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 24-Jan-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#define DT_DRV_COMPAT bosch_bma253

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "bma253.h"

LOG_MODULE_REGISTER(BMA253, CONFIG_SENSOR_LOG_LEVEL);

static int bma253_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	const struct bma253_config *config = dev->config;
	struct bma253_data *drv_data = dev->data;
	uint8_t buf[6];
	uint8_t lsb;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/*
	 * since all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	if (i2c_burst_read(config->i2c_master.bus, config->i2c_master.addr,
			   BMA253_REG_ACCEL_X_LSB, buf, 6) < 0) {
		LOG_DBG("Could not read accel axis data");
		return -EIO;
	}

	lsb = (buf[0] & BMA253_ACCEL_LSB_MASK) >> BMA253_ACCEL_LSB_SHIFT;
	drv_data->x_sample = (((int8_t)buf[1]) << BMA253_ACCEL_LSB_BITS) | lsb;

	lsb = (buf[2] & BMA253_ACCEL_LSB_MASK) >> BMA253_ACCEL_LSB_SHIFT;
	drv_data->y_sample = (((int8_t)buf[3]) << BMA253_ACCEL_LSB_BITS) | lsb;

	lsb = (buf[4] & BMA253_ACCEL_LSB_MASK) >> BMA253_ACCEL_LSB_SHIFT;
	drv_data->z_sample = (((int8_t)buf[5]) << BMA253_ACCEL_LSB_BITS) | lsb;

	if (i2c_reg_read_byte(config->i2c_master.bus, config->i2c_master.addr,
			      BMA253_REG_TEMP,
			      (uint8_t *)&drv_data->temp_sample) < 0) {
		LOG_DBG("Could not read temperature data");
		return -EIO;
	}

	return 0;
}

static void bma253_channel_accel_convert(struct sensor_value *val,
					int64_t raw_val)
{
	/*
	 * accel_val = (sample * BMA253_PMU_FULL_RAGE) /
	 *             (2^data_width * 10^6)
	 */
	raw_val = (raw_val * BMA253_PMU_FULL_RANGE) /
		  (1 << (8 + BMA253_ACCEL_LSB_BITS));
	val->val1 = raw_val / 1000000;
	val->val2 = raw_val % 1000000;

	/* normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}
}

static int bma253_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bma253_data *drv_data = dev->data;

	/*
	 * See datasheet "Sensor data" section for
	 * more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_ACCEL_X) {
		bma253_channel_accel_convert(val, drv_data->x_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		bma253_channel_accel_convert(val, drv_data->y_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		bma253_channel_accel_convert(val, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		bma253_channel_accel_convert(val, drv_data->x_sample);
		bma253_channel_accel_convert(val + 1, drv_data->y_sample);
		bma253_channel_accel_convert(val + 2, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* temperature_val = 23 + sample / 2 */
		val->val1 = (drv_data->temp_sample >> 1) + 23;
		val->val2 = 500000 * (drv_data->temp_sample & 1);
		return 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bma253_driver_api = {
#if CONFIG_BMA253_TRIGGER
	.attr_set = bma253_attr_set,
	.trigger_set = bma253_trigger_set,
#endif
	.sample_fetch = bma253_sample_fetch,
	.channel_get = bma253_channel_get,
};

int bma253_init(const struct device *dev)
{
	const struct bma253_config *config = dev->config;
//	struct bma253_data *drv_data = dev->data;
	uint8_t id = 0U;

//	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
//	if (drv_data->i2c == NULL) {
//		LOG_DBG("Could not get pointer to %s device",
//			    DT_INST_BUS_LABEL(0));
//		return -EINVAL;
//	}

	if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* read device ID */
	if (i2c_reg_read_byte(config->i2c_master.bus, config->i2c_master.addr,
			      BMA253_REG_CHIP_ID, &id) < 0) {
		LOG_ERR("Could not read chip id");
		return -EIO;
	}

	if (id != BMA253_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x)", id);
		return -EIO;
	}

	if (i2c_reg_write_byte(config->i2c_master.bus, config->i2c_master.addr,
			       BMA253_REG_PMU_BW, BMA253_PMU_BW) < 0) {
		LOG_ERR("Could not set data filter bandwidth");
		return -EIO;
	}

	/* set g-range */
	if (i2c_reg_write_byte(config->i2c_master.bus, config->i2c_master.addr,
			       BMA253_REG_PMU_RANGE, BMA253_PMU_RANGE) < 0) {
		LOG_ERR("Could not set data g-range");
		return -EIO;
	}

#ifdef CONFIG_BMA253_TRIGGER
	if (bma253_init_interrupt(dev) < 0) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	return 0;
}

#define DEVICE_INSTANCE(inst) \
const static struct bma253_config bma253_##inst##_cfg = { \
	.i2c_master = I2C_DT_SPEC_INST_GET(inst),		       \
};\
struct bma253_data bma253_##inst##_data; \
\
DEVICE_DT_INST_DEFINE(	inst, \
						bma253_init, \
						NULL, \
						&bma253_##inst##_data, \
						&bma253_##inst##_cfg, \
						APPLICATION, \
						CONFIG_BMA253_INIT_PRIORITY, \
						&bma253_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
