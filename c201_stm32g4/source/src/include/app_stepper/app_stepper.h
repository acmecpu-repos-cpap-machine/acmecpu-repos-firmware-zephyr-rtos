/*
 * Copyright (c) 2021 Acme CPU
 */


#ifndef SRC_INCLUDE_APP_STEPPER_APP_STEPPER_H_
#define SRC_INCLUDE_APP_STEPPER_APP_STEPPER_H_

#include <stdint.h>

struct stepper_params {
	uint32_t step_speed_hz;
	uint32_t num_rot;
	uint16_t rst_pos;
	uint16_t step_angle;
	uint8_t dir;
	uint8_t step_mode;
};


int app_stepper_init();

int app_stepper_direction_set(uint8_t dir);
int app_stepper_speed_hz_set(uint32_t speed_hz);
int app_stepper_num_rot_set(uint32_t num_rot);
int app_stepper_pos_rel_set(uint16_t pos_rel);
int app_stepper_pos_abs_set(uint16_t pos_abs);
int app_stepper_pos_curr_get(uint16_t *pos_curr);
int app_stepper_params_get(struct stepper_params *params);
int app_stepper_zero_set();

#endif /* SRC_INCLUDE_APP_STEPPER_APP_STEPPER_H_ */
