/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_BSP_DISPLAY_BSP_DISPLAY_H_
#define SRC_INCLUDE_BSP_DISPLAY_BSP_DISPLAY_H_

#include <stdint.h>

/**
 * @brief: 	Initializes a display
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_display_init();

/**
 * @brief: 	Turns on the display and sets the brightness to the last value
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_display_on();

/**
 * @brief: 	Turns off the display
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_display_off();

/**
 * @brief: 	Sets the brightness of a display as a percentage
 *
 * @param percent	0 to 100
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_display_set_brightness(uint8_t percent);

#endif /* SRC_INCLUDE_BSP_DISPLAY_BSP_DISPLAY_H_ */
