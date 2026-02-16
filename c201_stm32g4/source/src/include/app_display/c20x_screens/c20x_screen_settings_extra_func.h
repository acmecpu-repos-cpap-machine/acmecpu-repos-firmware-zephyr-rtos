/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 28-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_EXTRA_FUNC_H_
#define SRC_INCLUDE_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_EXTRA_FUNC_H_

#include <lvgl.h>
#include "app_display/app_display.h"

/******************************************************************************
 * WIFI HOTSPOT
 ******************************************************************************/
int settings_hotspot_extra_func(
							const char *display_name,
							const char *setting_path,
							app_display_key_cb prev_screen_cb
							);



/******************************************************************************
 * Extra functions common
 ******************************************************************************/
void c20x_screen_settings_extra_func_init();

#endif /* SRC_INCLUDE_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_EXTRA_FUNC_H_ */
