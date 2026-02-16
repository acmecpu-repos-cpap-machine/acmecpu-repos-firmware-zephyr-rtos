/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 24-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DASHBOARD_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DASHBOARD_H_

#include <lvgl.h>
#include <stdint.h>
#include <time.h>
#include "c20x_screen_config.h"
#include "c20x_screen_manager.h"
#include "app_errors.h"

typedef enum {
	DASH_LABEL_IDX_USER = 0,
	DASH_LABEL_IDX_STATE,
	DASH_LABEL_IDX_MODE,
	DASH_LABEL_IDX_PRESSURE,
	DASH_LABEL_IDX_RUNTIME,
//	DASH_LABEL_IDX_ERROR,
	DASH_LABEL_IDX_MAX,
} DASH_LABEL_IDX;

int c20x_screen_dashboard_pressure_label_set(float pressure);
int c20x_screen_dashboard_runtime_label_set(struct tm *time);
//int c20x_screen_dashboard_error_label_set(APP_ERRORS err_code);

void c20x_screen_db_cb_set(c20x_screen_change_cb screen_change_cb);

int c20x_screen_dashboard_init(uint16_t x_width, uint16_t y_height, uint16_t sbar_height, lv_group_t * lvgl_grp);

int c20x_screen_dashboard_deinit();

void c20x_screen_dashboard_reload();

void c20x_screen_dashboard_styles_init();

void c20x_screen_dashboard_init_onetime();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_DASHBOARD_H_ */
