/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT tps61169

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#define LOG_LEVEL CONFIG_TPS61169_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tps61169);

#include "tps61169.h"

/** Configuration data */
struct tsp61169_config {

	/* Enable pin definition */
//	const char *pwm_dev_name;
//	uint32_t pwm_ch;
//	uint32_t pwm_period;
//	uint32_t pwm_flags;
	const struct pwm_dt_spec brgt_pwm;	/* display brightness control */
};

struct tsp61169_data {
	struct k_sem lock;
};

/* Init function */
static int tps61169_init(const struct device *dev) {
	const struct tsp61169_config *config = dev->config;
	struct tsp61169_data *data = dev->data;
	int ret = -1;

	k_sem_init(&data->lock, 1, 1);

	if (!device_is_ready(config->brgt_pwm.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}
	uint32_t period_us = (config->brgt_pwm.period);
	float duty_factor = 0;//(float)(config->vref_pwm_duty/100);

	ret = pwm_set_dt(&config->brgt_pwm, PWM_USEC(period_us), PWM_USEC(period_us * duty_factor));
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d)", period_us);
		return ret;
	}

#if 0
	const struct device *pwm_dev = device_get_binding(config->pwm_dev_name);

	/* Disable the display power initially */
	uint32_t pwm_period = config->pwm_period;
	uint32_t pwm_pulse = 0;
	pwm_flags_t pwm_flags = config->pwm_flags;

	ret = pwm_pin_set_usec(pwm_dev, config->pwm_ch, pwm_period, pwm_pulse, pwm_flags);
	if (ret != 0) {
		LOG_ERR("pwm_pin_set_cycles for %s, channel %d failed", config->pwm_dev_name, config->pwm_ch);
		return -1;
	}
#endif
	return 0;
}

/* API Definitions */
static int tps61169_set_brightness(const struct device *dev, uint8_t level) {
	const struct tsp61169_config *config = dev->config;
	struct tsp61169_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	uint32_t period_us = (config->brgt_pwm.period);
	float mul = (float) level / 100;
	uint32_t pwm_pulse = (period_us * mul);

	ret = pwm_set_dt(&config->brgt_pwm, PWM_USEC(period_us), PWM_USEC(pwm_pulse));
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d)", period_us);
		goto err;
	}

#if 0
	const struct device *pwm_dev = device_get_binding(config->pwm_dev_name);
	float mul = (float) level / 100;
	uint32_t pwm_period = config->pwm_period;
	uint32_t pwm_pulse = (pwm_period * mul);
	pwm_flags_t pwm_flags = config->pwm_flags;

	ret = pwm_pin_set_usec(pwm_dev, config->pwm_ch, pwm_period, pwm_pulse, pwm_flags);

	if (ret != 0) {
		LOG_ERR("pwm_pin_set_cycles for %s, channel %d failed", config->pwm_dev_name, config->pwm_ch);
		goto err;
	}
#endif

err: k_sem_give(&data->lock);
	return ret;
}

static const struct tps61169_driver_api tps61169_drv_api_funcs = {
	.tps61169_set_brightness = tps61169_set_brightness,
};

#define DEVICE_INSTANCE(inst)																	\
																								\
const static struct tsp61169_config tps61169_##inst##_cfg = {									\
        /*.pwm_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst, disp_brightness),    \
		.pwm_ch = DT_INST_PWMS_CHANNEL_BY_NAME(inst, disp_brightness),\
		.pwm_period = DT_INST_PWMS_PERIOD_BY_NAME(inst, disp_brightness),\
		.pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst, disp_brightness), */		\
		.brgt_pwm = PWM_DT_SPEC_INST_GET_BY_NAME(inst, disp_brightness),					\
};																				\
																				\
static struct tsp61169_data tps61169_##inst##_drvdata;							\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		tps61169_init,															\
		device_pm_control_nop,													\
		&tps61169_##inst##_drvdata,											\
		&tps61169_##inst##_cfg,												\
		APPLICATION, CONFIG_TPS61169_INIT_PRIORITY,							\
		&tps61169_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);

