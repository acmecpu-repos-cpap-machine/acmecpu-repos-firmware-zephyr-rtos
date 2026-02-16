/*
 * Copyright (c) 2021 Acme CPU
 */


#ifndef MODULES_PCA9545A_PCA9545_H_
#define MODULES_PCA9545A_PCA9545_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

struct i2c_mgm_pca95xx_cfg {
	/** The master I2C bus name */
//	const char * const i2c_bus_dev_name;
	/** The slave address of the chip */
//	uint16_t i2c_slave_addr;

	struct i2c_dt_spec i2c_master;


#ifdef CONFIG_I2C_MGM_PCA95XX_INTERRUPT
	/* Interrupt pin definition */
//	const char *int_gpio_port;
//	gpio_pin_t int_gpio_pin;
//	gpio_flags_t int_gpio_flags;
	struct gpio_dt_spec int_gpio;
#endif
};

struct i2c_mgm_pca95xx_data {
	/** Master I2C device */
	const struct device *i2c_bus;
	const struct device *reset;
	uint8_t ch_state;
	uint8_t interrupt;
	struct k_mutex lock;
//	struct k_sem lock;
	struct k_sem irq_sync_sem;
};

struct i2c_mgm_pca95xx_ch_cfg {
	const char *switch_name;
	const struct i2c_mgm_pca95xx_cfg *switch_cfg;
	struct i2c_mgm_pca95xx_data * const switch_data;
	const uint8_t channel_number;
};

struct i2c_mgm_pca95xx_ch_data {
	const struct device *switch_dev;
};

typedef int (*i2c_mgm_pca95xx_reset_t)(const struct device *);
typedef int (*i2c_mgm_pca95xx_get_int_t)(const struct device *);
typedef int (*i2c_mgm_pca95xx_get_state_t)(const struct device *);
typedef int (*i2c_mgm_pca95xx_int_config_t)(const struct device *, gpio_pin_t, gpio_flags_t);

struct i2c_mgm_pca95xx_master_api_t {
	i2c_mgm_pca95xx_reset_t reset;
	i2c_mgm_pca95xx_get_int_t get_interrupt;
	i2c_mgm_pca95xx_get_state_t get_state;
	i2c_mgm_pca95xx_int_config_t pin_configure;
};

typedef int (*i2c_mgm_pca95xx_ch_config_t)(const struct device *, uint32_t);
typedef int (*i2c_mgm_pca95xx_ch_transfer_t)(const struct device *, struct i2c_msg *, uint8_t, uint16_t);

struct i2c_mgm_pca95xx_channel_api_t {
	i2c_mgm_pca95xx_ch_config_t config;
	i2c_mgm_pca95xx_ch_transfer_t transfer;
};

#endif /* MODULES_PCA9545A_PCA9545_H_ */
