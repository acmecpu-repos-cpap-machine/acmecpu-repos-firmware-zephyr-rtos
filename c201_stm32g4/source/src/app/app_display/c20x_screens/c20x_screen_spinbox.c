/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 1-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <zephyr.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_display_data.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"

/* static variables */
static const char *m_settings_save_path = NULL;
static app_display_key_cb m_prev_screen = NULL;

/* lvgl objects */
static lv_obj_t *m_spinbox;
static lv_obj_t *btn_plus;
static lv_obj_t *btn_minus;

/* lvgl styles */
static lv_style_t m_spin_style;
static lv_style_t m_spin_fo_style;
static lv_style_t m_spin_btn_style;
static lv_style_t m_spin_btn_fo_style;
static lv_style_t m_spin_btn_pr_style;

/* functions */

static void get_convert_save()	// TODO: make generic
{
    int32_t spin_val = lv_spinbox_get_value(m_spinbox);
    float val_cmh2o = (float) spin_val / 10;
    int32_t val_pa = val_cmh2o * 98.0665;
    LOG_INF("val_cmh2o = %0.2f, val_pa = %d", val_cmh2o, val_pa);
    app_settings_save_single(m_settings_save_path, &val_pa, sizeof(val_pa), true);
}

static void lv_spinbox_increment_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_increment(m_spinbox);
        get_convert_save();
    }
}

static void lv_spinbox_decrement_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_decrement(m_spinbox);
        get_convert_save();
    }
}

static void extra_btn_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ESC: {
		/* going to the previous screen, so delete the objects */
		lv_obj_del(m_spinbox);
		lv_obj_del(btn_plus);
		lv_obj_del(btn_minus);

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

static void lv_spinbox_style_set()
{
	lv_style_set_border_opa(&m_spin_style, LV_OPA_COVER);
//	lv_style_set_radius(&m_spin_style, 0);
    lv_style_set_border_width(&m_spin_style, 1);
    lv_style_set_border_color(&m_spin_style, lv_color_white());

    lv_style_set_text_color(&m_spin_fo_style, lv_color_black());

    lv_obj_add_style(m_spinbox, &m_spin_style, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(m_spinbox, &m_spin_fo_style, LV_PART_CURSOR);
}

static void lv_spinbox_btn_style_set()
{
	/* default */
	lv_style_set_bg_opa(&m_spin_btn_style, LV_OPA_COVER);
	lv_style_set_bg_color(&m_spin_btn_style, lv_color_black());
	lv_style_set_border_opa(&m_spin_btn_style, LV_OPA_COVER);
    lv_style_set_border_width(&m_spin_btn_style, 1);
    lv_style_set_border_color(&m_spin_btn_style, lv_color_white());
	lv_style_set_border_side(&m_spin_btn_style, LV_BORDER_SIDE_FULL);
	lv_style_set_pad_all(&m_spin_btn_style, 0);

	/* focused */
	lv_style_set_border_width(&m_spin_btn_fo_style, 3);
//	lv_style_set_border_color(&m_spin_btn_fo_style, lv_color_white());
	lv_style_set_border_side(&m_spin_btn_fo_style, LV_BORDER_SIDE_FULL);

	/* pressed */
	lv_style_set_bg_color(&m_spin_btn_pr_style, lv_color_white());
	lv_style_set_text_color(&m_spin_btn_pr_style, lv_color_black());
}

int c20x_screen_spinbox_init(	const char *display_name,
								int range_min,
								int range_max,
								int digit_count,
								int separator_position,
								int present_val,
								const char *settings_save_path,
								app_display_key_cb prev_screen_cb
		)
{
	int ret = 0;

	/* copy essentials */
	m_settings_save_path = settings_save_path;
	m_prev_screen = prev_screen_cb;

	/* set label */
	c20x_screen_settings_text_set(display_name);

	/* create a spinbox */
    m_spinbox = lv_spinbox_create(lv_scr_act());
    lv_spinbox_set_range(m_spinbox, range_min, range_max);
    lv_spinbox_set_digit_format(m_spinbox, digit_count, separator_position);
    lv_spinbox_step_prev(m_spinbox);
    lv_spinbox_set_value(m_spinbox, (present_val * 0.01 * 10));	// TODO: fix
    lv_obj_set_width(m_spinbox, C20X_SCREEN_SPINBOX_WIDTH_PX);
    lv_spinbox_set_pos(m_spinbox, 0);
    lv_obj_center(m_spinbox);
    lv_coord_t h = lv_obj_get_height(m_spinbox);

    /* add styles */
    lv_spinbox_style_set();
    lv_spinbox_btn_style_set();

    /* remove all other items from group */
    lv_group_remove_all_objs(app_display_lvgl_group_instance_get());

    /* create spinbox control button + */
    btn_plus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_plus, h, h);
    lv_obj_align_to(btn_plus, m_spinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_t *label = lv_label_create(btn_plus);
    lv_label_set_text(label, "+");
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_plus, lv_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);

	lv_obj_add_style(btn_plus, &m_spin_btn_style, LV_STATE_DEFAULT);
	lv_obj_add_style(btn_plus, &m_spin_btn_fo_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_plus, &m_spin_btn_pr_style, LV_STATE_PRESSED);

	/* add + button to lvgl group for navigation */
	lv_group_add_obj(app_display_lvgl_group_instance_get(), btn_plus);

	/* create spinbox control button - */
    btn_minus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_minus, h, h);
    lv_obj_align_to(btn_minus, m_spinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    label = lv_label_create(btn_minus);
    lv_label_set_text(label, "-");
    lv_obj_center(label);
    lv_obj_add_event_cb(btn_minus, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);

	lv_obj_add_style(btn_minus, &m_spin_btn_style, LV_STATE_DEFAULT);
	lv_obj_add_style(btn_minus, &m_spin_btn_fo_style, LV_STATE_FOCUSED);
	lv_obj_add_style(btn_minus, &m_spin_btn_pr_style, LV_STATE_PRESSED);

	/* add - button to lvgl group for navigation */
	lv_group_add_obj(app_display_lvgl_group_instance_get(), btn_minus);

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(extra_btn_handler);

	return ret;
}

/* IMPORTANT: this function should be called only once */
void c20x_screen_spinbox_styles_init()
{
	lv_style_init(&m_spin_style);
	lv_style_init(&m_spin_fo_style);
	lv_style_init(&m_spin_btn_style);
	lv_style_init(&m_spin_btn_fo_style);
	lv_style_init(&m_spin_btn_pr_style);
}
