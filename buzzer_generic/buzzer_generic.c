/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT bsp_buzzer

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>

#define LOG_LEVEL CONFIG_BUZZER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(buzzer);

#include "buzzer_generic.h"

/** Configuration data */
struct buzzer_config {

	/* Enable pin definition */
//	const char *pwm_dev_name;
//	uint32_t pwm_ch;
//	uint32_t pwm_period;
//	uint32_t pwm_flags;
	const struct pwm_dt_spec spec;
};

struct buzzer_data {
	struct k_sem lock;
	const struct device *buzzer_dev;
};

/* Init function */
static int buzzer_init(const struct device *dev) {
	const struct buzzer_config *config = dev->config;
	struct buzzer_data *data = dev->data;
	int ret = -1;

	if (!device_is_ready(config->spec.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

//	data->buzzer_dev = device_get_binding(config->pwm_dev_name);

	/* Disable the buzzer initially */
	uint32_t pwm_period = config->spec.period;
	uint32_t pwm_pulse = 0;
//	pwm_flags_t pwm_flags = config->spec.flags;

//	ret = pwm_pin_set_usec(data->buzzer_dev, config->pwm_ch, pwm_period, pwm_pulse, pwm_flags);
//	ret = pwm_set(config->spec.dev, config->spec.channel, config->spec.period, pwm_pulse, pwm_flags);
	ret = pwm_set_dt(&config->spec, PWM_USEC(pwm_period), PWM_USEC(pwm_pulse));
	if (ret != 0) {
		printk("pwm_set for channel %d failed\n", config->spec.channel);
		return -1;
	}

	return 0;
}

/* API Definitions */
static int set_buzzer_on(const struct device *dev) {
	const struct buzzer_config *config = dev->config;
	struct buzzer_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Enable the buzzer */
	uint32_t pwm_period = config->spec.period;
	uint32_t pwm_pulse = (pwm_period * 0.5);
	ret = pwm_set_dt(&config->spec, PWM_USEC(pwm_period), PWM_USEC(pwm_pulse));
	if (ret != 0) {
		printk("pwm_set for channel %d failed\n", config->spec.channel);
		goto err;
	}

	err: k_sem_give(&data->lock);
	return ret;
}

static int set_buzzer_off(const struct device *dev) {
	const struct buzzer_config *config = dev->config;
	struct buzzer_data *data = dev->data;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	/* Disable the buzzer */
	uint32_t pwm_period = config->spec.period;
	uint32_t pwm_pulse = 0;
	ret = pwm_set_dt(&config->spec, PWM_USEC(pwm_period), PWM_USEC(pwm_pulse));
	if (ret != 0) {
		printk("pwm_set for channel %d failed\n", config->spec.channel);
		goto err;
	}

	err: k_sem_give(&data->lock);
	return ret;
}

static const struct buzzer_driver_api buzzer_drv_api_funcs = {
	.buzzer_on = set_buzzer_on,
	.buzzer_off = set_buzzer_off,
};

#define DEVICE_INSTANCE(inst)																	\
																								\
const static struct buzzer_config buzzer_##inst##_cfg = {									\
/*       .pwm_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst, buzzer_ctrl),    \
		.pwm_ch = DT_INST_PWMS_CHANNEL_BY_NAME(inst, buzzer_ctrl),\
		.pwm_period = DT_INST_PWMS_PERIOD_BY_NAME(inst, buzzer_ctrl),\
		.pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst, buzzer_ctrl),\
		.spec = PWM_DT_SPEC_GET_BY_NAME(DT_NODELABEL(buzz), buzzer_ctrl), */\
		.spec = PWM_DT_SPEC_INST_GET_BY_NAME(inst, buzzer_ctrl),\
};																								\
																				\
static struct buzzer_data buzzer_##inst##_drvdata;							\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		buzzer_init,															\
		device_pm_control_nop,													\
		&buzzer_##inst##_drvdata,											\
		&buzzer_##inst##_cfg,												\
		APPLICATION, CONFIG_BUZZER_GENERIC_INIT_PRIORITY,							\
		&buzzer_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);

