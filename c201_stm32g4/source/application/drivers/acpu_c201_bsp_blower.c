/*
 * Copyright (c) 2021 Acme CPU
 */

#include <drivers/gpio.h>
#include <drivers/adc.h>
#include <drivers/dac.h>
#include <stdint.h>
#include <devicetree.h>
#include <sys/util.h>
#include "bsp_gpios.h"
#include "acpu_c201_bsp_blower.h"

#define DT_DRV_COMPAT acpu_blower

struct bsp_blower_cfg {
	const char *speed_ctrl_dev_name;
	int speed_ctrl_ch;
	int speed_ctrl_period;
	int speed_ctrl_flags;
	const char *speed_fb_dev_name;
	int speed_fb_ch;
	int speed_fb_period;
	int speed_fb_flags;
	const char *boost_ps_dev_name;
};

struct bsp_blower_data {
	const struct device *speed_ctrl_dev;
	const struct device *speed_fb_dev;
	const struct device *boost_ps_dev;
};

struct bsp_boost_ps_cfg {
	const char * voltage_ctrl_dev_name;
	int voltage_ctrl_ch;
	const char * voltage_fb_dev_name;
	int voltage_fb_ch;
	const char * enable_gpio_dev_name;
	int enable_gpio_pin;
	int enable_gpio_flags;
	int ref_voltage;
	int fb_voltage;
	int shunt_res;
	int vdiv_bot_res;
	int vdiv_top_res;
	int vdiv_aux_res;
};

struct bsp_boost_ps_data {
	const struct device *voltage_ctrl_dev;
	const struct device *voltage_fb_dev;
	const struct device *enable_gpio_dev;
};

static uint16_t bsp_blower_speed_get(const struct device *dev)
{
	return 0;
}

static int bsp_blower_dir_ctrl(const struct device *dev, uint8_t dir)
{
	int ret = 0;
	//const struct device *drv10975_dev = device_get_binding("BLDC_MOTOR_DRIVER");
	//const struct drv10975_api *bldc_api = drv10975_dev->api;
	//ret = bldc_api->direction(drv10975_dev, dir);
	return ret;
}

static int bsp_blower_speed_ctrl_adjust(const struct device *dev, uint16_t speed)
{
	return 0;
}

#define VREG(vfb, r_top, r_bot)	((vfb) * (r_top) / (r_bot) + (vfb))
#define DAC_MAX_CODE	(BIT(12) - 1)
#define VOUT_LIM(v, min, max) ((v) < (max) ? ((v) > (min) ? (v) : (min)) : (max))
#define VADD_MAX(vref, r_aux, r_shunt)	((vref) * (r_aux) / (r_shunt))
#define KT(vref, r_aux, r_shunt)	((r_shunt) * (DAC_MAX_CODE))

static int bsp_boost_ps_voltage_adjust(const struct device *dev, uint16_t vout)
{
	const struct bsp_boost_ps_cfg *cfg = dev->config;
	uint16_t vout_min = VREG(cfg->fb_voltage,cfg->vdiv_top_res,cfg->vdiv_bot_res);
	uint16_t vout_max = vout_min + VADD_MAX(cfg->ref_voltage, cfg->vdiv_aux_res, cfg->shunt_res);
	vout = VOUT_LIM(vout, vout_min, vout_max);
	uint16_t vout_code = (vout * cfg->shunt_res * DAC_MAX_CODE - vout_min * cfg->shunt_res * DAC_MAX_CODE)\
														/ (cfg->vdiv_aux_res * cfg->ref_voltage);
	const struct bsp_boost_ps_data *data = dev->data;
	const struct device *voltage_ctrl = data->voltage_ctrl_dev;
	const struct dac_driver_api *voltage_ctrl_api = voltage_ctrl->api;
	return voltage_ctrl_api->write_value(voltage_ctrl, cfg->voltage_ctrl_ch, vout_code);
}

static uint16_t bsp_boost_ps_voltage_feedback(const struct device *dev)
{
	const struct bsp_boost_ps_cfg *cfg = dev->config;
	const struct bsp_boost_ps_data *data = dev->data;
	const struct device *voltage_fb = data->voltage_fb_dev;
	const struct adc_driver_api *voltage_fb_api = voltage_fb->api;
	//return voltage_fb_api->read(voltage_fb, cfg->voltage_fb_ch);
	return 0;
}

static int bsp_blower_enable(const struct device *dev)
{
	int ret = 0;
	struct bsp_blower_data *data = ((const struct device *)dev)->data;
	const struct device *en_pin_port = ((struct bsp_boost_ps_data *)data->boost_ps_dev)->enable_gpio_dev;
	gpio_port_pins_t en_pin = ((struct bsp_boost_ps_cfg *)data->boost_ps_dev)->enable_gpio_pin;
	const struct gpio_driver_api *en_pin_drv = en_pin_port->api;
	ret = en_pin_drv->port_set_bits_raw(en_pin_port, en_pin);
	return ret;
}

static int bsp_blower_disable(const struct device *dev)
{
	int ret = 0;
	struct bsp_blower_data *data = ((const struct device *)dev)->data;
	const struct device *en_pin_port = ((struct bsp_boost_ps_data *)data->boost_ps_dev)->enable_gpio_dev;
	gpio_port_pins_t en_pin = ((struct bsp_boost_ps_cfg *)data->boost_ps_dev)->enable_gpio_pin;
	const struct gpio_driver_api *en_pin_drv = en_pin_port->api;
	ret = en_pin_drv->port_clear_bits_raw(en_pin_port, en_pin);
	return ret;
}

static int bsp_blower_init(const struct device *dev)
{
	int ret = 0;
	const struct bsp_blower_cfg *cfg = dev->config;
	struct bsp_blower_data *data = dev->data;
	data->speed_ctrl_dev = device_get_binding(cfg->speed_ctrl_dev_name);
	data->speed_fb_dev = device_get_binding(cfg->speed_fb_dev_name);
	data->boost_ps_dev = device_get_binding(cfg->boost_ps_dev_name);
	return ret;
}

static int bsp_boost_ps_init(const struct device *dev)
{
	int ret = 0;
	const struct bsp_boost_ps_cfg *cfg = dev->config;
	struct bsp_boost_ps_data *data = dev->data;
	data->enable_gpio_dev = device_get_binding(cfg->enable_gpio_dev_name);
	data->voltage_ctrl_dev = device_get_binding(cfg->voltage_ctrl_dev_name);
	data->voltage_fb_dev = device_get_binding(cfg->voltage_fb_dev_name);
	return ret;
}

static const struct bsp_blower_driver_api bsp_blower_api = {
		.enable = bsp_blower_enable,
		.disable = bsp_blower_disable,
		.ps_ctrl = bsp_boost_ps_voltage_adjust,
		.ps_feedback = bsp_boost_ps_voltage_feedback,
		.pwm_ctrl = bsp_blower_speed_ctrl_adjust,
		.dir_ctrl = bsp_blower_dir_ctrl,
		.speed_get = bsp_blower_speed_get,
};

#define CAT(x,y)	x##y

#define BOOST_PS(node_id)\
	const static struct bsp_boost_ps_cfg CAT(node_id,_cfg) = {					\
		.voltage_ctrl_dev_name = DT_IO_CHANNELS_LABEL_BY_NAME(node_id, voltage_ctrl),	\
		.voltage_ctrl_ch = DT_PHA_BY_NAME(node_id, io_channels, voltage_ctrl, output),\
		.voltage_fb_dev_name = DT_IO_CHANNELS_LABEL_BY_NAME(node_id, voltage_feedback),\
		.voltage_fb_ch = DT_IO_CHANNELS_INPUT_BY_NAME(node_id, voltage_feedback),\
		.enable_gpio_dev_name = DT_GPIO_LABEL(node_id,enable_gpios),\
		.enable_gpio_pin = DT_GPIO_PIN(node_id, enable_gpios),\
		.enable_gpio_flags = DT_GPIO_FLAGS(node_id, enable_gpios),\
		.ref_voltage = DT_PROP(node_id,vref),\
		.fb_voltage = DT_PROP(node_id,vfb),\
		.shunt_res = DT_PROP(node_id,v_i_conv_shunt),\
		.vdiv_bot_res = DT_PROP(node_id,vdiv_bot_resistor),\
		.vdiv_top_res = DT_PROP(node_id,vdiv_top_resistor),\
		.vdiv_aux_res = DT_PROP(node_id,vdiv_aux_resistor),\
	};																				\
\
	static struct bsp_boost_ps_data CAT(node_id,_data);							\
\
	DEVICE_DEFINE(CAT(boost_ps,node_id), DT_LABEL(node_id),						\
		bsp_boost_ps_init, NULL,													\
		&CAT(node_id,_data), &CAT(node_id,_cfg),					\
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,NULL);


#define BLOWER_INST(inst)\
\
	const static struct bsp_blower_cfg bsp_blower_##inst##_cfg = {						\
		.speed_ctrl_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst,speed_ctrl),		\
		.speed_fb_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst,speed_feedback),	\
		.boost_ps_dev_name = DT_LABEL(DT_CHILD(DT_DRV_INST(inst),dcdc_converter)),\
	};																				\
\
	static struct bsp_blower_cfg bsp_blower_##inst##_data;								\
\
	DEVICE_DEFINE(bsp_blower_drv##inst, DT_INST_LABEL(inst), bsp_blower_init, NULL,		\
		&bsp_blower_##inst##_data, &bsp_blower_##inst##_cfg,						\
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,								\
		&bsp_blower_api);\
\
	BOOST_PS(DT_CHILD(DT_DRV_INST(inst),dcdc_converter));

DT_INST_FOREACH_STATUS_OKAY(BLOWER_INST);
