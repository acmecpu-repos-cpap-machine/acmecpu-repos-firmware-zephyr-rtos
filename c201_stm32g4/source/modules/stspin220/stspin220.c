/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT st_stspin220

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include "stspin220.h"
#define LOG_LEVEL CONFIG_STSPIN220_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stspin220);

#define DUTY_CYCLE				50
#define STEP_CLOCK_BY_TIMER		1

#define MASK_STEP_MODE1		(1 << 1)
#define MASK_STEP_MODE2		(1 << 0)
#define MASK_STEP_MODE3		(1 << 3)
#define MASK_STEP_MODE4		(1 << 2)


/** Configuration data */
struct stspin220_config {
	uint8_t capabilities;

#if !STEP_CLOCK_BY_TIMER
	/* Step clock PWM definition */
//	const char * step_clk_dev_name;
//	uint32_t step_clk_pwm_pin;
//	uint32_t step_clk_pwm_period_ns;
//	pwm_flags_t step_clk_pwm_flags;
	const struct pwm_dt_spec step_clk_pwm;
#endif

	/* Vref PWM definition */
//	const char * vref_dev_name;
//	uint32_t vref_pwm_pin;
//	uint32_t vref_pwm_period_ns;
//	pwm_flags_t vref_pwm_flags;
	const struct pwm_dt_spec vref_pwm;
	uint8_t vref_pwm_duty;

#if STEP_CLOCK_BY_TIMER
	/* Step Clock pin definition */
//	const char *step_clk_port;
//	gpio_pin_t step_clk_pin;
//	gpio_flags_t step_clk_flags;
	struct gpio_dt_spec step_clk_gpio;
#endif

	/* Step Mode 1 pin definition */
//	const char *step_mode1_gpio_port;
//	gpio_pin_t step_mode1_gpio_pin;
//	gpio_flags_t step_mode1_gpio_flags;
	struct gpio_dt_spec step_mode1_gpio;

	/* Step Mode 2 pin definition */
//	const char *step_mode2_gpio_port;
//	gpio_pin_t step_mode2_gpio_pin;
//	gpio_flags_t step_mode2_gpio_flags;
	struct gpio_dt_spec step_mode2_gpio;

	/* Direction pin definition */
//	const char *dir_gpio_port;
//	gpio_pin_t dir_gpio_pin;
//	gpio_flags_t dir_gpio_flags;
	struct gpio_dt_spec dir_gpio;

	/* Enable pin definition */
//	const char *en_gpio_port;
//	gpio_pin_t en_gpio_pin;
//	gpio_flags_t en_gpio_flags;
	struct gpio_dt_spec en_gpio;

	/* Standby pin definition */
//	const char *standby_gpio_port;
//	gpio_pin_t standby_gpio_pin;
//	gpio_flags_t standby_gpio_flags;
	struct gpio_dt_spec standby_gpio;

	/* Fault pin definition */
//	const char *fault_gpio_port;
//	gpio_pin_t fault_gpio_pin;
//	gpio_flags_t fault_gpio_flags;
	struct gpio_dt_spec fault_gpio;

	/* Vref gpio */
	struct gpio_dt_spec vref_gpio;
};

struct stspin220_data {
	struct k_sem lock;

	/* Running status of the motor */
	bool run_stat;

	/* to store the application handler function */
	stspin220_stop_handler_t stop_handler;

#if STEP_CLOCK_BY_TIMER
	/* step timer */
	struct k_timer step_timer;

	/* number of steps to take and live count of steps variables */
	uint32_t num_steps;
	uint32_t timer_expiry_count;

	/* Step clock device */
	const struct device *step_clk_dev;

	/* Self-reference to this driver (stspin220) instance */
	const struct device *instance;
#endif

	/* Step clock period and duty cycle, so that we can resume with previous set values */
	uint32_t step_clk_period_us;
	uint32_t step_clk_duty_cycle;
};

/* Static function declarations */
static int stspin220_standby(const struct device *dev);
static int stspin220_resume(const struct device *dev);

/* Static function definitions */
#if STEP_CLOCK_BY_TIMER
static void step_timer_handler(struct k_timer *tmr) {
	struct stspin220_data *data = CONTAINER_OF(tmr, struct stspin220_data, step_timer);
	const struct stspin220_config *config = data->instance->config;

	if (++data->timer_expiry_count >= data->num_steps) {
		k_timer_stop(&data->step_timer);
	}
//	gpio_pin_toggle(data->step_clk_dev, config->step_clk_pin);
	gpio_pin_toggle(config->step_clk_gpio.port, config->step_clk_gpio.pin);
}

static void step_timer_stopped_handler(struct k_timer *tmr) {
	struct stspin220_data *data = CONTAINER_OF(tmr, struct stspin220_data, step_timer);
	LOG_DBG("step_timer stopped");

	if (data->stop_handler != NULL)
		data->stop_handler(data->instance, (data->timer_expiry_count/2));
	data->run_stat = false;
}
#endif

static int stspin220_step_clock_stop(const struct device *dev) {
	struct stspin220_data *data = dev->data;
	int ret = 0;
#if STEP_CLOCK_BY_TIMER
	k_timer_stop(&data->step_timer);
#else
	const struct stspin220_config *config = dev->config;
	const struct device *step_clock_dev = device_get_binding(config->step_clk_dev_name);
	if (step_clock_dev == NULL) {
		LOG_ERR("Could not get step clock device");
		return -ENODEV;
	}

	ret = pwm_pin_set_usec(step_clock_dev, config->step_clk_pwm_pin, 0, 0, config->step_clk_pwm_flags);
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d) with duty cycle (%d)", 0, 0);
		return ret;
	}
#endif
	return ret;
}

static int stspin220_step_clock_start(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;
	int ret = 0;

#if STEP_CLOCK_BY_TIMER
//	ret = gpio_pin_set(data->step_clk_dev, config->step_clk_pin, 0);
	ret = gpio_pin_set(config->step_clk_gpio.port, config->step_clk_gpio.pin, 0);
	data->timer_expiry_count = 0;
	data->num_steps *= 2;	/* we need to count to double the steps as we are having a timer interval of half (50% duty) */
	float duty_factor = (float)DUTY_CYCLE/100;
	uint32_t interval_us = (data->step_clk_period_us * duty_factor);
	k_timer_start(&data->step_timer, K_USEC(interval_us), K_USEC(interval_us));

#else
	uint32_t period = data->step_clk_period_us;
	uint32_t duty_cycle = data->step_clk_duty_cycle;

	const struct device *step_clock_dev = device_get_binding(config->step_clk_dev_name);
	if (step_clock_dev == NULL) {
		LOG_ERR("Could not get step clock device");
		return -ENODEV;
	}

	float duty_factor = (float)duty_cycle/100;
	ret = pwm_pin_set_usec(step_clock_dev, config->step_clk_pwm_pin, period, (period * duty_factor), config->step_clk_pwm_flags);
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d) with duty cycle (%d)", period, duty_cycle);
		return ret;
	}
#endif

	return ret;
}

/* Init function */
static int stspin220_init(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;
	int ret = 0;

	k_sem_init(&data->lock, 1, 1);
	data->run_stat = false;
	data->stop_handler = NULL;

#if STEP_CLOCK_BY_TIMER
	/* store the driver instance */
	data->instance = dev;

	/* get the device reference of the step clock and save it */
//	data->step_clk_dev = device_get_binding(config->step_clk_port);
//	if (data->step_clk_dev == NULL) {
//		LOG_ERR("Could not get direction gpio device %s", config->step_clk_port);
//		return -ENODEV;
//	}
	if (device_is_ready(config->step_clk_gpio.port)) {
		ret = gpio_pin_configure(config->step_clk_gpio.port, config->step_clk_gpio.pin,
				(config->step_clk_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure direction gpio pin %d (%d)",
					config->step_clk_gpio.pin, ret);
			return ret;
		}
	} else {
		LOG_ERR("Could not get direction gpio pin %d",
				config->step_clk_gpio.pin);
		return -ENODEV;
	}

	/* Initialize a timer to generate step clock pulses */
	k_timer_init(&data->step_timer, step_timer_handler, step_timer_stopped_handler);
#endif

	data->step_clk_period_us = 10000;
	data->step_clk_duty_cycle = DUTY_CYCLE;

	/* Set reference voltage Vref */
//	const struct device *vref_pwm_dev = device_get_binding(config->vref_dev_name);
//	if (vref_pwm_dev == NULL) {
//		LOG_ERR("Could not get vref pwm device");
//		return -ENODEV;
//	}
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)	// TODO: Fixme, PWM not working on e206 board, hence using gpio
	if (device_is_ready(config->vref_gpio.port)) {
		ret = gpio_pin_configure(config->vref_gpio.port, config->vref_gpio.pin,
				(config->vref_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure vref gpio pin %d (%d)", config->vref_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->vref_gpio.port, config->vref_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Failed to set vref gpio pin %d (%d)", config->vref_gpio.pin, ret);
			return ret;
		}
	} else {
		LOG_ERR("Could not get vref gpio pin %d", config->vref_gpio.pin);
		return -ENODEV;
	}
#else
	if (!device_is_ready(config->vref_pwm.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}

	uint32_t period_us = (config->vref_pwm.period);
	float duty_factor = (float)(config->vref_pwm_duty/100);

//	ret = pwm_pin_set_usec(vref_pwm_dev, config->vref_pwm_pin, period_us, (period_us * duty_factor), config->vref_pwm_flags);
	ret = pwm_set_dt(&config->vref_pwm, PWM_USEC(period_us), PWM_USEC(period_us * duty_factor));
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d) with duty cycle (%d)", period_us, config->vref_pwm_duty);
		return ret;
	}
#endif
	return 0;
}

/* API Definitions */
static int stspin220_direction_set(const struct device *dev, const uint8_t dir) {
	const struct stspin220_config *config = dev->config;
	int ret = -1;

//	const struct device *dir_gpio_dev = device_get_binding(config->dir_gpio_port);
//	if (dir_gpio_dev == NULL) {
//		LOG_ERR("Could not get direction gpio device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->dir_gpio.port)) {
		ret = gpio_pin_configure(config->dir_gpio.port, config->dir_gpio.pin,
				(config->dir_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure direction gpio pin %d (%d)",
					config->dir_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->dir_gpio.port, config->dir_gpio.pin, dir);
		if (ret != 0) {
			LOG_ERR("Error setting direction GPIO pin (%d)", config->dir_gpio.pin);
			return ret;
		}
	} else {
		LOG_ERR("Could not get direction gpio device");
		return -ENODEV;
	}

	return 0;
}

static int stspin220_step_clock_set(const struct device *dev, const uint32_t period_us) {
//	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;
	int ret=0;

	if (period_us == 0) {
		LOG_ERR("Period cannot be zero!");
		return -EINVAL;
	}

	data->step_clk_period_us = period_us;
	data->step_clk_duty_cycle = DUTY_CYCLE;

	return ret;
}

static int stspin220_num_step_set(const struct device *dev, const uint32_t num_steps) {
	struct stspin220_data *data = dev->data;
	int ret = 0;

	if (data->run_stat) {
		LOG_ERR("Number of steps cannot be changed while to motor is running!");
		return -EINVAL;
	}

	if (num_steps == 0) {
		LOG_ERR("Number of steps cannot be zero!");
		return -EINVAL;
	}

	k_sem_take(&data->lock, K_FOREVER);
	data->num_steps = num_steps;
	k_sem_give(&data->lock);

	return ret;
}

static int stspin220_vref_pwm_set(const struct device *dev, const uint32_t period_us, const uint32_t duty_cycle) {
	const struct stspin220_config *config = dev->config;
	int ret = -1;

	/* Set reference voltage Vref */
//	const struct device *vref_pwm_dev = device_get_binding(config->vref_dev_name);
//	if (vref_pwm_dev == NULL) {
//		LOG_ERR("Could not get vref pwm device");
//		return -ENODEV;
//	}
	if (!device_is_ready(config->vref_pwm.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}

//	uint32_t pwm_period = config->vref_pwm_period_ns/1000;	// period in usecs
//	float pwm_pulse_mul = 0.88;	// evaluates to 0.44V Vref
	float duty_factor = (float)duty_cycle/100;

//	ret = pwm_pin_set_usec(vref_pwm_dev, config->vref_pwm_pin, period_us, (period_us * duty_factor), config->vref_pwm_flags);
	ret = pwm_set_dt(&config->vref_pwm, PWM_USEC(period_us), PWM_USEC(period_us * duty_factor));
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d) with duty cycle (%d)", period_us, duty_cycle);
		return ret;
	}

	return 0;
}

static int stspin220_step_mode_set(const struct device *dev, const uint8_t step_mode) {
	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;

	if (data->run_stat) {
		LOG_ERR("cannot set mode while motor is running");
		return -EBUSY;
	}

	bool step_mode1, step_mode2, step_mode3, step_mode4;

	step_mode1 = (step_mode & MASK_STEP_MODE1);
	step_mode2 = (step_mode & MASK_STEP_MODE2);
	step_mode3 = (step_mode & MASK_STEP_MODE3);
	step_mode4 = (step_mode & MASK_STEP_MODE4);

	int ret = 0;

	/* Set Step Mode 1 */
//	const struct device *step_mode1_dev = device_get_binding(config->step_mode1_gpio_port);
//	if (step_mode1_dev == NULL) {
//		LOG_ERR("Could not get step mode 1 gpio device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->step_mode1_gpio.port)) {
		ret = gpio_pin_configure(config->step_mode1_gpio.port, config->step_mode1_gpio.pin,
				(config->step_mode1_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure step mode 1 gpio pin %d (%d)",
					config->step_mode1_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->step_mode1_gpio.port, config->step_mode1_gpio.pin,
				step_mode1);
		if (ret != 0) {
			LOG_ERR("Error setting step mode 1 GPIO pin (%d)",
					config->step_mode1_gpio.pin);
			return ret;
		}
	}

	/* Set Step Mode 2 */
//	const struct device *step_mode2_dev = device_get_binding(config->step_mode2_gpio_port);
//	if (step_mode2_dev == NULL) {
//		LOG_ERR("Could not get step mode 2 gpio device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->step_mode2_gpio.port)) {
		ret = gpio_pin_configure(config->step_mode2_gpio.port, config->step_mode2_gpio.pin,
				(config->step_mode2_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure step mode 2 gpio pin %d (%d)",
					config->step_mode2_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->step_mode2_gpio.port, config->step_mode2_gpio.pin,
				step_mode2);
		if (ret != 0) {
			LOG_ERR("Error setting step mode 2 GPIO pin (%d)",
					config->step_mode2_gpio.pin);
			return ret;
		}
	}

	/* Set Step Mode 3 */
#if STEP_CLOCK_BY_TIMER
	// TODO
	ret = gpio_pin_set(config->step_clk_gpio.port, config->step_clk_gpio.pin, step_mode3);
	if (ret != 0) {
		LOG_ERR("Error setting step mode 3 GPIO pin (%d)", config->step_clk_gpio.pin);
		return ret;
	}

#else
	uint32_t period_ms = 0;
	if (step_mode3) {
		period_ms = 1000;
	}
//	const struct device *step_clock_dev = device_get_binding(config->step_clk_dev_name);
//	if (step_clock_dev == NULL) {
//		LOG_ERR("Could not get step clock device");
//		return -ENODEV;
//	}
	if (!device_is_ready(config->pwm.dev)) {
		LOG_ERR("PWM device not ready");
		return -ENODEV;
	}
//	ret = pwm_pin_set_usec(step_clock_dev, config->step_clk_pwm_pin, period_ms, (period_ms * 1), config->step_clk_pwm_flags);
	ret = pwm_set_dt(&config->pwm, PWM_MSEC(period_ms), PWM_MSEC((period_ms * 1)));
	if (ret != 0) {
		LOG_ERR("Error setting pwm period (%d) with duty cycle (%d)", period_ms, (period_ms * 1));
		return ret;
	}
#endif

	/* Set Step Mode 4 */
	ret = stspin220_direction_set(dev, step_mode4);
	return ret;
}

static int stspin220_fault_get(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
	int ret = -1;

//	const struct device *fault_gpio_dev = device_get_binding(config->fault_gpio_port);
//	if (fault_gpio_dev == NULL) {
//		LOG_ERR("Could not get fault gpio device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->fault_gpio.port)) {
		ret = gpio_pin_configure(config->fault_gpio.port, config->fault_gpio.pin,
				(config->fault_gpio.dt_flags | GPIO_INPUT));
		ret |= gpio_pin_interrupt_configure(config->fault_gpio.port,
				config->fault_gpio.pin, GPIO_INT_EDGE_FALLING);
		if (ret != 0) {
			LOG_ERR("Failed to configure fault gpio pin %d (%d)",
					config->fault_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_get(config->fault_gpio.port, config->fault_gpio.pin);
	} else {
		LOG_ERR("Could not get fault gpio device");
		return -ENODEV;
	}

	return ret;
}

static int stspin220_enable(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
//	struct stspin220_data *data = dev->data;
	int ret = -1;

	/* Enable the device */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev == NULL) {
//		LOG_ERR("Could not get enable device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->en_gpio.port)) {
		ret = gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin,
				(config->en_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->en_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Error setting enable GPIO pin (%d)", config->en_gpio.pin);
			return ret;
		}
	} else {
		LOG_ERR("Could not get enable device");
		return -ENODEV;
	}

	/* Bring out of standby mode */
	ret = stspin220_resume(dev);
//	const struct device *standby_gpio_dev = device_get_binding(config->standby_gpio_port);
//	if (standby_gpio_dev == NULL) {
//		LOG_ERR("Could not get standby device");
//		return -ENODEV;
//	}
//	ret = gpio_pin_configure(standby_gpio_dev, config->standby_gpio_pin, (config->standby_gpio_flags | GPIO_OUTPUT));
//	if (ret != 0) {
//		LOG_ERR("Failed to configure standby pin %d (%d)", config->standby_gpio_pin, ret);
//		return ret;
//	}
//	ret = gpio_pin_set(standby_gpio_dev, config->standby_gpio_pin, 1);
//	if (ret != 0) {
//		LOG_ERR("Error setting standby GPIO (%s)", config->standby_gpio_port);
//		return ret;
//	}

	return ret;
}

static int stspin220_disable(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
//	struct stspin220_data *data = dev->data;
	int ret = -1;

	/* Disable the device */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev == NULL) {
//		LOG_ERR("Could not get enable device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->en_gpio.port)) {
		ret = gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin,
				(config->en_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->en_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Error setting enable GPIO pin (%d)", config->en_gpio.pin);
			return ret;
		}
	} else {
		LOG_ERR("Could not get enable device");
		return -ENODEV;
	}

	/* Go into standby mode */
	ret = stspin220_standby(dev);
//	const struct device *standby_gpio_dev = device_get_binding(config->standby_gpio_port);
//	if (standby_gpio_dev == NULL) {
//		LOG_ERR("Could not get standby device");
//		return -ENODEV;
//	}
//	ret = gpio_pin_configure(standby_gpio_dev, config->standby_gpio_pin, (config->standby_gpio_flags | GPIO_OUTPUT));
//	if (ret != 0) {
//		LOG_ERR("Failed to configure standby pin %d (%d)", config->standby_gpio_pin, ret);
//		return ret;
//	}
//	ret = gpio_pin_set(standby_gpio_dev, config->standby_gpio_pin, 0);
//	if (ret != 0) {
//		LOG_ERR("Error setting standby GPIO (%s)", config->standby_gpio_port);
//		return ret;
//	}

	return ret;
}

static int stspin220_standby(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
//	struct stspin220_data *data = dev->data;
	int ret = -1;

	/* Go into standby mode */
//	const struct device *standby_gpio_dev = device_get_binding(config->standby_gpio_port);
//	if (standby_gpio_dev == NULL) {
//		LOG_ERR("Could not get standby device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->standby_gpio.port)) {
		ret = gpio_pin_configure(config->standby_gpio.port, config->standby_gpio.pin,
				(config->standby_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure standby pin %d (%d)",
					config->standby_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->standby_gpio.port, config->standby_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Error setting standby GPIO pin (%d)",
					config->standby_gpio.pin);
			return ret;
		}
	} else {
		LOG_ERR("Could not get standby device");
		ret = -ENODEV;
	}

	return ret;
}

static int stspin220_resume(const struct device *dev) {
	const struct stspin220_config *config = dev->config;
//	struct stspin220_data *data = dev->data;
	int ret = -1;
	/* Bring out of standby mode */
//	const struct device *standby_gpio_dev = device_get_binding(config->standby_gpio_port);
//	if (standby_gpio_dev == NULL) {
//		LOG_ERR("Could not get standby device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->standby_gpio.port)) {
		ret = gpio_pin_configure(config->standby_gpio.port, config->standby_gpio.pin,
				(config->standby_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure standby pin %d (%d)",
					config->standby_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->standby_gpio.port, config->standby_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Error setting standby GPIO pin (%d)",
					config->standby_gpio.pin);
			return ret;
		}
	} else {
		LOG_ERR("Could not get standby device");
		ret = -ENODEV;
	}

	return ret;
}

static int stspin220_start(const struct device *dev) {
//	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;
	int ret = 0;

	if (data->num_steps <= 0) {
		LOG_ERR("Number of steps not set, cannot start motor");
		return -EINVAL;
	}
	if (data->run_stat) {
		LOG_ERR("Motor already running, stop then start");
		return -EBUSY;
	}

	/* Start the step clock */
	ret = stspin220_step_clock_start(dev);
	if (ret == 0) {
		data->run_stat = true;
	}

	return ret;
}

static int stspin220_stop(const struct device *dev) {
//	const struct stspin220_config *config = dev->config;
	struct stspin220_data *data = dev->data;
	int ret = -1;

	/* Stop the step clock pwm */
	ret = stspin220_step_clock_stop(dev);
	if (ret == 0) {
		data->run_stat = false;
	}

	return ret;
}

static int stspin220_status_get(const struct device *dev) {
	struct stspin220_data *data = dev->data;
	if (data->run_stat) {
		return STSPIN220_STEPPER_RUNNING;
	} else {
		return STSPIN220_STEPPER_NOT_RUNNING;
	}
}

static int stspin220_stop_handler_set(const struct device *dev, stspin220_stop_handler_t handler) {
	struct stspin220_data *data = dev->data;
	int ret = 0;
	if (handler == NULL) {
		return -EINVAL;
	}
	data->stop_handler = handler;

	return ret;
}

static const struct stspin220_driver_api stspin220_drv_api_funcs = {
	.dir_set = stspin220_direction_set,
	.step_clock_set = stspin220_step_clock_set,
	.num_step_set = stspin220_num_step_set,
	.vref_pwm_set = stspin220_vref_pwm_set,
	.step_mode_set = stspin220_step_mode_set,
	.fault_get = stspin220_fault_get,
	.enable = stspin220_enable,
	.disable = stspin220_disable,
	.standby = stspin220_standby,
	.resume = stspin220_resume,
	.start = stspin220_start,
	.stop = stspin220_stop,
	.status_get = stspin220_status_get,
	.stop_handler_set = stspin220_stop_handler_set,
};



#define DEVICE_INSTANCE(inst)													\
																				\
const static struct stspin220_config stspin220_##inst##_cfg = {					\
	/*.step_clk_dev_name = DT_PWMS_LABEL_BY_NAME(DT_DRV_INST(inst),stepclk),		\
	.step_clk_pwm_pin = DT_INST_PWMS_CHANNEL_BY_NAME(inst,stepclk),				\
	.step_clk_pwm_period_ns = DT_INST_PWMS_PERIOD_BY_NAME(inst,stepclk),		\
	.step_clk_pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst,stepclk),				\
	.step_clk_pwm = PWM_DT_SPEC_INST_GET_BY_NAME(inst, stepclk),				*/	\
	\
	/*.vref_dev_name = DT_PWMS_LABEL_BY_NAME(DT_DRV_INST(inst),vref),				\
	.vref_pwm_pin = DT_INST_PWMS_CHANNEL_BY_NAME(inst,vref),					\
	.vref_pwm_period_ns = DT_INST_PWMS_PERIOD_BY_NAME(inst,vref),				\
	.vref_pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst,vref),					*/\
	.vref_pwm = PWM_DT_SPEC_INST_GET_BY_NAME(inst, vref),					\
																				\
	/*.step_clk_port = DT_INST_GPIO_LABEL(inst, step_clock_gpios),				\
	.step_clk_pin = DT_INST_GPIO_PIN(inst, step_clock_gpios),					\
	.step_clk_flags = DT_INST_GPIO_FLAGS(inst, step_clock_gpios),				*/\
	.step_clk_gpio = GPIO_DT_SPEC_INST_GET(inst, step_clock_gpios), \
																				\
	/*.step_mode1_gpio_port = DT_INST_GPIO_LABEL(inst, step_mode1_gpios),			\
	.step_mode1_gpio_pin = DT_INST_GPIO_PIN(inst, step_mode1_gpios),			\
	.step_mode1_gpio_flags = DT_INST_GPIO_FLAGS(inst, step_mode1_gpios),		*/\
	.step_mode1_gpio = GPIO_DT_SPEC_INST_GET(inst, step_mode1_gpios), \
																				\
	/*.step_mode2_gpio_port = DT_INST_GPIO_LABEL(inst, step_mode2_gpios),			\
	.step_mode2_gpio_pin = DT_INST_GPIO_PIN(inst, step_mode2_gpios),			\
	.step_mode2_gpio_flags = DT_INST_GPIO_FLAGS(inst, step_mode2_gpios),		*/\
	.step_mode2_gpio = GPIO_DT_SPEC_INST_GET(inst, step_mode2_gpios), \
																				\
	/*.dir_gpio_port = DT_INST_GPIO_LABEL(inst, direction_gpios),					\
	.dir_gpio_pin = DT_INST_GPIO_PIN(inst, direction_gpios),					\
	.dir_gpio_flags = DT_INST_GPIO_FLAGS(inst, direction_gpios),				*/\
	.dir_gpio = GPIO_DT_SPEC_INST_GET(inst, direction_gpios), \
																				\
	/*.en_gpio_port = DT_INST_GPIO_LABEL(inst, enable_gpios),						\
	.en_gpio_pin = DT_INST_GPIO_PIN(inst, enable_gpios),						\
	.en_gpio_flags = DT_INST_GPIO_FLAGS(inst, enable_gpios),					*/\
	.en_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios), \
																				\
	/*.standby_gpio_port = DT_INST_GPIO_LABEL(inst, standby_gpios),				\
	.standby_gpio_pin = DT_INST_GPIO_PIN(inst, standby_gpios),					\
	.standby_gpio_flags = DT_INST_GPIO_FLAGS(inst, standby_gpios),				*/\
	.standby_gpio = GPIO_DT_SPEC_INST_GET(inst, standby_gpios), \
																				\
	/*.fault_gpio_port = DT_INST_GPIO_LABEL(inst, fault_gpios),					\
	.fault_gpio_pin = DT_INST_GPIO_PIN(inst, fault_gpios),						\
	.fault_gpio_flags = DT_INST_GPIO_FLAGS(inst, fault_gpios),					*/\
	.fault_gpio = GPIO_DT_SPEC_INST_GET(inst, fault_gpios), \
																				\
	.vref_pwm_duty = DT_PROP(DT_DRV_INST(inst),vref_pwm_duty),					\
																				\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, vref_gpios), (	\
			.vref_gpio = GPIO_DT_SPEC_INST_GET(inst, vref_gpios), \
	))\
};																				\
																				\
static struct stspin220_data stspin220_##inst##_drvdata;						\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		stspin220_init,															\
		device_pm_control_nop,													\
		&stspin220_##inst##_drvdata,											\
		&stspin220_##inst##_cfg,												\
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,							\
		&stspin220_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);

