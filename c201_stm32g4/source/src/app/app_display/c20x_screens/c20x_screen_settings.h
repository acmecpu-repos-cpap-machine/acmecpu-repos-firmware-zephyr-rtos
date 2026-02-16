/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 04-Nov-2021
 *      Author: Rohan Dey
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_H_

#include <lvgl.h>
#include "c20x_screen_config.h"
#include "c20x_screen_manager.h"

void c20x_screen_settings_cb_set(c20x_screen_change_cb screen_change_cb);

void c20x_screen_settings_text_set(const char* text);

int c20x_screen_settings_init(uint32_t x_width, uint32_t y_height, uint32_t label_height, lv_group_t * lvgl_grp);

void c20x_screen_settings_deinit();

//int c20x_screen_settings_obj_get(lv_obj_t **pobj);

//int c20x_screen_setings_key_cb_set(c20x_screen_settings_key_cb *key_cb);

void c20x_screen_settings_start();

void c20x_screen_settings_styles_init();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_SETTINGS_H_ */
