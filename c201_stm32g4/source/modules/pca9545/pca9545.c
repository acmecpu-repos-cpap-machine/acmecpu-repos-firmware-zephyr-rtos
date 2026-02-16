/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT ti_pca9545a

/**
 * @file Driver for [PT]CA95XX I2C Bus Multiplexer.
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>

#include "pca9545.h"
#include "pca9545_reg.h"

#define LOG_LEVEL CONFIG_I2C_MGM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_mgm_pca95xx);

/*
 static inline int pca9545_reg_read(struct i2c_mgm_pca95xx_master_data *driver,
 uint8_t reg, uint8_t * const val)
 {
 return i2c_reg_read_byte(driver->i2c_master,
 DT_INST_REG_ADDR(0),
 reg, val);
 }

 static inline int pca9545_reg_write(struct i2c_mgm_pca95xx_data *driver,
 uint8_t reg, uint8_t val)
 {
 return i2c_reg_write_byte(driver->i2c_master,
 DT_INST_REG_ADDR(0),
 reg, val);
 }

 static inline int pca9545_reg_update(struct i2c_mgm_pca95xx_data *driver, uint8_t reg,
 uint8_t mask, uint8_t val)
 {
 return i2c_reg_update_byte(driver->i2c_master,
 DT_INST_REG_ADDR(0),
 reg, mask, val);
 }

 static inline int pca9545_set_hardware_config(const struct device *dev)
 {
 struct i2c_mgm_pca95xx_data *data = dev->data;
 return i2c_write(data->i2c_master, NULL, sizeof(NULL),	DT_INST_REG_ADDR(0));
 }

 static int pca9545_init_device(const struct device *dev)
 {
 struct i2c_mgm_pca95xx_data *data = dev->data;
 return i2c_write(data->i2c_master, NULL, sizeof(NULL),  DT_INST_REG_ADDR(0));
 }
 */
static int i2c_mgm_pca95xx_switch_init(const struct device *dev) {
	struct i2c_mgm_pca95xx_data *data = (struct i2c_mgm_pca95xx_data*) dev->data;
	const struct i2c_mgm_pca95xx_cfg *config =
			(const struct i2c_mgm_pca95xx_cfg*) dev->config;

   	if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}


	data->i2c_bus = config->i2c_master.bus;// device_get_binding(config->i2c_bus_dev_name);
	k_mutex_init(&data->lock);
//	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->irq_sync_sem, 0, 1);

	return 0;
}

static int i2c_mgm_pca95xx_channel_init(const struct device *dev) {
	struct i2c_mgm_pca95xx_ch_data *data =
			(struct i2c_mgm_pca95xx_ch_data*) dev->data;
	const struct i2c_mgm_pca95xx_ch_cfg *config =
			(const struct i2c_mgm_pca95xx_ch_cfg*) dev->config;
	data->switch_dev = device_get_binding(config->switch_name);
	return 0;
}

static int i2c_mgm_pca95xx_ch_config(const struct device *dev, uint32_t config) {
	return 0;
}

static inline int i2c_mgm_pca95xx_select_ch(const struct device *dev,
		const uint8_t *buf, uint32_t num_bytes, uint16_t addr) {

	return i2c_write(dev, buf, num_bytes, addr);
}

static int i2c_mgm_pca95xx_ch_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr) {
	struct i2c_mgm_pca95xx_ch_data *ch_data = (struct i2c_mgm_pca95xx_ch_data*) dev->data;
	const struct i2c_mgm_pca95xx_ch_cfg *ch_cfg = (const struct i2c_mgm_pca95xx_ch_cfg*) dev->config;

	struct i2c_mgm_pca95xx_data *switch_data = (struct i2c_mgm_pca95xx_data*) ch_data->switch_dev->data;
	const struct i2c_mgm_pca95xx_cfg *switch_cfg = (const struct i2c_mgm_pca95xx_cfg*) ch_data->switch_dev->config;

	const struct i2c_driver_api *i2c_bus_api = (const struct i2c_driver_api*) switch_data->i2c_bus->api;

	int ret = 0;
	uint8_t channel = ch_cfg->channel_number;
	channel = BIT(channel);

	k_mutex_lock(&switch_data->lock, K_FOREVER);
//	k_sem_take(&switch_data->lock, K_FOREVER);
	ret = i2c_mgm_pca95xx_select_ch(switch_data->i2c_bus, &channel, 1, switch_cfg->i2c_master.addr);
	if (ret != 0) {
		LOG_ERR("i2c_mgm_pca95xx_select_ch failed, %d", ret);
	}
	ret = i2c_bus_api->transfer(switch_data->i2c_bus, msgs, num_msgs, addr);
	if (ret != 0) {
		LOG_ERR("i2c_bus_api->transfer failed, %d", ret);;
	}
	k_mutex_unlock(&switch_data->lock);
//	k_sem_give(&switch_data->lock);
	return ret;
}

static int i2c_mgm_pca95xx_slave_int_register(const struct device *port,
		gpio_pin_t pin, gpio_flags_t flags) {
	return 0;
}

static struct i2c_mgm_pca95xx_master_api_t i2c_mgm_pca95xx_switch_api = {
		.reset = NULL, .get_interrupt = NULL, .get_state = NULL,
		.pin_configure = i2c_mgm_pca95xx_slave_int_register, };

//static struct i2c_mgm_pca95xx_channel_api_t i2c_mgm_pca95xx_channel_api = {
static const struct i2c_driver_api i2c_mgm_pca95xx_ch_api = {
		.configure = i2c_mgm_pca95xx_ch_config,
		.transfer = i2c_mgm_pca95xx_ch_transfer,
		.recover_bus = NULL,
};

/*#define LABEL_AND_COMMA(node_id) DT_LABEL(node_id),*/
#define LABEL_AND_COMMA(node_id) DT_PROP(DT_PARENT(channel),label),
#define CAT(x,y) x##_##y
#define CAT_NEST(x,y) CAT(x,y)

#define I2C_MGM_PCA95XX_CHANNEL_INIT(channel) \
\
static const struct i2c_mgm_pca95xx_ch_cfg CAT_NEST(i2c_mgm_cfg, CAT(inst, channel)) = \
{ \
	.channel_number = DT_REG_ADDR(channel), \
	/*.switch_name = DT_LABEL(DT_PARENT(channel)),*/ \
	  .switch_name = DT_PROP(DT_PARENT(channel),label), \
}; \
\
struct i2c_mgm_pca95xx_ch_data CAT_NEST(i2c_mgm_data, CAT(inst, channel)); \
\
DEVICE_DT_DEFINE(channel, \
	i2c_mgm_pca95xx_channel_init, \
	NULL, \
	&CAT_NEST(i2c_mgm_data, CAT(inst, channel)), \
	&CAT_NEST(i2c_mgm_cfg, CAT(inst, channel)), \
	POST_KERNEL, CONFIG_I2C_MGM_PCA95XX_CH_INIT_PRIORITY, \
	&i2c_mgm_pca95xx_ch_api);


#define I2C_MGM_PCA95XX_DEVICE_INSTANCE(inst) \
\
static const struct i2c_mgm_pca95xx_cfg i2c_mgm_##inst##_cfg = { \
		/*.i2c_bus_dev_name = DT_INST_BUS_LABEL(inst), \
		.i2c_slave_addr = DT_INST_REG_ADDR(inst), */			\
		.i2c_master = I2C_DT_SPEC_INST_GET(inst),		       	\
		IF_ENABLED(CONFIG_I2C_MGM_PCA95XX_INTERRUPT, ( 			\
			IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios), ( \
				/*.int_gpio_port = DT_INST_GPIO_LABEL(inst, interrupt_gpios), \
				.int_gpio_pin = DT_INST_GPIO_PIN(inst, interrupt_gpios), \
				.int_gpio_flags = DT_INST_GPIO_FLAGS(inst, interrupt_gpios),*/ \
				.int_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios), \
		)))) \
}; \
\
static struct i2c_mgm_pca95xx_data i2c_mgm_##inst##_data;                \
\
DEVICE_DT_INST_DEFINE( \
		inst,  \
		i2c_mgm_pca95xx_switch_init, \
		device_pm_control_nop, \
		&i2c_mgm_##inst##_data, \
		&i2c_mgm_##inst##_cfg, \
		POST_KERNEL, \
		CONFIG_I2C_MGM_PCA95XX_INIT_PRIORITY, \
		&i2c_mgm_pca95xx_switch_api); \
\
DT_INST_FOREACH_CHILD(inst, I2C_MGM_PCA95XX_CHANNEL_INIT);

DT_INST_FOREACH_STATUS_OKAY(I2C_MGM_PCA95XX_DEVICE_INSTANCE)
