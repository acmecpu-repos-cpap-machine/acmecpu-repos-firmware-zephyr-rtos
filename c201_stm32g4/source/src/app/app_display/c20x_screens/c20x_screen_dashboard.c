/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 27-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/device.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
//#include "app_settings/app_settings_display_data.h"
#include "c20x_screen_config.h"
#include "c20x_screen_dashboard.h"
#include "c20x_screen_manager.h"
#include "c20x_screen_common_styles.h"

#include "app_blower/app_blower.h"
#include "app_battery/app_battery.h"

#include "lib_events/lib_events.h"

/* dashboard icons images */
//LV_IMG_DECLARE(icon_time_pressed);
//LV_IMG_DECLARE(icon_time_released);
//LV_IMG_DECLARE(icon_settings_pressed);
//LV_IMG_DECLARE(icon_settings_released);
//LV_IMG_DECLARE(icon_menu_pressed);
//LV_IMG_DECLARE(icon_menu_released);
LV_IMG_DECLARE(settings_icon_pr_20x20);
LV_IMG_DECLARE(settings_icon_re_20x20);
LV_IMG_DECLARE(results_icon_pr_20x20);
LV_IMG_DECLARE(results_icon_re_20x20);

/* event variables */
static struct lib_events_callback m_cb_settings_changed;

struct Dash_Icon {
	lv_obj_t *bg;
	lv_obj_t *ibtn;
	lv_obj_t *label;
	uint8_t app_id;
};
struct Dash_Label {
	lv_obj_t *bg;
	lv_obj_t *label;
	char pretext[7];
};

//static lv_obj_t *dash_bg;			// dashboard screen background
static struct Dash_Label m_label[DASH_LABEL_IDX_MAX];
static struct Dash_Icon m_ico[C20X_SCREEN_DASH_NUM_ICONS_TO_DISP];
static lv_style_t bg_style;
static lv_style_t m_imgbtn_bg_style;
static lv_style_t m_imgbtn_bg_style_fo;

static int m_ico_idx = -1;
static lv_group_t * m_db_grp = NULL;
static c20x_screen_change_cb m_screen_change = NULL;

/* dashboard label values */
static char m_mode[30] = {0x00};

/*
 * Static functions
 */

static void draw_border_on_focused_icon(struct Dash_Icon *ico, lv_group_t *group)
{
	for (int i=0; i<C20X_SCREEN_DASH_NUM_ICONS_TO_DISP; i++) {
	    if (ico[i].ibtn == lv_group_get_focused(group)) {
	    	LOG_INF("app icon %d is focussed", i);
	    	lv_style_set_border_color(&m_imgbtn_bg_style_fo, lv_color_white());
#if (CONFIG_C20X_SCREENS_OLED)
	    	lv_style_set_border_width(&m_imgbtn_bg_style_fo, 1);
#elif (CONFIG_C20X_SCREENS_TFT)
	    	lv_style_set_border_width(&m_imgbtn_bg_style_fo, 2);
#endif
	        lv_obj_add_style(ico[i].bg, &m_imgbtn_bg_style_fo, LV_PART_MAIN);
	    } else {
//	    	lv_style_set_border_width(&m_imgbtn_bg_style, 0);
	        lv_obj_add_style(ico[i].bg, &m_imgbtn_bg_style, LV_PART_MAIN);
	    }
	}
}

static void btn_handler(lv_event_t *event)
{
	LOG_INF("Clicked %d\n", m_ico_idx);
	m_screen_change(m_ico[m_ico_idx].app_id);
}

void focus_cb_handler(struct _lv_group_t* grp)
{
//	lv_obj_t *focus_obj = *grp->obj_focus;
//	lv_group_focus_obj(focus_obj->parent);	// set focus to the bg of the imgbtn
	draw_border_on_focused_icon(m_ico, grp);
}

static void keypad_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ENTER:
		break;
	case LV_KEY_ESC:
		break;
	case LV_KEY_LEFT:
		break;
	case LV_KEY_RIGHT:
		break;
	case LV_KEY_UP:
		lv_group_focus_prev(m_db_grp);
		if (--m_ico_idx < 0)
			m_ico_idx = (C20X_SCREEN_DASH_NUM_ICONS_TO_DISP-1);
		break;
	case LV_KEY_DOWN:
		lv_group_focus_next(m_db_grp);
		if (++m_ico_idx >= C20X_SCREEN_DASH_NUM_ICONS_TO_DISP)
			m_ico_idx = 0;
		break;
	default:
		break;
	}
}

static void dash_bg_style_make()
{
#if (CONFIG_C20X_SCREENS_OLED)
	lv_style_set_bg_opa(&bg_style, LV_OPA_COVER);
	lv_style_set_border_color(&bg_style, lv_color_white());
	lv_style_set_border_width(&bg_style, 1);
	lv_style_set_border_side(&bg_style, LV_BORDER_SIDE_NONE);
	lv_style_set_pad_all(&bg_style, 0);
	lv_style_set_radius(&bg_style, 0);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_style_set_bg_opa(&bg_style, LV_OPA_50);
	lv_style_set_bg_color(&bg_style, lv_color_make(128, 128, 128));
	lv_style_set_border_color(&bg_style, lv_color_make(200, 200, 200));
	lv_style_set_border_width(&bg_style, 1);
	lv_style_set_border_side(&bg_style, LV_BORDER_SIDE_FULL);
//	lv_style_set_pad_all(&bg_style, 0);
//	lv_style_set_radius(&bg_style, 0);
#endif
}

static void imgbtn_bg_style_make()
{
	lv_style_set_bg_opa(&m_imgbtn_bg_style, LV_OPA_TRANSP);
	lv_style_set_radius(&m_imgbtn_bg_style, 0);

	lv_style_set_border_opa(&m_imgbtn_bg_style, LV_OPA_100);
	lv_style_set_border_color(&m_imgbtn_bg_style, lv_color_white());
	lv_style_set_border_side(&m_imgbtn_bg_style, LV_BORDER_SIDE_FULL);
	lv_style_set_border_width(&m_imgbtn_bg_style, 0);
}

//static void icon_bg_style_make()
//{
//	/* default */
//	lv_style_set_bg_opa(&m_icon_style, LV_OPA_TRANSP);
//	lv_style_set_radius(&m_icon_style, 0);
//	lv_style_set_border_color(&m_icon_style, lv_color_white());
//	lv_style_set_border_width(&m_icon_style, 0);
//	lv_style_set_border_side(&m_icon_style, LV_BORDER_SIDE_FULL);
//	lv_style_set_pad_all(&m_icon_style, 0);
//
//	/* focused */
//	lv_style_set_border_width(&m_icon_style_fo, 1);
//}

static void dash_icon_setup(struct Dash_Icon *icon, lv_obj_t *parent, uint8_t idx, uint16_t w, uint16_t h,
		uint16_t top_gap, const lv_img_dsc_t *pressed, const lv_img_dsc_t *released, const char * text)
{
	/* create a empty background */
	icon->bg = lv_obj_create(parent);
	lv_obj_set_width(icon->bg, w+4);
	lv_obj_set_height(icon->bg, h+4);
//	lv_obj_add_style(icon->bg, &m_imgbtn_bg_style, LV_PART_MAIN);

	/* create an image button */
	icon->ibtn = lv_imgbtn_create(parent);
	lv_obj_set_size(icon->ibtn, w, h);
	lv_imgbtn_set_src(icon->ibtn, LV_IMGBTN_STATE_RELEASED, NULL, released, NULL);
	lv_imgbtn_set_src(icon->ibtn, LV_IMGBTN_STATE_PRESSED, NULL, pressed, NULL);

	lv_obj_add_event_cb(icon->ibtn, btn_handler, LV_EVENT_CLICKED, NULL);

//	icon->label = lv_label_create(icon->bg);
//	lv_label_set_text(icon->label, text);
//	lv_obj_align(icon->bg, NULL, LV_ALIGN_IN_TOP_LEFT, (idx * w)+C20x_SCREEN_DASH_PADDING, top_gap+C20x_SCREEN_DASH_PADDING);
//	lv_obj_align(icon->ibtn, NULL, LV_ALIGN_IN_TOP_MID, 0, C20x_SCREEN_DASH_PADDING);
//	lv_obj_align(icon->label, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -C20x_SCREEN_DASH_PADDING);
}

#if 0
static int dashboard_setup_3apps(uint16_t x_width, uint16_t y_height, uint16_t sbar_height) {

	uint16_t w = x_width - (C20x_SCREEN_DASH_PADDING*4);	// left + right + 2 in middle
	uint16_t h = y_height - (C20x_SCREEN_DASH_PADDING*2);	// top and bottom
	w = w/3;

	/* setup icon 1 */
	m_ico_idx++;
	m_ico[m_ico_idx].app_id = C20X_SCREEN_SETTINGS;
	dash_icon_setup(&m_ico[m_ico_idx], m_ico_idx, w, h, sbar_height,
			(lv_img_dsc_t*) &icon_settings_pressed,
			(lv_img_dsc_t*) &icon_settings_released, "Settings");
	lv_obj_align(m_ico[m_ico_idx].bg, LV_ALIGN_TOP_LEFT, 0, sbar_height+C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].ibtn, LV_ALIGN_TOP_MID, 0, C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].label, LV_ALIGN_BOTTOM_MID, 0, -C20x_SCREEN_DASH_PADDING);

	/* setup icon 2 */
	m_ico_idx++;
	m_ico[m_ico_idx].app_id = C20X_SCREEN_DATETIME;
	dash_icon_setup(&m_ico[m_ico_idx], m_ico_idx, w, h, sbar_height,
			(lv_img_dsc_t*) &icon_time_pressed, (lv_img_dsc_t*) &icon_time_released, "Time");
	lv_obj_align(m_ico[m_ico_idx].bg, LV_ALIGN_TOP_MID, 0, sbar_height+C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].ibtn, LV_ALIGN_TOP_MID, 0, C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].label, LV_ALIGN_BOTTOM_MID, 0, -C20x_SCREEN_DASH_PADDING);

	/* setup icon 3 */
	m_ico_idx++;
	m_ico[m_ico_idx].app_id = C20X_SCREEN_MENU;
	dash_icon_setup(&m_ico[m_ico_idx], m_ico_idx, w, h, sbar_height,
			(lv_img_dsc_t*) &icon_menu_pressed, (lv_img_dsc_t*) &icon_menu_released, "Menu");
	lv_obj_align(m_ico[m_ico_idx].bg, LV_ALIGN_TOP_RIGHT, 0, sbar_height+C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].ibtn, LV_ALIGN_TOP_MID, 0, C20x_SCREEN_DASH_PADDING);
	lv_obj_align(m_ico[m_ico_idx].label, LV_ALIGN_BOTTOM_MID, 0, -C20x_SCREEN_DASH_PADDING);

	return 0;
}

static void dash_setup_button(struct Dash_Icon *icon, lv_obj_t *parent, uint8_t idx, uint16_t w, uint16_t h, const char* text)
{
	/* make an icon background */
	icon->bg = lv_obj_create(parent);
	lv_obj_set_width(icon->bg, w+2);
	lv_obj_set_height(icon->bg, h+2);
	lv_obj_add_style(icon->bg, &bg_style, 0);

	/* make a button */
	icon->ibtn = lv_btn_create(icon->bg);
	lv_obj_set_width(icon->ibtn, w);
	lv_obj_set_height(icon->ibtn, h);

	lv_obj_set_style_bg_opa(icon->ibtn, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_border_color(icon->ibtn, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_border_width(icon->ibtn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
//	lv_obj_set_style_outline_width(icon->ibtn, 4, LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_set_style_border_side(icon->ibtn, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(icon->ibtn, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_FOCUSED);
	lv_obj_set_style_bg_color(icon->ibtn, lv_color_black(), LV_PART_MAIN);
	lv_obj_set_style_pad_all(icon->ibtn, 0, LV_PART_MAIN);

	lv_obj_add_event_cb(icon->ibtn, btn_handler, LV_EVENT_CLICKED, NULL);
//	lv_obj_align_to(icon->ibtn, icon->bg, LV_ALIGN_CENTER, 0, 0);
	lv_obj_center(icon->ibtn);

	icon->label = lv_label_create(icon->ibtn);
//	lv_obj_set_style_text_color(icon->label, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);
//	lv_obj_set_style_text_color(icon->label, lv_color_white(), LV_PART_MAIN | LV_PART_SELECTED);
	lv_obj_set_style_text_color(icon->label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(icon->label, text);
    lv_obj_center(icon->label);
}
#endif

static void dashboard_setup_2apps_vertical(uint16_t x_width, uint16_t y_height, uint16_t y_offset)
{
	uint16_t btn_w = C20x_SCREEN_DASH_ICON_WIDTH;
	uint16_t btn_h = C20x_SCREEN_DASH_ICON_HEIGHT;

	/* setup icon 1 */
	m_ico_idx++;
	m_ico[m_ico_idx].app_id = C20X_SCREEN_SETTINGS;
//	dash_setup_button(&m_ico[m_ico_idx], lv_scr_act(), m_ico_idx, btn_w, btn_h, "S");
	dash_icon_setup(&m_ico[m_ico_idx], lv_scr_act(), m_ico_idx, btn_w, btn_h,
			y_offset, &settings_icon_pr_20x20, &settings_icon_re_20x20, NULL);
#if (CONFIG_C20X_SCREENS_OLED)
	lv_obj_align_to(m_ico[m_ico_idx].bg, lv_scr_act(), LV_ALIGN_TOP_RIGHT, -4, y_offset);
#elif (CONFIG_C20X_SCREENS_TFT)
//	lv_obj_align_to(m_ico[m_ico_idx].bg, lv_scr_act(), LV_ALIGN_TOP_RIGHT, -20, y_offset+30);
	lv_obj_align_to(m_ico[m_ico_idx].bg, lv_scr_act(), LV_ALIGN_BOTTOM_RIGHT, -20, -60);
#endif
	lv_obj_align_to(m_ico[m_ico_idx].ibtn, m_ico[m_ico_idx].bg, LV_ALIGN_CENTER, 0, 0);

	/* setup icon 2 */
	m_ico_idx++;
	m_ico[m_ico_idx].app_id = C20x_SCREEN_RESULTS;
//	dash_setup_button(&m_ico[m_ico_idx], lv_scr_act(), m_ico_idx, btn_w, btn_h, "R");
	dash_icon_setup(&m_ico[m_ico_idx], lv_scr_act(), m_ico_idx, btn_w, btn_h,
			y_offset, &results_icon_pr_20x20, &results_icon_re_20x20, NULL);
#if (CONFIG_C20X_SCREENS_OLED)
	lv_obj_align_to(m_ico[m_ico_idx].bg, lv_scr_act(), LV_ALIGN_BOTTOM_RIGHT, -4, -2);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_obj_align_to(m_ico[m_ico_idx].bg, lv_scr_act(), LV_ALIGN_BOTTOM_RIGHT, -20, -20);
#endif
	lv_obj_align_to(m_ico[m_ico_idx].ibtn, m_ico[m_ico_idx].bg, LV_ALIGN_CENTER, 0, 0);
}

static int c20x_screen_dashboard_label_set(const char* txt_val, DASH_LABEL_IDX idx)
{
	if ((txt_val == NULL) || (idx >= DASH_LABEL_IDX_MAX)) {
		LOG_ERR("invalid param");
		return -EINVAL;
	}

	char label_text[20] = {0x00};
	sprintf(label_text, "%s%s", m_label[idx].pretext, txt_val);
	LOG_DBG("[%d] %s", idx, label_text);
	lv_label_set_text(m_label[idx].label, label_text);
	return 0;
}

static void dash_label_setup(struct Dash_Label *label, lv_obj_t *parent, uint16_t label_width, uint16_t label_height, bool bg_white, const char* text)
{
	label->bg = lv_obj_create(parent);
	lv_obj_set_width(label->bg, label_width);
	lv_obj_set_height(label->bg, label_height);

	lv_obj_add_style(label->bg, &bg_style, 0);

#if (CONFIG_C20X_SCREENS_OLED)
	if (bg_white)
		lv_obj_set_style_bg_color(label->bg, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
#endif

	label->label = lv_label_create(parent);
	lv_obj_add_style(label->label, c20x_screen_cmn_style_textsqueezed_obj_get(), LV_PART_MAIN | LV_STATE_DEFAULT);
#if (CONFIG_C20X_SCREENS_OLED)
	if (bg_white) {
//		lv_obj_set_style_bg_color(label->label, lv_color_white(), LV_PART_MAIN);
		lv_obj_set_style_text_color(label->label, lv_color_black(), LV_PART_MAIN);
	}
	else {
//		lv_obj_set_style_bg_color(label->label, lv_color_black(), LV_PART_MAIN);
		lv_obj_set_style_text_color(label->label, lv_color_white(), LV_PART_MAIN);
	}
#endif
	lv_label_set_long_mode(label->label, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(label->label, label_width);
#if (CONFIG_C20X_SCREENS_TFT)
//	lv_obj_set_height(label->label, label_height);
#endif
	lv_label_set_text(label->label, text);
	lv_obj_set_style_text_align(label->label, LV_TEXT_ALIGN_LEFT, 0);
//    lv_obj_align_to(label->label, label->bg, LV_ALIGN_LEFT_MID, 2, 0);
}

static void setting_value_to_text_get(const char *settings_path, char *out_buf)
{
	struct setting_value val;
	memset(&val, 0, sizeof(struct setting_value));
	int ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
	if (ret != 0) {
		strcpy(out_buf, "err");
	} else {
		int idx = app_settings_array_idx_get(settings_path);
		struct app_settings_data const* asd = app_settings_data_obj_get(idx);
		app_settings_option_val_to_key(asd->options, &val, out_buf);
	}
}

static void dashboard_setup_labels_vertical(uint16_t x_width, uint16_t y_height, uint16_t y_offset)
{
	int ret=0;
	char buf[30] = {0x00};
	lv_obj_t *next_align = NULL;
	uint16_t label_w = C20x_SCREEN_DASH_LABEL_WIDTH_MAX;
#if (CONFIG_C20X_SCREENS_OLED)
	uint16_t label_h = C20x_SCREEN_DASH_LABEL_HEIGHT_MAX-1;
#elif (CONFIG_C20X_SCREENS_TFT)
	uint16_t label_h = C20x_SCREEN_DASH_LABEL_HEIGHT_MAX-1;
#endif

	/* User label */
	ret = app_settings_load_single(SETTINGS_KEY_FULL_NAM, buf, SETTING_VAL_SRN_LEN_MAX);
	if (ret != 0) {
		strcpy(buf, "err");
	}
	dash_label_setup(&m_label[DASH_LABEL_IDX_USER], lv_scr_act(), label_w, label_h, true, buf);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_USER].bg, lv_scr_act(), LV_ALIGN_TOP_LEFT, 0, y_offset);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_USER].label, m_label[DASH_LABEL_IDX_USER].bg, LV_ALIGN_LEFT_MID, 2, 0);
	next_align = m_label[DASH_LABEL_IDX_USER].bg;

	memset(buf, 0x00, sizeof(buf));

	/* State label */
	setting_value_to_text_get(SETTINGS_KEY_FULL_BST, buf);
	dash_label_setup(&m_label[DASH_LABEL_IDX_STATE], lv_scr_act(), (label_w*0.3), label_h, true, buf);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_STATE].bg, next_align, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 1);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_STATE].label, m_label[DASH_LABEL_IDX_STATE].bg, LV_ALIGN_LEFT_MID, 2, 0);
	next_align = m_label[DASH_LABEL_IDX_STATE].bg;

	memset(buf, 0x00, sizeof(buf));

	/* Mode label */
	setting_value_to_text_get(SETTINGS_KEY_FULL_TS_MOD, m_mode);
	dash_label_setup(&m_label[DASH_LABEL_IDX_MODE], lv_scr_act(), (label_w*0.7), label_h, false, m_mode);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_MODE].bg, next_align, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_MODE].label, m_label[DASH_LABEL_IDX_MODE].bg, LV_ALIGN_LEFT_MID, 2, 0);
//	next_align = m_label[DASH_LABEL_IDX_MODE].bg;

	memset(buf, 0x00, sizeof(buf));

	/* Pressure label */
	strcpy(m_label[DASH_LABEL_IDX_PRESSURE].pretext, "P: ");
	char pressure[6] = {0x00};
	setting_value_to_text_get(SETTINGS_KEY_FULL_TS_FIC, pressure);
	sprintf(buf, "%s%s cmH2O", m_label[DASH_LABEL_IDX_PRESSURE].pretext, pressure);
	dash_label_setup(&m_label[DASH_LABEL_IDX_PRESSURE], lv_scr_act(), label_w, label_h, true, buf);
//	lv_obj_align_to(m_label[DASH_LABEL_IDX_PRESSURE].bg, next_align, LV_ALIGN_TOP_LEFT, 0, y_align_offset);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_PRESSURE].bg, next_align, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 1);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_PRESSURE].label, m_label[DASH_LABEL_IDX_PRESSURE].bg, LV_ALIGN_LEFT_MID, 2, 0);
	next_align = m_label[DASH_LABEL_IDX_PRESSURE].bg;

	memset(buf, 0x00, sizeof(buf));

	/* Runtime label */
	strcpy(m_label[DASH_LABEL_IDX_RUNTIME].pretext, "T:");
	struct tm rem_time;
	memset(&rem_time, 0x00, sizeof(struct tm));
	app_battery_runtime_get(&rem_time);
	char tmbuf[20] = {0x00};
	strftime(tmbuf, sizeof(tmbuf), "%kh %Mm", &rem_time);
	sprintf(buf, "%s%s", m_label[DASH_LABEL_IDX_RUNTIME].pretext, tmbuf);
	dash_label_setup(&m_label[DASH_LABEL_IDX_RUNTIME], lv_scr_act(), label_w, label_h, true, buf);
//	lv_obj_align_to(m_label[DASH_LABEL_IDX_RUNTIME].bg, next_align, LV_ALIGN_TOP_LEFT, 0, y_align_offset);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_RUNTIME].bg, next_align, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 1);
	lv_obj_align_to(m_label[DASH_LABEL_IDX_RUNTIME].label, m_label[DASH_LABEL_IDX_RUNTIME].bg, LV_ALIGN_LEFT_MID, 2, 0);
	next_align = m_label[DASH_LABEL_IDX_RUNTIME].bg;

	memset(buf, 0x00, sizeof(buf));

	/* Error label */
//	strcpy(m_label[DASH_LABEL_IDX_ERROR].pretext, "E:");
//	sprintf(buf, "%s%s", m_label[DASH_LABEL_IDX_ERROR].pretext, app_error_code_to_msg(APP_ERR_NO_ERROR));
//	dash_label_setup(&m_label[DASH_LABEL_IDX_ERROR], dash_bg, label_w, label_h, false, buf);
//	lv_obj_align_to(m_label[DASH_LABEL_IDX_ERROR].bg, dash_bg, LV_ALIGN_TOP_LEFT, 0, 41);

}


/*
 * Global functions
 */
int c20x_screen_dashboard_init(uint16_t x_width, uint16_t y_height, uint16_t sbar_height, lv_group_t * lvgl_grp) {
	int ret = 0;

	/* reset the index */
	m_ico_idx = -1;

	/* setup styles */
	dash_bg_style_make();
//	icon_bg_style_make();
	imgbtn_bg_style_make();

	/* setup labels */
	dashboard_setup_labels_vertical(x_width, y_height, sbar_height+1);

	/* setup buttons with icons */
	dashboard_setup_2apps_vertical(x_width, y_height, sbar_height+1);

	/* setup lvgl group for navigation */
	m_db_grp = lvgl_grp;

	for (int i=0; i<C20X_SCREEN_DASH_NUM_ICONS_TO_DISP; i++) {
		lv_group_add_obj(m_db_grp, m_ico[i].ibtn);
	}

	/* set the lvgl group for navigation */
	m_ico_idx = 0;
	lv_group_focus_obj(m_ico[m_ico_idx].ibtn);	// set focus to the first icon

	lv_group_set_focus_cb(m_db_grp, focus_cb_handler);

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(keypad_handler);

	/* draw rect on the first icon */
	draw_border_on_focused_icon(m_ico, m_db_grp);
	return ret;
}

int c20x_screen_dashboard_deinit()
{
	int ret = 0;
//	lv_group_remove_all_objs(m_db_grp);
	for (int i=0; i<C20X_SCREEN_DASH_NUM_ICONS_TO_DISP; i++) {
		lv_obj_del(m_ico[i].ibtn);
		lv_obj_del(m_ico[i].bg);
	}

	for (int i=0; i<DASH_LABEL_IDX_MAX; i++) {
		lv_obj_del(m_label[i].bg);
	}

	/* set focus cb to NULL so it doesn't get called from other screens */
	lv_group_set_focus_cb(m_db_grp, NULL);

	return ret;
}

void c20x_screen_db_cb_set(c20x_screen_change_cb screen_change_cb)
{
	m_screen_change = screen_change_cb;
}

int c20x_screen_dashboard_pressure_label_set(float pres_cmh2o)
{
	if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
		return -1;

	char buf[20] = {0x00};
	sprintf(buf, "%.1fcmH2O", (double)pres_cmh2o);
	return c20x_screen_dashboard_label_set(buf, DASH_LABEL_IDX_PRESSURE);
}

int c20x_screen_dashboard_runtime_label_set(struct tm *tp)
{
	if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
		return -1;
//	char buf[20] = {0x00};
	char tmbuf[20] = {0x00};
	strftime(tmbuf, sizeof(tmbuf), "%kh %Mm", tp);
//	sprintf(buf, "%s%s", m_label[DASH_LABEL_IDX_RUNTIME].pretext, tmbuf);
	return c20x_screen_dashboard_label_set(tmbuf, DASH_LABEL_IDX_RUNTIME);
}

//int c20x_screen_dashboard_error_label_set(APP_ERRORS err_code)
//{
//	if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
//		return -1;
//	char buf[20] = {0x00};
//	sprintf(buf, "%s", app_error_code_to_msg(err_code));
//	return c20x_screen_dashboard_label_set(buf, DASH_LABEL_IDX_ERROR);
//}

void c20x_screen_dashboard_reload()
{
	if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
		return;

	LOG_DBG("reloading dashboard labels");

	char buf[30] = {0x00};

	/* refresh user label */
	lv_label_set_text(m_label[DASH_LABEL_IDX_USER].label, NULL);

	/* reload state label */
//	setting_value_to_text_get(SETTINGS_KEY_FULL_BST, buf);
	uint8_t state = 0;
#if CONFIG_APP_BLOWER
	state = app_blower_run_state_get();
#endif
	struct setting_value val;
	val.val1 = state; val.val2 = 0;
	int idx = app_settings_array_idx_get(SETTINGS_KEY_FULL_BST);
	struct app_settings_data const* asd = app_settings_data_obj_get(idx);
	app_settings_option_val_to_key(asd->options, &val, buf);
	c20x_screen_dashboard_label_set(buf, DASH_LABEL_IDX_STATE);

	/* refresh mode label */
//	lv_label_set_text(m_label[DASH_LABEL_IDX_MODE].label, NULL);
	c20x_screen_dashboard_label_set(m_mode, DASH_LABEL_IDX_MODE);

	/* reload pressure label */
	memset(buf, 0x00, sizeof(buf));
//	setting_value_to_text_get(SETTINGS_KEY_FULL_TS_FIC, buf);
//	strcat(buf, " cmH2O");
	if (state == BLOWER_NOT_RUNNING) {
		sprintf(buf, "0.0 cmH2O");
		c20x_screen_dashboard_label_set(buf, DASH_LABEL_IDX_PRESSURE);
	} else if (state == BLOWER_RUNNING) {
		struct app_blower_params params;
#if CONFIG_APP_BLOWER
		app_blower_runtime_params_get(&params);
#endif
		sprintf(buf, "%0.1f cmH2O", ((double)params.controlled_press_kpa * PRESS_KPA_TO_CMH2O_MUL));
		c20x_screen_dashboard_label_set(buf, DASH_LABEL_IDX_PRESSURE);
	}

	/* reload runtime label */
	struct tm rem_time;
	memset(&rem_time, 0x00, sizeof(struct tm));
	app_battery_runtime_get(&rem_time);
	c20x_screen_dashboard_runtime_label_set(&rem_time);

	/*refresh error label */
//	lv_label_set_text(m_label[DASH_LABEL_IDX_ERROR].label, NULL);
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch (event) {
	case LIB_EVENT_POWER_OFF:
	{
		break;
	}
	case LIB_EVENT_SETTINGS_CHANGED:
	{
		char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
		app_settings_changed_latest_get(changed_setting);
		if ((strcmp(changed_setting, SETTINGS_KEY_FULL_TS_MOD) == 0)) {
			memset(m_mode, 0x00, sizeof(m_mode));
			setting_value_to_text_get(SETTINGS_KEY_FULL_TS_MOD, m_mode);
		}
		break;
	}
	default:
		break;
	}
}

/* IMPORTANT: these functions should be called only once */
void c20x_screen_dashboard_styles_init()
{
	lv_style_init(&bg_style);
//	lv_style_init(&m_icon_style);
//	lv_style_init(&m_icon_style_fo);
	lv_style_init(&m_imgbtn_bg_style);
	lv_style_init(&m_imgbtn_bg_style_fo);
}

void c20x_screen_dashboard_init_onetime()
{
	lib_events_callback_add(&m_cb_settings_changed, app_event_handler, LIB_EVENT_SETTINGS_CHANGED);
}
/*
 *
 */
