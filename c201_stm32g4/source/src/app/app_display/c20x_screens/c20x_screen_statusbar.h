/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 24-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_STATUSBAR_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_STATUSBAR_H_

#include <stdint.h>

void c20x_screen_statusbar_reload();

int c20x_screen_statusbar_init(uint32_t x_width, uint32_t y_height);

void c20x_screen_statusbar_deinit();

void c20x_screen_statusbar_init_onetime();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_STATUSBAR_H_ */
