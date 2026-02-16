/*
 * Copyright (c) 2021 Acme CPU
 * c20x_screen_switch.c
 *
 *  Created on: 24-Nov-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <device.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_display_data.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"

static const char *m_settings_save_path = NULL;
static uint8_t m_datatype = SETTING_DATATYPE_NONE;
static app_display_key_cb m_prev_screen = NULL;

static lv_style_t m_sw_style;
static lv_style_t m_sw_knob_style;

//static lv_obj_t *m_sw_label = NULL;
static lv_obj_t *m_sw = NULL;

//static void switch_handler(lv_obj_t * obj, lv_event_t event)
static void switch_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t * obj = lv_event_get_target(event);
    LV_UNUSED(obj);
    if(code == LV_EVENT_VALUE_CHANGED) {
//        printf("State: %s\n", lv_switch_get_state(obj) ? "On" : "Off");

		int ret = 0;
		switch (m_datatype) {
		case SETTING_DATATYPE_UINT8: {
//			uint8_t state = lv_switch_get_state(obj);
			uint8_t state = lv_obj_has_state(obj, LV_STATE_CHECKED);
			ret = app_settings_save_single(m_settings_save_path, &state, sizeof(state), true);
			break;
		}
		case SETTING_DATATYPE_UINT16: {
//			uint16_t state = lv_switch_get_state(obj);
			uint16_t state = lv_obj_has_state(obj, LV_STATE_CHECKED);
			ret = app_settings_save_single(m_settings_save_path, &state, sizeof(state), true);
			break;
		}
		case SETTING_DATATYPE_UINT32: {
//			uint32_t state = lv_switch_get_state(obj);
			uint32_t state = lv_obj_has_state(obj, LV_STATE_CHECKED);
			ret = app_settings_save_single(m_settings_save_path, &state, sizeof(state), true);
			break;
		}
		case SETTING_DATATYPE_INT: {
//			int state = lv_switch_get_state(obj);
			int state = lv_obj_has_state(obj, LV_STATE_CHECKED);
			ret = app_settings_save_single(m_settings_save_path, &state, sizeof(state), true);
			break;
		}
		default:
			break;
		}

		if (ret) {
			LOG_ERR("settings save failed!");
		}
	}
}

static void extra_btn_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ESC: {
		/* going to the previous screen, so delete the objects */
//		lv_obj_del(m_sw_label);
		lv_obj_del(m_sw);

		/* call the previous screen button handler to load it */
		m_prev_screen(key);

		break;
	}
	case LV_KEY_HOME:
		break;
	default:
		break;

	}
}

static void lv_switch_style_set()
{
	/* main */
	lv_style_set_bg_opa(&m_sw_style, LV_OPA_COVER);
	lv_style_set_border_opa(&m_sw_style, LV_OPA_COVER);
//	lv_style_set_radius(&m_sw_style, 5);
    lv_style_set_border_width(&m_sw_style, 1);
    lv_style_set_border_color(&m_sw_style, lv_color_white());

    /* knob */
//    lv_style_set_radius(&m_sw_knob_style, 5);
	lv_style_set_bg_color(&m_sw_knob_style, lv_color_black());

}

int c20x_screen_switch_init(const char *display_name, bool present_val,
		const char *settings_save_path, uint8_t datatype,
		app_display_key_cb prev_screen_cb) {
	int ret = 0;

	/* copy */
	m_settings_save_path = settings_save_path;
	m_datatype = datatype;
	m_prev_screen = prev_screen_cb;

	/* Create a label for the display name */
//	m_sw_label = lv_label_create(lv_scr_act(), NULL);
//	lv_label_set_align(m_sw_label, LV_LABEL_ALIGN_LEFT);
//	lv_label_set_text(m_sw_label, display_name);
////	lv_obj_align(m_sw_label, NULL, LV_ALIGN_CENTER, 0, -40);
//	lv_obj_align(m_sw_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 5);

	c20x_screen_settings_text_set(display_name);

//	lv_obj_set_flex_flow(lv_scr_act(), LV_FLEX_FLOW_COLUMN);
//	lv_obj_set_flex_align(lv_scr_act(), LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	/* Create a switch and apply the styles */
	m_sw = lv_switch_create(lv_scr_act());
	lv_obj_set_width(m_sw, C20X_SCREEN_SETTINGS_SWITCH_WIDTH_PX);
	lv_obj_set_height(m_sw, C20X_SCREEN_SETTINGS_SWITCH_HEIGHT_PX);
	lv_obj_align(m_sw, LV_ALIGN_CENTER, 0, 0);

	/* add styles */
	lv_switch_style_set();
    lv_obj_add_style(m_sw, &m_sw_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(m_sw, &m_sw_knob_style, LV_PART_KNOB | LV_STATE_CHECKED);


	if (present_val) {
//		lv_switch_on(m_sw, LV_ANIM_OFF);
		lv_obj_add_state(m_sw, LV_STATE_CHECKED);
	}
	else {
//		lv_switch_off(m_sw, LV_ANIM_OFF);
		lv_obj_clear_state(m_sw, LV_STATE_CHECKED);
	}

//	lv_obj_set_event_cb(m_sw, switch_handler);
	lv_obj_add_event_cb(m_sw, switch_handler, LV_EVENT_ALL, NULL);

	/* set the lvgl group for navigation */
	app_display_lvgl_group_set_current(m_sw);

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(extra_btn_handler);

	return ret;
}

/* IMPORTANT: this function should be called only once */
void c20x_screen_switch_styles_init()
{
	lv_style_init(&m_sw_style);
	lv_style_init(&m_sw_knob_style);
}
