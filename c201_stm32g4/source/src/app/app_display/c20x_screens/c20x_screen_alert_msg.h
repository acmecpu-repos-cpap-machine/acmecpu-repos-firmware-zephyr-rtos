/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 27-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_ALERT_MSG_H_
#define SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_ALERT_MSG_H_

#include <lvgl.h>
#include "c20x_screen_manager.h"

typedef void (*alert_msg_caller_cb)(const char*, void*);

/**
 * @brief	creates a LVGL message box
 *
 * @param
 * 		parent			msg box parent
 * 		btn_count		number of buttons on the btnmatrix
 * 		btns			button names
 * 		title			msg box title
 * 		text			msg box text
 * 		timeout_sec		number of seconds to display the message for, if 0 the stays forever, until a button is pressed
 * 		back_screen		callback to go back to previous screen
 * 		user_cb			callback to be called on press of a msgbox button
 */
int c20x_screen_alert_msg_show(
								lv_obj_t *parent,
								uint8_t btn_count,
								const char *btns[],
								const char *title,
								const char *text,
								bool close_btn,
								uint32_t timeout_sec,
								uint8_t prev_screen_id,
								alert_msg_caller_cb user_cb,
								void *user_data
								);
void c20x_screen_alert_msg_cb_set(c20x_screen_change_cb screen_change_cb);
void c20x_screen_alert_msg_styles_init();

#endif /* SRC_APP_APP_DISPLAY_C20X_SCREENS_C20X_SCREEN_ALERT_MSG_H_ */
