/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2020 Norbit ODM AS
 * Copyright (c) 2021 Acme CPU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Interrupt Extension Driver for PCA95XX I2C-based GPIO driver.
 */

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include "gpio_utils.h"
#include "gpio_pca95xx_reg.h"
#include "gpio_pca95xx_int.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_pca95xx_int);

/*
 * We are borrowing these structures from the gpio_pca95xx.c file
 * Not necessarily the best way to do so but
 * the structures of the pca95xx device driver are not exposed through
 * a header file.
 * We also need to give them new names other than the original ones
 */

/** Configuration data */
struct gpio_master_pca95xx_config {
	struct gpio_driver_config common;
	const char * const i2c_master_dev_name;
	uint16_t i2c_slave_addr;
	uint8_t capabilities;
#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	const char *int_gpio_port;
	gpio_pin_t int_gpio_pin;
	gpio_flags_t int_gpio_flags;
#endif
};

/** Runtime driver data */
struct gpio_master_pca95xx_drv_data {
	struct gpio_driver_data common;
	const struct device *i2c_master;
	struct {
		uint16_t input;
		uint16_t output;
		uint16_t dir;
		uint16_t pud_en;
		uint16_t pud_sel;
	} reg_cache;
	struct k_sem lock;
#ifdef CONFIG_GPIO_PCA95XX_INTERRUPT
	/* Self-reference to the driver instance */
	const struct device *instance;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* interrupt triggering pin masks */
	struct {
		uint16_t edge_rising;
		uint16_t edge_falling;
		uint16_t level_high;
		uint16_t level_low;
	} interrupts;
	struct gpio_callback gpio_callback;
	struct k_work interrupt_worker;
	bool interrupt_active;
#endif
};

static const struct device *pca95xx_int_ext_dev;

static int gpio_pca95xx_int_ext_irq_enable(const struct device *port, gpio_pin_t pin)
{
	int ret = 0;
	const struct gpio_pca95xx_int_ext_drv_cfg *pca95xx_int_ext_cfg = pca95xx_int_ext_dev->config;
	struct gpio_pca95xx_int_ext_drv_data *pca95xx_int_ext_data = pca95xx_int_ext_dev->data;
	uint8_t i = 0;

	while (!(pca95xx_int_ext_data->port_interrupt_data[i].pca95xx_device == port))
	{
		i++;
		if(i == pca95xx_int_ext_cfg->pca95xx_device_num)
			return -EOPNOTSUPP;
	}

	const struct gpio_master_pca95xx_config *port_cfg = port->config;
	struct gpio_master_pca95xx_drv_data *port_data = port->data;
	uint16_t i2c_address = port_cfg->i2c_slave_addr;

	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch |= BIT(pin);
	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask &= ~BIT(pin);

	k_sem_take(&port_data->lock, K_FOREVER);
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INPUT_LATCH_PORT0,\
				(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch,\
					sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch));
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INT_MASK_PORT0,\
			(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask,
				sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask));
	k_sem_give(&port_data->lock);
	return ret;
}

static int gpio_pca95xx_int_ext_irq_disable(const struct device *port, gpio_pin_t pin)
{
	int ret = 0;
	const struct gpio_pca95xx_int_ext_drv_cfg *pca95xx_int_ext_cfg = pca95xx_int_ext_dev->config;
	struct gpio_pca95xx_int_ext_drv_data *pca95xx_int_ext_data = pca95xx_int_ext_dev->data;
	uint8_t i = 0;

	while (!(pca95xx_int_ext_data->port_interrupt_data[i].pca95xx_device == port))
	{
		i++;
		if(i == pca95xx_int_ext_cfg->pca95xx_device_num)
			return -EOPNOTSUPP;
	}

	const struct gpio_master_pca95xx_config *port_cfg = port->config;
	struct gpio_master_pca95xx_drv_data *port_data = port->data;
	uint16_t i2c_address = port_cfg->i2c_slave_addr;

	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch &= ~BIT(pin);
	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask |= BIT(pin);

	k_sem_take(&port_data->lock, K_FOREVER);
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INPUT_LATCH_PORT0,\
				(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch,\
					sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch));
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INT_MASK_PORT0,\
				(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask,\
					sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.int_mask));
	k_sem_give(&port_data->lock);
	return ret;
}

static int gpio_pca95xx_int_ext_latch_enable(const struct device *port, gpio_pin_t pin)
{
	int ret = 0;
	const struct gpio_pca95xx_int_ext_drv_cfg *pca95xx_int_ext_cfg = pca95xx_int_ext_dev->config;
	struct gpio_pca95xx_int_ext_drv_data *pca95xx_int_ext_data = pca95xx_int_ext_dev->data;
	uint8_t i = 0;

	while (!(pca95xx_int_ext_data->port_interrupt_data[i].pca95xx_device == port))
	{
		i++;
		if(i == pca95xx_int_ext_cfg->pca95xx_device_num)
			return -EOPNOTSUPP;
	}

	const struct gpio_master_pca95xx_config *port_cfg = port->config;
	struct gpio_master_pca95xx_drv_data *port_data = port->data;
	uint16_t i2c_address = port_cfg->i2c_slave_addr;

	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch |= BIT(pin);

	k_sem_take(&port_data->lock, K_FOREVER);
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INPUT_LATCH_PORT0,\
				(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch,\
					sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch));
	k_sem_give(&port_data->lock);
	return ret;
}

static int gpio_pca95xx_int_ext_latch_disable(const struct device *port, gpio_pin_t pin)
{
	int ret = 0;
	const struct gpio_pca95xx_int_ext_drv_cfg *pca95xx_int_ext_cfg = pca95xx_int_ext_dev->config;
	struct gpio_pca95xx_int_ext_drv_data *pca95xx_int_ext_data = pca95xx_int_ext_dev->data;
	uint8_t i = 0;

	while (!(pca95xx_int_ext_data->port_interrupt_data[i].pca95xx_device == port))
	{
		i++;
		if(i == pca95xx_int_ext_cfg->pca95xx_device_num)
			return -EOPNOTSUPP;
	}

	const struct gpio_master_pca95xx_config *port_cfg = port->config;
	struct gpio_master_pca95xx_drv_data *port_data = port->data;
	uint16_t i2c_address = port_cfg->i2c_slave_addr;

	pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch &= ~BIT(pin);

	k_sem_take(&port_data->lock, K_FOREVER);
	ret |= i2c_burst_write(port_data->i2c_master, i2c_address, REG_INPUT_LATCH_PORT0,\
				(const uint8_t *)&pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch,\
					sizeof(pca95xx_int_ext_data->port_interrupt_data[i].reg_cache.input_latch));
	k_sem_give(&port_data->lock);
	return ret;
}

static int gpio_pca95xx_int_ext_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	int ret = 0;
	enum gpio_int_trig trig;
	enum gpio_int_mode mode;
	trig = (enum gpio_int_trig)(flags & (GPIO_INT_LOW_0 | GPIO_INT_HIGH_1));
	mode = (enum gpio_int_mode)(flags & (GPIO_INT_EDGE | GPIO_INT_DISABLE | GPIO_INT_ENABLE));
	if((mode & GPIO_INT_ENABLE) == GPIO_INT_ENABLE)
	{
		ret |= gpio_pca95xx_int_ext_irq_enable(port, pin);
	}
	ret |= gpio_pin_configure(port, pin, flags);
	return ret;
}

static const struct gpio_pca95xx_int_ext_api gpio_pca95xx_int_ext_drv = {
		.pin_configure = gpio_pca95xx_int_ext_pin_configure,
		.latch_enable = gpio_pca95xx_int_ext_latch_enable,
		.latch_disable = gpio_pca95xx_int_ext_latch_disable,
		.pca95xx_irq_enable = gpio_pca95xx_int_ext_irq_enable,
		.pca95xx_irq_disable = gpio_pca95xx_int_ext_irq_disable,
};

/**
 * @brief Initialization function of PCA95XX Interrupt Extension Driver
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int gpio_pca95xx_int_ext_init(const struct device *dev)
{
	pca95xx_int_ext_dev = dev;
	const struct gpio_pca95xx_int_ext_drv_cfg *cfg = dev->config;
	struct gpio_pca95xx_int_ext_drv_data *data = dev->data;

	for (uint8_t i = 0; i < cfg->pca95xx_device_num; i++)
	{
		data->port_interrupt_data[i].pca95xx_device = device_get_binding(cfg->pca95xx_device_names[i]);
		if (!data->port_interrupt_data[i].pca95xx_device)
		{
			return -EINVAL;
		}
		data->port_interrupt_data[i].reg_cache.input_latch = 0x0000;
		data->port_interrupt_data[i].reg_cache.int_mask = 0xFFFF;
		data->port_interrupt_data[i].reg_cache.int_status = 0x0000;
	}
	return 0;
}

#define CAT(x,y) x##y
#define DT_DRV_COMPAT nxp_pca95xx
#define LABELS_TO_ARRAY(inst) DT_INST_LABEL(inst),

#define GPIO_PCA95XX_INT_EXTENSION_DRV				\
\
	const char *pca95xx_compat_labels[] = {\
		DT_INST_FOREACH_STATUS_OKAY(LABELS_TO_ARRAY)};\
\
	static const struct gpio_pca95xx_int_ext_drv_cfg gpio_pca95xx_int_ext_cfg = {\
		.pca95xx_device_names = pca95xx_compat_labels,\
		.pca95xx_device_num	= ARRAY_SIZE(pca95xx_compat_labels),\
	};\
\
	static struct gpio_pca95xx_port_data\
		gpio_pca95xx_port_data[ARRAY_SIZE(pca95xx_compat_labels)];\
\
	static struct gpio_pca95xx_int_ext_drv_data gpio_pca95xx_int_ext_data = {\
		.port_interrupt_data = gpio_pca95xx_port_data,\
	};\
\
	DEVICE_AND_API_INIT(gpio_pca95xx_int_ext,\
		"gpio_pca95xx_int_ext", gpio_pca95xx_int_ext_init,\
		&gpio_pca95xx_int_ext_data,	&gpio_pca95xx_int_ext_cfg,			\
		POST_KERNEL, CONFIG_GPIO_PCA95XX_INT_EXT_INIT_PRIORITY,			\
		&gpio_pca95xx_int_ext_drv);

//DT_INST_FOREACH_STATUS_OKAY(GPIO_PCA95XX_INT_INSTANCE)
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_pca95xx)
GPIO_PCA95XX_INT_EXTENSION_DRV
#endif
