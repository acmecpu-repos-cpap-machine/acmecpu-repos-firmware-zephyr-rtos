/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 1-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SPINBOX_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SPINBOX_H_

#include <stdint.h>
#include "app_display/app_display.h"

int c20x_screen_spinbox_init(	const char *display_name,
								int range_min,
								int range_max,
								int digit_count,
								int separator_position,
								int present_val,
								const char *settings_save_path,
								app_display_key_cb prev_screen_cb
		);

void c20x_screen_spinbox_styles_init();


#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SPINBOX_H_ */
