/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DYNAMIC_ROLLER_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DYNAMIC_ROLLER_H_

#include <stdint.h>
//#include "app_settings/app_settings_value.h"
#include "app_display/app_display.h"
#include "app_display/app_display_menu.h"

//struct rollerData {
//	char key[10];
//	uint32_t val;
//};

//struct rollerMetaData {
//	uint8_t num_items;
//	struct app_settings_value *data;
//	int selected_idx;
//	const char *settings_path;
//	uint16_t settings_len;
//};

int c20x_screen_dynamic_roller_init(
							const char *display_name,
							struct gui_param_metadata *mdata,
							uint8_t roller_count,
							uint8_t visible_row_count,
							int16_t range_min_idx, int16_t range_max_idx,
							app_display_key_cb prev_screen_cb,
							menu_extra_func extra_func
						);

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DYNAMIC_ROLLER_H_ */
