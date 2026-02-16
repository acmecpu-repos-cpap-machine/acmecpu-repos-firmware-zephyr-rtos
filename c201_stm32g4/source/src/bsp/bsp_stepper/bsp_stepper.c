/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_stepper);

#include "acpu_c201_modules.h"
#include "app_settings/app_settings.h"
#include "stspin220.h"
#include "app_stepper/bsp_stepper.h"

static uint32_t m_curr_pos = 0;
static uint8_t m_step_mode = 1;
static uint16_t m_full_step_angle = 18;
static bool m_init_stat = false;

/*
 * This function convers the input step_mode to a step_mode_value which can be
 * sent to the underlying driver. It also calculates the number of steps required
 * for travel_deg input based on the step_angle */
static uint8_t convert_value_from_step_mode(int step_mode, float step_angle,
		float travel_deg, float *pstep_full_rotate) {

	uint8_t step_mode_value = STSPIN220_STEP_MODE_STEP_FULL;
	float step_per_rotation = 0;

	switch (step_mode) {
	case STEP_MODE_FULL:
		step_mode_value = STSPIN220_STEP_MODE_STEP_FULL;
		step_per_rotation = (float) travel_deg / (step_angle / 1);
		break;
	case STEP_MODE_HALF:
		step_mode_value = STSPIN220_STEP_MODE_STEP_2;
		step_per_rotation = (float) travel_deg / (step_angle / 2);
		break;
	case STEP_MODE_1_4TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_4;
		step_per_rotation = (float) travel_deg / (step_angle / 4);
		break;
	case STEP_MODE_1_8TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_8;
		step_per_rotation = (float) travel_deg / (step_angle / 8);
		break;
	case STEP_MODE_1_16TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_16;
		step_per_rotation = (float) travel_deg / (step_angle / 16);
		break;
	case STEP_MODE_1_32TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_32;
		step_per_rotation = (float) travel_deg / (step_angle / 32);
		break;
	case STEP_MODE_1_64TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_64;
		step_per_rotation = (float) travel_deg / (step_angle / 64);
		break;
	case STEP_MODE_1_128TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_128;
		step_per_rotation = (float) travel_deg / (step_angle / 128);
		break;
	case STEP_MODE_1_256TH:
		step_mode_value = STSPIN220_STEP_MODE_STEP_256;
		step_per_rotation = (float) travel_deg / (step_angle / 256);
		break;
	}

	if (pstep_full_rotate != NULL)
		*pstep_full_rotate = step_per_rotation;

	return step_mode_value;
}

static int bsp_stepper_spin_motor(uint32_t step_clk_us, uint8_t step_mode_val, uint8_t dir, uint32_t total_num_steps) {
	/* get the device driver instance */
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_STEPPER);
	if (dev == NULL) {
		LOG_ERR("Could not find dev device %s", ACPU_C201_MOD_NAME_STEPPER);
		return -ENODEV;
	}
	struct stspin220_driver_api *api = (struct stspin220_driver_api*) dev->api;
	int ret=0;
	/* setup driver */
//	api->vref_pwm_set(dev, 200, 88); // TODO move to driver and delete this api
	ret = api->step_clock_set(dev, step_clk_us);
//	api->step_mode_set(dev, step_mode_val);
//	api->enable(dev);
	ret |= api->dir_set(dev, dir);
	ret |= api->num_step_set(dev, total_num_steps);

	if (!ret) {
		/* spin the motor */
		api->start(dev);
	} else {
		return -1;
	}

	while (api->status_get(dev) == STSPIN220_STEPPER_RUNNING) {
		k_sleep(K_MSEC(10));
	}

//	api->disable(dev);

	return 0;
}

int bsp_stepper_goto_reset_pos(uint16_t pos, uint32_t speed_hz, uint32_t num_rotation, uint8_t dir) {
	if (!m_init_stat) {
		return -ENODEV;
	}

	int ret = 0;

	/* calculation */

	/* calculate the number of steps in a 360deg rotation
	 * steps_full_rotate = {360 / (full_step_angle / step_mode)}
	 * if,
	 * 		full_step_angle = 18
	 * 		step_mode = 1/4
	 * 		steps_full_rotate = 360 / (18/4) = 80
	 * */
	float total_num_steps = 1;
	uint8_t step_mode_val = convert_value_from_step_mode(m_step_mode, m_full_step_angle, 360, &total_num_steps);

	/* calculate total number of steps required from number of rotations */
	if (num_rotation == 0) {
		total_num_steps = 0;
		return 0;
	}
	else {
		total_num_steps *= num_rotation;
	}

	/* calculate the clock period in micro-seconds */
	uint32_t step_clk_us = (1 * 1000000) / speed_hz;

	ret = bsp_stepper_spin_motor(step_clk_us, step_mode_val, dir, total_num_steps);

	/* After reset the motor position should be 0 degrees */
	if (!ret) {
		m_curr_pos = 0;
	} else {
		LOG_ERR("bsp_stepper_spin_motor failed");
	}

	LOG_DBG("m_curr_pos = %d", m_curr_pos);
	return ret;
}

int bsp_stepper_goto_pos_abs(uint16_t pos, uint32_t speed_hz, uint32_t num_rotation) {
	if (!m_init_stat) {
		return -ENODEV;
	}

	/* convert step mode value */
	float total_num_steps = 0;
	uint8_t step_mode_val = convert_value_from_step_mode(m_step_mode, m_full_step_angle, 360, &total_num_steps);

	/* calculate num steps for required rotations */
	if (num_rotation == 0)
		total_num_steps = 0;
	else
		total_num_steps *= num_rotation;

	/* find the direction of travel */
	uint8_t dir;
	int pos_delta = pos - m_curr_pos;
	if (pos_delta > 0)
		dir = STSPIN220_DIR_ANTICLOCKWISE;
	else
		dir = STSPIN220_DIR_CLOCKWISE;

	pos_delta = abs(pos_delta);

	/* calculate number of steps for delta position */
	float pos_delta_steps = 0;
	convert_value_from_step_mode(m_step_mode, m_full_step_angle, pos_delta, &pos_delta_steps);

	/* evaluate total number of steps */
	total_num_steps += pos_delta_steps;

	/* calculate the clock period in micro-seconds */
	uint32_t step_clk_us = (1 * 1000000) / speed_hz;

	int ret = bsp_stepper_spin_motor(step_clk_us, step_mode_val, dir, total_num_steps);
	if (!ret) {
		m_curr_pos = pos;
		if (m_curr_pos == 360) {
			m_curr_pos = 0;
		}
	} else {
		LOG_ERR("bsp_stepper_spin_motor failed");
	}
	LOG_DBG("m_curr_pos = %d", m_curr_pos);

	return ret;
}

int bsp_stepper_goto_pos_rel(uint16_t travel_deg, uint32_t speed_hz, uint8_t dir) {
	if (!m_init_stat) {
		return -ENODEV;
	}

	if ((travel_deg < 0) || (travel_deg > 360)) {
		return -EINVAL;
	}

	/* convert step mode value and calculate total number of steps */
	float total_num_steps = 0;
	uint8_t step_mode_val = convert_value_from_step_mode(m_step_mode, m_full_step_angle, travel_deg, &total_num_steps);

	/* calculate the clock period in micro-seconds */
	uint32_t step_clk_us = (1 * 1000000) / speed_hz;

	int t_pos = m_curr_pos;
	int ret = bsp_stepper_spin_motor(step_clk_us, step_mode_val, dir, total_num_steps);
	if (!ret) {
		if (dir == STSPIN220_DIR_ANTICLOCKWISE) {
			t_pos += travel_deg;
			if (t_pos > 360)
				m_curr_pos = (t_pos - 360);
			else
				m_curr_pos = t_pos;
		} else if (dir == STSPIN220_DIR_CLOCKWISE) {
			t_pos -= travel_deg;
			if (t_pos < 0)
				m_curr_pos = (t_pos + 360);
			else
				m_curr_pos = t_pos;
		}
	} else {
		LOG_ERR("bsp_stepper_spin_motor failed");
	}

	LOG_DBG("m_curr_pos = %d", m_curr_pos);

	return ret;
}

uint16_t bsp_stepper_current_pos_get() {
	return m_curr_pos;
}

int bsp_stepper_zero_set() {
	m_curr_pos = 0;
	return 0;
}

int bsp_stepper_init(uint8_t step_mode, uint16_t full_step_angle) {
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_STEPPER);
	if (dev == NULL) {
		LOG_ERR("Could not find dev device %s", ACPU_C201_MOD_NAME_STEPPER);
		return -ENODEV;
	}
	struct stspin220_driver_api *api = (struct stspin220_driver_api*) dev->api;

	uint8_t step_mode_val = convert_value_from_step_mode(step_mode, 18 /*dummy*/, 360 /*dummy*/, NULL);

	/* setup driver */
	int ret = 0;
//	int ret = api->vref_pwm_set(dev, 200, 88); // TODO move to driver and delete this api
	ret |= api->step_mode_set(dev, step_mode_val);
	ret |= api->enable(dev);

	if (!ret) {
		m_step_mode = step_mode;
		m_full_step_angle = full_step_angle;
		m_init_stat = true;
	} else {
		LOG_ERR("bsp_stepper_init failed!");
	}

	return ret;
}

int bsp_stepper_deinit() {
	if (!m_init_stat) {
		return -ENODEV;
	}

	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_STEPPER);
	if (dev == NULL) {
		LOG_ERR("Could not find dev device %s", ACPU_C201_MOD_NAME_STEPPER);
		return -ENODEV;
	}
	struct stspin220_driver_api *api = (struct stspin220_driver_api*) dev->api;
	int ret = api->disable(dev);
	if (!ret)
		m_init_stat = false;

	return ret;
}

int bsp_stepper_standby() {
	if (!m_init_stat) {
		return -ENODEV;
	}

	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_STEPPER);
	if (dev == NULL) {
		LOG_ERR("Could not find dev device %s", ACPU_C201_MOD_NAME_STEPPER);
		return -ENODEV;
	}
	struct stspin220_driver_api *api = (struct stspin220_driver_api*) dev->api;
	int ret = api->standby(dev);
	return ret;
}

int bsp_stepper_resume() {
	if (!m_init_stat) {
		return -ENODEV;
	}

	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_STEPPER);
	if (dev == NULL) {
		LOG_ERR("Could not find dev device %s", ACPU_C201_MOD_NAME_STEPPER);
		return -ENODEV;
	}
	struct stspin220_driver_api *api = (struct stspin220_driver_api*) dev->api;
	uint8_t step_mode_val = convert_value_from_step_mode(m_step_mode, 18 /*dummy*/, 360 /*dummy*/, NULL);

	/* setup driver */
	int ret = api->vref_pwm_set(dev, 200, 88); // TODO move to driver and delete this api
	ret |= api->step_mode_set(dev, step_mode_val);
	ret |= api->resume(dev);

	return ret;
}

