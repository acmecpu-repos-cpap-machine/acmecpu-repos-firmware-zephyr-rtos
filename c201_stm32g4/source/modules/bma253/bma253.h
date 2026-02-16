/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 24-Jan-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef MODULES_BMA253_BMA253_H_
#define MODULES_DRIVERS_SENSOR_BMA253_BMA253_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

//#define BMA253_I2C_ADDRESS		DT_INST_REG_ADDR(0)

#define BMA253_REG_CHIP_ID		0x00
#define BMA253_CHIP_ID			0xFA

#define BMA253_REG_PMU_BW		0x10
#if CONFIG_BMA253_PMU_BW_1
	#define BMA253_PMU_BW		0x08
#elif CONFIG_BMA253_PMU_BW_2
	#define BMA253_PMU_BW		0x09
#elif CONFIG_BMA253_PMU_BW_3
	#define BMA253_PMU_BW		0x0A
#elif CONFIG_BMA253_PMU_BW_4
	#define BMA253_PMU_BW		0x0B
#elif CONFIG_BMA253_PMU_BW_5
	#define BMA253_PMU_BW		0x0C
#elif CONFIG_BMA253_PMU_BW_6
	#define BMA253_PMU_BW		0x0D
#elif CONFIG_BMA253_PMU_BW_7
	#define BMA253_PMU_BW		0x0E
#elif CONFIG_BMA253_PMU_BW_8
	#define BMA253_PMU_BW		0x0F
#endif

/*
 * BMA253_PMU_FULL_RANGE measured in mili-m/s^2 instead
 * of m/s^2 to avoid using struct sensor_value for it
 */
#define BMA253_REG_PMU_RANGE		0x0F
#if CONFIG_BMA253_PMU_RANGE_2G
	#define BMA253_PMU_RANGE	0x03
	#define BMA253_PMU_FULL_RANGE	(4 * SENSOR_G)
#elif CONFIG_BMA253_PMU_RANGE_4G
	#define BMA253_PMU_RANGE	0x05
	#define BMA253_PMU_FULL_RANGE	(8 * SENSOR_G)
#elif CONFIG_BMA253_PMU_RANGE_8G
	#define BMA253_PMU_RANGE	0x08
	#define BMA253_PMU_FULL_RANGE	(16 * SENSOR_G)
#elif CONFIG_BMA253_PMU_RANGE_16G
	#define BMA253_PMU_RANGE	0x0C
	#define BMA253_PMU_FULL_RANGE	(32 * SENSOR_G)
#endif

#define BMA253_REG_TEMP			0x08

#define BMA253_REG_INT_STATUS_0		0x09
#define BMA253_BIT_SLOPE_INT_STATUS	BIT(2)
#define BMA253_REG_INT_STATUS_1		0x0A
#define BMA253_BIT_DATA_INT_STATUS	BIT(7)

#define BMA253_REG_INT_EN_0		0x16
#define BMA253_BIT_SLOPE_EN_X		BIT(0)
#define BMA253_BIT_SLOPE_EN_Y		BIT(1)
#define BMA253_BIT_SLOPE_EN_Z		BIT(2)
#define BMA253_SLOPE_EN_XYZ (BMA253_BIT_SLOPE_EN_X | \
		BMA253_BIT_SLOPE_EN_Y | BMA253_BIT_SLOPE_EN_X)

#define BMA253_REG_INT_EN_1		0x17
#define BMA253_BIT_DATA_EN		BIT(4)

#define BMA253_REG_INT_MAP_0		0x19
#define BMA253_INT_MAP_0_BIT_SLOPE	BIT(2)

#define BMA253_REG_INT_MAP_1		0x1A
#define BMA253_INT_MAP_1_BIT_DATA	BIT(0)

#define BMA253_REG_INT_RST_LATCH	0x21
#define BMA253_INT_MODE_LATCH		0x0F
#define BMA253_BIT_INT_LATCH_RESET	BIT(7)

#define BMA253_REG_INT_5		0x27
#define BMA253_SLOPE_DUR_SHIFT		0
#define BMA253_SLOPE_DUR_MASK		(3 << BMA253_SLOPE_DUR_SHIFT)

#define BMA253_REG_SLOPE_TH		0x28

#define BMA253_REG_ACCEL_X_LSB		0x2
#define BMA253_REG_ACCEL_Y_LSB		0x4
#define BMA253_REG_ACCEL_Z_LSB		0x6

#define BMA253_ACCEL_LSB_BITS	4
#define BMA253_ACCEL_LSB_SHIFT	4

#define BMA253_ACCEL_LSB_MASK		\
		(BIT_MASK(BMA253_ACCEL_LSB_BITS) << BMA253_ACCEL_LSB_SHIFT)

#define BMA253_REG_ACCEL_X_MSB		0x3
#define BMA253_REG_ACCEL_Y_MSB		0x5
#define BMA253_REG_ACCEL_Z_MSB		0x7

#define BMA253_THREAD_PRIORITY		10
#define BMA253_THREAD_STACKSIZE_UNIT	1024

struct bma253_config {
	// const char *i2c_master_name;
	struct i2c_dt_spec i2c_master;
};

struct bma253_data {
//	const struct device *i2c;
	int16_t x_sample;
	int16_t y_sample;
	int16_t z_sample;
	int8_t temp_sample;

#ifdef CONFIG_BMA253_TRIGGER
	const struct device *dev;
	const struct device *gpio;
	struct gpio_callback gpio_cb;

	struct sensor_trigger data_ready_trigger;
	sensor_trigger_handler_t data_ready_handler;

	struct sensor_trigger any_motion_trigger;
	sensor_trigger_handler_t any_motion_handler;

#if defined(CONFIG_BMA253_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_BMA253_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem gpio_sem;
#elif defined(CONFIG_BMA253_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif

#endif /* CONFIG_BMA253_TRIGGER */
};

#ifdef CONFIG_BMA253_TRIGGER
int bma253_trigger_set(const struct device *dev,
		       const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler);

int bma253_attr_set(const struct device *dev,
		    enum sensor_channel chan,
		    enum sensor_attribute attr,
		    const struct sensor_value *val);

int bma253_init_interrupt(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA253_BMA253_H_ */
