/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_H_
#define SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_H_

#include <stdint.h>
#include <lvgl.h>

enum {
	APP_DISPLAY_KEY_POWER = 101,
	APP_DISPLAY_KEY_HOME,
	APP_DISPLAY_KEY_MIC,
	APP_DISPLAY_KEY_VOL_UP,
	APP_DISPLAY_KEY_VOL_DOWN,
	APP_DISPLAY_KEY_MUTE,
};

/**
 * @brief: 	Function pointer prototype for handling key presses
 * */
typedef void (*app_display_key_cb)(uint32_t key);

/**
 * @brief: 	This function is used to get the lv_group_t instance used for the application
 *
 * @return	lv_group_t*	lv_group_t pointer
 *
 * */
lv_group_t * app_display_lvgl_group_instance_get();

/**
 * @brief: 	This function is used to add the current lvgl object to the lvgl group used
 * 			for controlling the screen with keypad / buttons
 *
 * @param	obj		LVGL object. Can be list, switch, slider etc. Must not be NULL
 *
 * */
void app_display_lvgl_group_set_current(lv_obj_t * obj);

/**
 * @brief: 	Function used to set the callback to handle key presses. Generally most of the
 * 			key presses should get handled by LVGL groups. The ones that are not handled
 * 			or requires custom control, should be handled by this callback, for e.g. the back or home button
 *
 * */
void app_display_key_cb_set(app_display_key_cb key_cb);

/**
 * @brief: 	Function used to set the callback to handle special key presses.
 * 			Special keys are keys which have an action on every screen.
 * 			Examples of special keys are power, home, volume, mic etc.
 * */
void app_display_spl_key_cb_set(app_display_key_cb spl_key_cb);

/**
 * @brief: 	Function used to return the systems display device. This could be useful if
 * 			other files/module wants to access the display directly (without using LVGL etc.)
 * 			to do some operation
 *
 * @return:	struct device * object 	Success
 * 			NULL					Failure
 * */
const struct device* app_display_device_get(void);

/**
 * @brief: 	Initializes a display, sets the brightness as per settings, starts refresh threads
 * 			and displays the first screen
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int app_display_init();

#endif /* SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_H_ */
