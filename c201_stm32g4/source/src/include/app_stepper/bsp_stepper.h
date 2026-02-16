/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_STEPPER_BSP_STEPPER_H_
#define SRC_INCLUDE_APP_STEPPER_BSP_STEPPER_H_

#include <stdint.h>

#define STEP_MODE_FULL		(1)
#define STEP_MODE_HALF		(2)
#define STEP_MODE_1_4TH		(4)
#define STEP_MODE_1_8TH		(8)
#define STEP_MODE_1_16TH	(16)
#define STEP_MODE_1_32TH	(32)
#define STEP_MODE_1_64TH	(64)
#define STEP_MODE_1_128TH	(128)
#define STEP_MODE_1_256TH	(256)

#define BSP_STEPPER_DIR_CLOCKWISE				0
#define BSP_STEPPER_DIR_ANTICLOCKWISE			1


/**
 * @brief: 	Initializes the stepper motor bsp with step mode and motor's full step angle
 *
 * @param:	step_mode			the desired step mode (full step / half step / 1/4th step ...)
 * 			full_step_angle 	full step angle of the motor used in the application in degrees
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_init(uint8_t step_mode, uint16_t full_step_angle);

/**
 * @brief: 	De-initializes the stepper motor bsp. bsp_stepper_init() must be called before calling
 * 			any other functions. If the step_mode or full_step_angle needs to be changed, then
 * 			call this function and then call bsp_stepper_init() with new values
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_deinit();

/**
 * @brief: 	Put the stepper motor driver into standby mode for power saving
 * 			bsp_stepper_resume() must be called before calling other functions
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_standby();

/**
 * @brief: 	Resume the stepper motor driver from standby mode
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_resume();

/**
 * @brief: 	Function to go to a reset position, this can be used after power up so that the
 * 			stepper motor goes to an initial reset position
 *
 * @param:	pos				the reset position in degrees (0 to 360)
 * 			speed_hz 		speed of rotation in hertz (this will be converted into step clock)
 * 			num_rotation	number of full (360 degree) rotations. Can be 0
 * 			dir				direction of rotation (0 - clockwise, 1 - anti clockwise)
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_goto_reset_pos(uint16_t pos, uint32_t speed_hz, uint32_t num_rotation, uint8_t dir);

/**
 * @brief: 	Function to go to an absolute position.
 * 			The direction of travel will be automatically calculated.
 *
 * @param:	pos				the absolute position in degrees (0 to 360)
 * 			speed_hz 		speed of rotation in hertz (this will be converted into step clock)
 * 			num_rotation	number of full (360 degree) rotations. Can be 0
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_goto_pos_abs(uint16_t pos, uint32_t speed_hz, uint32_t num_rotation);

/**
 * @brief: 	Function to go to a position relative to the current position
 *
 * @param:	travel_deg		The number of degrees to be traveled (0 to 360)
 * 			speed_hz 		speed of rotation in hertz (this will be converted into step clock)
 * 			dir				direction of rotation (0 - clockwise, 1 - anti clockwise)
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_stepper_goto_pos_rel(uint16_t travel_deg, uint32_t speed_hz, uint8_t dir);

/**
 * @brief: 	Function to get the current position of the stepper motor
 * 			Calling bsp_stepper_goto_reset_pos() sets the current position to 0
 *
 * @return:	current position (0 to 359)
 * */
uint16_t bsp_stepper_current_pos_get();

/**
 * @brief: 	Function to set the current position of the stepper motor as the absolute 0
 * 			Used for position error correction
 *
 * @return:	0 SUCCESS
 * */
int bsp_stepper_zero_set();

#endif /* SRC_INCLUDE_APP_STEPPER_BSP_STEPPER_H_ */
