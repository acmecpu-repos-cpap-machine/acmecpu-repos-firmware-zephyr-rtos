/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 13-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_POWER_KEY_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_POWER_KEY_H_

#include <stdint.h>
#include "c20x_screen_manager.h"

int c20x_screen_power_key_detect(uint32_t key);
int c20x_screen_power_key_init();

void c20x_screen_poweroff_cb_set(c20x_screen_change_cb screen_change_cb);
int c20x_screen_poweroff_load(C20X_SCREENS prev_screen_id, lv_group_t * lvgl_grp);
int c20x_screen_poweroff_deinit();
void c20x_screen_poweroff_styles_init();
void c20x_screen_poweroff_init_onetime();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_POWER_KEY_H_ */
