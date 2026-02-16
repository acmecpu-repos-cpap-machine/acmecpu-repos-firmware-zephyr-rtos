/*
 * Copyright (c) 2021 Acme CPU
 * c20x_screen_switch.c
 *
 *  Created on: 24-Nov-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SWITCH_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SWITCH_H_

#include <stdint.h>
#include "app_display/app_display.h"

int c20x_screen_switch_init(const char *display_name, bool present_val,
		const char *settings_save_path, uint8_t datatype,
		app_display_key_cb prev_screen_cb);

void c20x_screen_switch_styles_init();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SWITCH_H_ */
