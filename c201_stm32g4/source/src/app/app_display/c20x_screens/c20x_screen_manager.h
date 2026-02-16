/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 30-Nov-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_MANAGER_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_MANAGER_H_

#define C20X_SCREEN_MGR_THREAD_SLEEP_MSEC	100
#define C20X_SCREEN_MGR_SBAR_RELOAD_SEC		30
#define C20X_SCREEN_MGR_DB_RELOAD_MSEC		500

typedef enum {
	C20X_SCREEN_NONE=0,

	C20X_SCREEN_SPLASH,

	C20X_SCREEN_DASHBOARD,
	C20X_SCREEN_SETTINGS,
	C20X_SCREEN_DATETIME,
	C20X_SCREEN_MENU,
	C20x_SCREEN_RESULTS,
	C20X_SCREEN_ALERT_MSG,
	C20X_SCREEN_POWER_OFF,

	C20X_TFT_TEST,

	C20X_SCREEN_MAX
} C20X_SCREENS;

/**
 * @brief: 	Function pointer prototype for handling screen to screen transition
 * */
typedef void (*c20x_screen_change_cb)(uint8_t screen_id);

C20X_SCREENS c20x_screen_manager_curr_screen_get();
void c20x_screen_manager_init(lv_group_t * lvgl_grp);

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_MANAGER_H_ */
