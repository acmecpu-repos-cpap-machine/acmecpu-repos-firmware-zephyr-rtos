/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 8-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_HAPTIC_APP_HAPTIC_H_
#define SRC_INCLUDE_APP_HAPTIC_APP_HAPTIC_H_

/**
 * @brief
 *      Enables actuator, e.g. starts BLDC vibration motor
 */
void app_haptic_on();

/**
 * @brief
 *      Disables actuator
 */
void app_haptic_off();

/**
 * @brief
 *      Initialize the haptic device (vibration motor) for feedback to user
 * 		
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_haptic_init();

#endif  /*SRC_INCLUDE_APP_HAPTIC_APP_HAPTIC_H_*/