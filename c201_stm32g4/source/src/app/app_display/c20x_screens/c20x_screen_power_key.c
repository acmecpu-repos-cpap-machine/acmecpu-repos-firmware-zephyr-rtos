/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 13-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "lib_push_switch/lib_push_switch.h"
#include "app_push_switch/app_push_switch.h"
#include "app_blower/app_blower.h"
#include "bsp_buzzer/bsp_buzzer.h"
#include "c20x_screen_alert_msg.h"
#include "c20x_screen_manager.h"

static struct k_work m_key_detection_work;
//static const char * mbox_btn_pwr[] = {"Cancel", "Ok", ""};

/* power off screen variables */
#define NUM_BUTTONS		2
static lv_obj_t *label_pwr_off;
static lv_obj_t *btn_cancel;
static lv_obj_t *btn_ok;
static lv_group_t * m_grp = NULL;
static C20X_SCREENS m_prev_screen_id = C20X_SCREEN_NONE;
static c20x_screen_change_cb m_screen_change = NULL;
static int8_t m_focussed_btn = -1;

static void key_detection_work_handler(struct k_work *work)
{
	int64_t start_time = k_uptime_get();
	int64_t duration = 0;
//	uint32_t ms_to_off=0;
	int detected_type = SWITCH_PRESSED_NORMAL_EDGE;
	int ret=0;
	do {
		int64_t temp = start_time;
		duration = k_uptime_delta(&temp);
		if (duration > LIB_PUSH_SWITCH_EDGE_LONG_PRESS_DURATION_MIN) {
			detected_type = SWITCH_PRESSED_NORMAL_EDGE_LONG;
			break;
		}
		k_sleep(K_MSEC(10));
	} while (app_push_switch_state_get_power_key() == SWITCH_ASSERTED);	// check if the key is pressed

	if (detected_type == SWITCH_PRESSED_NORMAL_EDGE) {
		LOG_INF("POWER KEY NORMAL PRESS");
#if CONFIG_APP_BLOWER
		uint8_t state = app_blower_run_state_get();
		if (state == BLOWER_NOT_RUNNING) {
			int count=0;
			do {
				ret = app_blower_settings_change_state(APP_BLOWER_START);
				if (ret < 0) {
					LOG_WRN("blower_settings_change APP_BLOWER_START failed, %d, try again", ret);
				}
				k_sleep(K_MSEC(10));
			} while ((ret != 0) && (++count < 10));
			int screen = c20x_screen_manager_curr_screen_get();
			LOG_DBG("screen = %d", screen);
			c20x_screen_alert_msg_show(NULL, 0, NULL, "Alert", "Blower ON",
					false, 3, screen, NULL, NULL);
		}
		else if (state == BLOWER_RUNNING) {
			int count=0;
			do {
				ret = app_blower_settings_change_state(APP_BLOWER_STOP);
				if (ret < 0) {
					LOG_WRN("blower_settings_change APP_BLOWER_STOP failed, %d, try again", ret);
				}
				k_sleep(K_MSEC(10));
			} while ((ret != 0) && (++count < 10));
			int screen = c20x_screen_manager_curr_screen_get();
			LOG_DBG("screen = %d", screen);
			c20x_screen_alert_msg_show(NULL, 0, NULL, "Alert", "Blower OFF",
					false, 3, screen, NULL, NULL);
		}
#endif
		bsp_buzzer_play_switch_pressed();
	} else if (detected_type == SWITCH_PRESSED_NORMAL_EDGE_LONG) {
//		int screen = c20x_screen_manager_curr_screen_get();
//		LOG_DBG("screen = %d", screen);
//		c20x_screen_alert_msg_show(NULL, 2, mbox_btn_pwr, "Power Off", "Do you want to power off the device?",
//				false, 0, screen, NULL, NULL);
		bsp_buzzer_on();
		k_sleep(K_MSEC(500));
		bsp_buzzer_off();
//		m_screen_change(C20X_SCREEN_POWER_OFF);
		LOG_INF("POWER KEY LONG PRESS");
	}
}

int c20x_screen_power_key_detect(uint32_t key)
{
	if (key == APP_DISPLAY_KEY_POWER)
		k_work_submit(&m_key_detection_work);
	else
		return -1;
	return 0;
}

/* power off screen functions */
static void btn_handler(lv_event_t *event)
{
	if (m_focussed_btn == (NUM_BUTTONS-1)) {
		LOG_INF("**************** power off device");
		m_screen_change(m_prev_screen_id);
	} else {
		m_screen_change(m_prev_screen_id);
	}
}

static void keypad_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ENTER:
		break;
	case LV_KEY_ESC:
		m_screen_change(m_prev_screen_id);
		break;
	case LV_KEY_LEFT:
		lv_group_focus_next(m_grp);
		if (++m_focussed_btn >= NUM_BUTTONS)
			m_focussed_btn = 0;
		break;
	case LV_KEY_RIGHT:
		lv_group_focus_prev(m_grp);
		if (--m_focussed_btn < 0)
			m_focussed_btn = (NUM_BUTTONS-1);
		break;
	case LV_KEY_UP:
		break;
	case LV_KEY_DOWN:
		break;
	default:
		break;
	}
}

void c20x_screen_poweroff_cb_set(c20x_screen_change_cb screen_change_cb)
{
	m_screen_change = screen_change_cb;
}

int c20x_screen_poweroff_load(C20X_SCREENS prev_screen_id, lv_group_t * lvgl_grp)
{
	int ret = 0;
	m_prev_screen_id = prev_screen_id;
	m_grp = lvgl_grp;

	/* label */
	label_pwr_off = lv_label_create(lv_scr_act());
	lv_label_set_recolor(label_pwr_off, true);
	lv_label_set_text(label_pwr_off, "#ff0000 Power off the device?#");
	lv_obj_set_style_text_align(label_pwr_off, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_align_to(label_pwr_off, lv_scr_act(), LV_ALIGN_TOP_MID, 0, 20);

	/* cancel button */
	btn_cancel = lv_btn_create(lv_scr_act());
	lv_obj_set_size(btn_cancel, 50, 30);
	lv_obj_add_event_cb(btn_cancel, btn_handler, LV_EVENT_CLICKED, NULL);
	lv_obj_align(btn_cancel, LV_ALIGN_CENTER, -50, 40);
	lv_obj_t * label = lv_label_create(btn_cancel);
	lv_label_set_text(label, "Cancel");
	lv_obj_center(label);

	/* ok button */
	btn_ok = lv_btn_create(lv_scr_act());
	lv_obj_set_size(btn_ok, 50, 30);
	lv_obj_add_event_cb(btn_ok, btn_handler, LV_EVENT_CLICKED, NULL);
	lv_obj_align(btn_ok, LV_ALIGN_CENTER, 50, 40);
	label = lv_label_create(btn_ok);
	lv_label_set_text(label, "Ok");
	lv_obj_center(label);

	/* add buttons to lvgl group */
	lv_group_add_obj(m_grp, btn_cancel);
	lv_group_add_obj(m_grp, btn_ok);
	lv_group_focus_obj(btn_ok);	// set focus ok
//	lv_group_set_focus_cb(m_grp, focus_cb_handler);
	m_focussed_btn = 1;

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(keypad_handler);

	return ret;
}

int c20x_screen_poweroff_deinit()
{
	int ret = 0;
	lv_obj_del(label_pwr_off);
	lv_obj_del(btn_cancel);
	lv_obj_del(btn_ok);

	/* set focus cb to NULL so it doesn't get called from other screens */
	lv_group_set_focus_cb(m_grp, NULL);

	return ret;
}



/* IMPORTANT: these functions should be called only once */
int c20x_screen_power_key_init()
{
	int ret = 0;
	k_work_init(&m_key_detection_work, key_detection_work_handler);
	return ret;
}

void c20x_screen_poweroff_styles_init()
{
//	lv_style_init(&bg_style);
}

void c20x_screen_poweroff_init_onetime()
{

}
/*
 *
 */
