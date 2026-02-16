/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 07-Sep-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 *
 *  References:
 *  	https://www.kernel.org/doc/Documentation/devicetree/bindings/power/supply/ltc2941.txt
 *  	https://github.com/torvalds/linux/blob/master/drivers/power/supply/ltc2941-battery-gauge.c
 */


#define DT_DRV_COMPAT lltc_ltc294x

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_LTC294X_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ltc294x);

#include "ltc294x.h"

#define I16_MSB(x)			((x >> 8) & 0xFF)
#define I16_LSB(x)			(x & 0xFF)

#define LTC294X_WORK_DELAY		10	/* Update delay in seconds */

#define LTC294X_MAX_VALUE		0xFFFF
#define LTC294X_MID_SUPPLY		0x7FFF

#define LTC2941_MAX_PRESCALER_EXP	7
#define LTC2943_MAX_PRESCALER_EXP	6
#define LTC2944_MAX_PRESCALER_EXP	12

enum ltc294x_reg {
	LTC294X_REG_STATUS					= 0x00,
	LTC294X_REG_CONTROL					= 0x01,
	LTC294X_REG_ACC_CHARGE_MSB			= 0x02,
	LTC294X_REG_ACC_CHARGE_LSB			= 0x03,
	LTC294X_REG_CHARGE_THR_HIGH_MSB		= 0x04,
	LTC294X_REG_CHARGE_THR_HIGH_LSB		= 0x05,
	LTC294X_REG_CHARGE_THR_LOW_MSB		= 0x06,
	LTC294X_REG_CHARGE_THR_LOW_LSB		= 0x07,
	LTC294X_REG_VOLTAGE_MSB				= 0x08,
	LTC294X_REG_VOLTAGE_LSB				= 0x09,
	LTC2942_REG_TEMPERATURE_MSB			= 0x0C,
	LTC2942_REG_TEMPERATURE_LSB			= 0x0D,
	LTC2943_REG_CURRENT_MSB				= 0x0E,
	LTC2943_REG_CURRENT_LSB				= 0x0F,
	LTC2943_REG_TEMPERATURE_MSB			= 0x14,
	LTC2943_REG_TEMPERATURE_LSB			= 0x15,
};

enum ltc294x_id {
	LTC2941_ID=1,
	LTC2942_ID,
	LTC2943_ID,
	LTC2944_ID,
};

#define LTC2941_REG_STATUS_CHIP_ID	BIT(7)

#define LTC2942_REG_CONTROL_MODE_SCAN	(BIT(7) | BIT(6))
#define LTC2943_REG_CONTROL_MODE_SCAN	BIT(7)
#define LTC294X_REG_CONTROL_PRESCALER_MASK	(BIT(5) | BIT(4) | BIT(3))
#define LTC294X_REG_CONTROL_SHUTDOWN_MASK	(BIT(0))
#define LTC294X_REG_CONTROL_PRESCALER_SET(x) \
	((x << 3) & LTC294X_REG_CONTROL_PRESCALER_MASK)
#define LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED	0
#define LTC294X_REG_CONTROL_ADC_DISABLE(x)	((x) & ~(BIT(7) | BIT(6)))

/** Configuration data */
struct ltc294x_config {
	// const char *i2c_master_name;
	// uint16_t i2c_addr;
	struct i2c_dt_spec i2c_master;
};

struct ltc294x_data {
	// const struct device *i2c;
	// uint8_t i2c_addr;

	/* LTC294X settings */
	int r_sense;
	int prescaler;
	uint8_t id;

	int charge;	/* Last charge register content */
	int Qlsb;	/* nAh */

	/* LTC294x register values */
	int charge_thr_high;
	int charge_thr_low;
	int charge_acc;	/* uAh */
	int charge_counter;
	int voltage;
	int current;
	int temp;

	struct k_work_delayable work;
	const struct device *instance;          /* Self-reference to the driver instance */
};

static inline int convert_bin_to_uAh(const struct ltc294x_data *data, int Q)
{
	return ((Q * (data->Qlsb / 10))) / 100;
}

static inline int convert_uAh_to_bin(const struct ltc294x_data *data, int uAh)
{
	int Q;

	Q = (uAh * 100) / (data->Qlsb/10);
	return (Q < LTC294X_MAX_VALUE) ? Q : LTC294X_MAX_VALUE;
}

static int ltc294x_read_regs(const struct ltc294x_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	// return i2c_burst_read(dev->i2c, dev->i2c_addr, reg, data, length);
	return i2c_burst_read_dt(&config->i2c_master, reg, data, length);
}

static int ltc294x_write_regs(const struct ltc294x_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	// return i2c_burst_write(dev->i2c, dev->i2c_addr, reg, data, length);
	return i2c_burst_write_dt(&config->i2c_master, reg, data, length);
}

static int ltc294x_read_charge_register(const struct ltc294x_config *config, enum ltc294x_reg reg)
{
	int ret;
	uint8_t datar[2];

	ret = ltc294x_read_regs(config, reg, &datar[0], 2);
	if (ret < 0)
		return ret;
	return (datar[0] << 8) + datar[1];
}

static int ltc294x_get_charge(const struct device *dev, enum ltc294x_reg reg, int *val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int value = ltc294x_read_charge_register(config, reg);

	if (value < 0)
		return value;
	/* When r_sense < 0, this counts up when the battery discharges */
	if (data->Qlsb < 0)
		value -= 0xFFFF;
	*val = convert_bin_to_uAh(data, value);
	return 0;
}

static int ltc294x_get_charge_counter(const struct device *dev, int *val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int value = ltc294x_read_charge_register(config, LTC294X_REG_ACC_CHARGE_MSB);

	if (value < 0)
		return value;
	value -= LTC294X_MID_SUPPLY;
	*val = convert_bin_to_uAh(data, value);
	return 0;
}

static int ltc294x_get_voltage(const struct device *dev, int *val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int ret;
	uint8_t datar[2];
	uint32_t value;

	ret = ltc294x_read_regs(config, LTC294X_REG_VOLTAGE_MSB, &datar[0], 2);
	value = (datar[0] << 8) | datar[1];
	switch (data->id) {
	case LTC2943_ID:
		value *= 23600 * 2;
		value /= 0xFFFF;
		value *= 1000 / 2;
		break;
	case LTC2944_ID:
		value *= 70800 / 5*4;
		value /= 0xFFFF;
		value *= 1000 * 5/4;
		break;
	default:
		value *= 6000 * 10;
		value /= 0xFFFF;
		value *= 1000 / 10;
		break;
	}
	*val = value;
	return ret;
}

static int ltc294x_get_current(const struct device *dev, int *val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int ret;
	uint8_t datar[2];
	int32_t value;

	if ((data->id != LTC2943_ID) && (data->id != LTC2944_ID)) {
//		LOG_ERR("not supported");
		return -EINVAL;
	}

	ret = ltc294x_read_regs(config, LTC2943_REG_CURRENT_MSB, &datar[0], 2);
	value = (datar[0] << 8) | datar[1];
	value -= 0x7FFF;
	if (data->id == LTC2944_ID)
		value *= 64000;
	else
		value *= 60000;
	/* Value is in range -32k..+32k, r_sense is usually 10..50 mOhm,
	 * the formula below keeps everything in int32_t range while preserving
	 * enough digits */
	*val = 1000 * (value / (data->r_sense * 0x7FFF)); /* in uA */
	return ret;
}

static int ltc294x_get_temperature(const struct device *dev, int *val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	enum ltc294x_reg reg;
	int ret;
	uint8_t datar[2];
	uint32_t value;

	if (data->id == LTC2942_ID) {
		reg = LTC2942_REG_TEMPERATURE_MSB;
		value = 6000;	/* Full-scale is 600 Kelvin */
	} else {
		reg = LTC2943_REG_TEMPERATURE_MSB;
		value = 5100;	/* Full-scale is 510 Kelvin */
	}
	ret = ltc294x_read_regs(config, reg, &datar[0], 2);
	value *= (datar[0] << 8) | datar[1];
	/* Convert to tenths of degree Celsius */
	*val = value / 0xFFFF - 2722;
	return ret;
}

static int ltc294x_set_charge_now(const struct device *dev, int val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int ret;
	uint8_t dataw[2];
	uint8_t ctrl_reg;
	int32_t value;

	value = convert_uAh_to_bin(data, val);
	/* Direction depends on how sense+/- were connected */
	if (data->Qlsb < 0)
		value += 0xFFFF;
	if ((value < 0) || (value > 0xFFFF)) /* input validation */
		return -EINVAL;

	/* Read control register */
	ret = ltc294x_read_regs(config, LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0)
		return ret;
	/* Disable analog section */
	ctrl_reg |= LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(config, LTC294X_REG_CONTROL, &ctrl_reg, 1);
	if (ret < 0)
		return ret;
	/* Set new charge value */
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	LOG_DBG("MSB = 0x%x, LSB = 0x%x", dataw[0], dataw[1]);
	ret = ltc294x_write_regs(config, LTC294X_REG_ACC_CHARGE_MSB, &dataw[0], 2);
	if (ret < 0)
		goto error_exit;
	/* Enable analog section */
	error_exit: ctrl_reg &= ~LTC294X_REG_CONTROL_SHUTDOWN_MASK;
	ret = ltc294x_write_regs(config, LTC294X_REG_CONTROL, &ctrl_reg, 1);

	return ret < 0 ? ret : 0;
}

static int ltc294x_set_charge_thr(const struct device *dev,
					enum ltc294x_reg reg, int val)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	uint8_t dataw[2];
	int32_t value;

	value = convert_uAh_to_bin(data, val);
	/* Direction depends on how sense+/- were connected */
	if (data->Qlsb < 0)
		value += 0xFFFF;
	if ((value < 0) || (value > 0xFFFF)) /* input validation */
		return -EINVAL;

	/* Set new charge value */
	dataw[0] = I16_MSB(value);
	dataw[1] = I16_LSB(value);
	return ltc294x_write_regs(config, reg, &dataw[0], 2);
}


static void ltc294x_update(const struct device *dev)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int charge = ltc294x_read_charge_register(config, LTC294X_REG_ACC_CHARGE_MSB);

	if (charge != data->charge) {
		data->charge = charge;
//		power_supply_changed(info->supply);
		/* TODO: callback to application */
	}
}

static void ltc294x_work(struct k_work *twork)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(twork);
	struct ltc294x_data *data = CONTAINER_OF(dwork, struct ltc294x_data, work);

	ltc294x_update(data->instance);
	k_work_schedule(&data->work, K_SECONDS(LTC294X_WORK_DELAY));
}

static int ltc294x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ltc294x_data *data = dev->data;
	int ret = 0;

	/* POWER_SUPPLY_PROP_CHARGE_FULL */
	ret = ltc294x_get_charge(dev, LTC294X_REG_CHARGE_THR_HIGH_MSB, &data->charge_thr_high);

	/* POWER_SUPPLY_PROP_CHARGE_EMPTY */
	ret = ltc294x_get_charge(dev, LTC294X_REG_CHARGE_THR_LOW_MSB, &data->charge_thr_low);

	/* POWER_SUPPLY_PROP_CHARGE_NOW */
	ret = ltc294x_get_charge(dev, LTC294X_REG_ACC_CHARGE_MSB, &data->charge_acc);

	/* POWER_SUPPLY_PROP_CHARGE_COUNTER */
	ret = ltc294x_get_charge_counter(dev, &data->charge_counter);

	/* POWER_SUPPLY_PROP_VOLTAGE_NOW */
	ret = ltc294x_get_voltage(dev, &data->voltage);

	/* POWER_SUPPLY_PROP_CURRENT_NOW */
	ret = ltc294x_get_current(dev, &data->current);

	/* POWER_SUPPLY_PROP_TEMP */
	ret = ltc294x_get_temperature(dev, &data->temp);

	return ret;
}

/**
 * @brief sensor value get
 *
 * @param dev ltc294x device to access
 * @param chan Channel number to read
 * @param valp Returns the sensor value read on success
 * @return 0 if successful
 * @return -ENOTSUP for unsupported channels
 */
static int ltc294x_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *valp)
{
	struct ltc294x_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY:
		valp->val1 = (data->charge_acc / 1000);		/* uAh to mAh */
		valp->val2 = (data->charge_acc % 1000) * 1000;		/* decimal part */
		break;

	case SENSOR_CHAN_GAUGE_VOLTAGE:
		valp->val1 = (data->voltage);
		valp->val2 = 0;
		break;

	case SENSOR_CHAN_GAUGE_TEMP:
		valp->val1 = (data->temp);
		valp->val2 = 0;
		break;

	default:
		break;
	}
	return 0;
}

static int ltc294x_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	// struct ltc294x_data *data = dev->data;
	int ret = 0;
	int tmp = (int) attr;

	switch (tmp) {
	case SENSOR_ATTR_CHARGE_THR_HIGH:
		ltc294x_set_charge_thr(dev, LTC294X_REG_CHARGE_THR_HIGH_MSB, val->val1);
		break;
	case SENSOR_ATTR_CHARGE_THR_LOW:
		ltc294x_set_charge_thr(dev, LTC294X_REG_CHARGE_THR_LOW_MSB, val->val1);
		break;
	case SENSOR_ATTR_CHARGE_VALUE_NOW:
		ltc294x_set_charge_now(dev, val->val1);
		break;
	default:
		break;
	}

	return ret;
}

static int ltc294x_reset(const struct device *dev, int prescaler_exp)
{
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;

	int ret;
	uint8_t value;
	uint8_t control;

	/* Read status and control registers */
	ret = ltc294x_read_regs(config, LTC294X_REG_CONTROL, &value, 1);
	if (ret < 0)
		return ret;

	control = LTC294X_REG_CONTROL_PRESCALER_SET(prescaler_exp) |
				LTC294X_REG_CONTROL_ALCC_CONFIG_DISABLED;
	/* Put device into "monitor" mode */
	switch (data->id) {
	case LTC2942_ID:	/* 2942 measures every 2 sec */
		control |= LTC2942_REG_CONTROL_MODE_SCAN;
		break;
	case LTC2943_ID:
	case LTC2944_ID:	/* 2943 and 2944 measure every 10 sec */
		control |= LTC2943_REG_CONTROL_MODE_SCAN;
		break;
	default:
		break;
	}

	if (value != control) {
		ret = ltc294x_write_regs(config, LTC294X_REG_CONTROL, &control, 1);
		if (ret < 0)
			return ret;
	}

	return 0;
}

/* Init function */
static int ltc294x_init(const struct device *dev) {
	const struct ltc294x_config *config = dev->config;
	struct ltc294x_data *data = dev->data;
	int ret = -1;
	uint8_t status;
	uint32_t prescaler_exp;

	/* get the I2C master device */
	// data->i2c = device_get_binding(config->i2c_master_name);
	// if (data->i2c == NULL) {
	// 	LOG_ERR("Could not get pointer to %s device", config->i2c_master_name);
	// 	return -EINVAL;
	// }
	// data->i2c_addr = config->i2c_addr;

	if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

    /* Store self-reference */
	data->instance = dev;

	/* set prescaler */
	prescaler_exp = data->prescaler;
	if (data->id == LTC2943_ID) {
		if (prescaler_exp > LTC2943_MAX_PRESCALER_EXP)
			prescaler_exp = LTC2943_MAX_PRESCALER_EXP;
		data->Qlsb = ((340 * 50000) / data->r_sense) >> (12 - 2 * prescaler_exp);
	}
#if 0
	else if (data->id == LTC2944_ID) {
		if (prescaler_exp > LTC2944_MAX_PRESCALER_EXP)
			prescaler_exp = LTC2944_MAX_PRESCALER_EXP;
		data->Qlsb = ((340 * 50000) / data->r_sense) >> (12 - prescaler_exp);
	}
#else
	else if (data->id == LTC2944_ID) {
		if (prescaler_exp > LTC2943_MAX_PRESCALER_EXP)
			prescaler_exp = LTC2943_MAX_PRESCALER_EXP;
		data->Qlsb = ((340 * 50000) / data->r_sense) >> (12 - 2 * prescaler_exp);
	}
#endif
	else {
		if (prescaler_exp > LTC2941_MAX_PRESCALER_EXP)
			prescaler_exp = LTC2941_MAX_PRESCALER_EXP;
		data->Qlsb = ((85 * 50000) / data->r_sense) >> (7 - prescaler_exp);
	}
	LOG_INF("Qlsb = %d", data->Qlsb);
	/* Read status register to check for LTC2942 */
	if (data->id == LTC2941_ID || data->id == LTC2942_ID) {
		ret = ltc294x_read_regs(config, LTC294X_REG_STATUS, &status, 1);
		if (ret < 0) {
			LOG_ERR("Could not read status register");
			return -1;
		}
		if (status & LTC2941_REG_STATUS_CHIP_ID)
			data->id = LTC2941_ID;
		else
			data->id = LTC2942_ID;
	}

	/* initialize a delayable work queue */
	k_work_init_delayable(&data->work, ltc294x_work);

	/* reset the chip */
	ret = ltc294x_reset(dev, prescaler_exp);
	if (ret < 0) {
		LOG_ERR("Communication with chip failed, ltc294x_reset");
		return ret;
	}

	/* schedule the work queue */
	k_work_schedule(&data->work, K_SECONDS(LTC294X_WORK_DELAY));

	return ret;
}

static const struct sensor_driver_api ltc294x_driver_api = {
	.sample_fetch = ltc294x_sample_fetch,
	.channel_get = ltc294x_channel_get,
	.attr_set = ltc294x_attr_set
};

#define DEVICE_INSTANCE(inst) \
\
const static struct ltc294x_config ltc294x_##inst##_cfg = { \
	/*.i2c_master_name = DT_INST_BUS_LABEL(0), \
	.i2c_addr = DT_INST_REG_ADDR(0), */\
	.i2c_master = I2C_DT_SPEC_INST_GET(inst),		       \
};\
	\
static struct ltc294x_data ltc294x_##inst##_drvdata = { \
	.r_sense = DT_PROP(DT_DRV_INST(inst),resistor_sense), \
	.prescaler = DT_PROP(DT_DRV_INST(inst),prescaler_exponent), \
	.id = DT_PROP(DT_DRV_INST(inst),device_id), \
}; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		ltc294x_init,									\
		device_pm_control_nop,							\
		&ltc294x_##inst##_drvdata,						\
		&ltc294x_##inst##_cfg,							\
		APPLICATION, CONFIG_LTC294X_INIT_PRIORITY,		\
		&ltc294x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
