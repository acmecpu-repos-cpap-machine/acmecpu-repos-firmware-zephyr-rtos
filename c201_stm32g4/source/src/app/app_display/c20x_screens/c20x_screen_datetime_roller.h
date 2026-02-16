/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 2-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DATETIME_ROLLER_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DATETIME_ROLLER_H_

#include <stdint.h>
#include <time.h>
#include "app_display/app_display.h"


typedef enum {
	C20X_SCREEN_DATE=0,
	C20X_SCREEN_TIME
} C20X_SCREEN_DATETIME_SELECT;

int c20x_screen_datetime_roller_init(	const char *display_name,
										uint8_t screen_date_or_time,
										uint16_t x_width, uint16_t y_height,
										uint16_t top_label_height,
										struct tm *time,
										const char *settings_save_path,
										app_display_key_cb prev_screen_cb

		);


#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DATETIME_ROLLER_H_ */
