/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 24-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
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
#if CONFIG_APP_BATTERY
#include "app_battery/app_battery.h"
#endif
#include "app_time/app_time.h"
#include "c20x_screen_manager.h"
#include "c20x_screen_common_styles.h"

#include "lib_events/lib_events.h"

static struct lib_events_callback m_cb_settings_changed;
static struct lib_events_callback m_evnt_cb_wifi_started;
static struct lib_events_callback m_evnt_cb_wifi_stopped;
static struct lib_events_callback m_evnt_cb_bt_started;
static struct lib_events_callback m_evnt_cb_bt_stopped;


K_SEM_DEFINE(m_sbar_lock, 0, 1);

/* images */
//LV_IMG_DECLARE(batt_25);
//LV_IMG_DECLARE(batt_50);
//LV_IMG_DECLARE(batt_75);
//LV_IMG_DECLARE(batt_100);
LV_IMG_DECLARE(batt);
LV_IMG_DECLARE(batt_charge);
LV_IMG_DECLARE(bluetooth);
LV_IMG_DECLARE(wifi);

//static lv_style_t statbar_style;
static lv_obj_t *bg = NULL;				// background
static lv_obj_t *bat_percent = NULL;	// battery percent text
static lv_obj_t *ico_bat = NULL;		// battery image
static lv_obj_t *ico_wifi = NULL;		// wifi image
static lv_obj_t *ico_blue = NULL;		// bluetooth image
//static lv_obj_t *lte;					// LTE image
static lv_obj_t *datetime = NULL;		// date time text

/* status of network interfaces */
static bool m_wifi_stat = false;
static bool m_hp_stat = false;
static bool m_bt_stat = false;


//static void status_bar_style_create() {
//	lv_style_init(&statbar_style);
//#if CONFIG_C20X_SCREENS_OLED
//	lv_style_set_bg_opa(&statbar_style, LV_STATE_DEFAULT, LV_OPA_COVER);
//
//	/* https://docs.lvgl.io/master/overview/style-props.html#border */
////	lv_style_set_border_post(&statbar_style, LV_STATE_DEFAULT, false);
////	lv_style_set_border_color(&statbar_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
//	lv_style_set_border_opa(&statbar_style, LV_STATE_DEFAULT, LV_OPA_20);
//	lv_style_set_border_side(&statbar_style, LV_STATE_DEFAULT, LV_BORDER_SIDE_BOTTOM);
//
////	lv_style_set_radius(&statbar_style, LV_STATE_DEFAULT, 0);
////	lv_style_set_bg_color(&statbar_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
//#endif
//}

static void status_bar_batt_load(lv_obj_t *lv_batt_per, lv_obj_t *lv_batt) {
	/* Get the battery level */
	uint8_t batt_level=0;
#if CONFIG_APP_BATTERY
	app_battery_level_get(&batt_level);
#endif
	char buf[5] = {0x00};
	sprintf(buf, "%d%%", batt_level);
	lv_label_set_text(lv_batt_per, buf);

	if (app_battery_chargestat_get()) {
#if (CONFIG_C20X_SCREENS_OLED)
		lv_img_set_src(lv_batt, &batt_charge);
#elif (CONFIG_C20X_SCREENS_TFT)
		lv_img_set_src(lv_batt, LV_SYMBOL_CHARGE);
#endif
	} else {
#if (CONFIG_C20X_SCREENS_OLED)
		lv_img_set_src(lv_batt, &batt);
#elif (CONFIG_C20X_SCREENS_TFT)
		if (batt_level > 75)
			lv_img_set_src(lv_batt, LV_SYMBOL_BATTERY_FULL);
		else if (batt_level > 50)
			lv_img_set_src(lv_batt, LV_SYMBOL_BATTERY_3);
		else if (batt_level > 25)
			lv_img_set_src(lv_batt, LV_SYMBOL_BATTERY_2);
		else
			lv_img_set_src(lv_batt, LV_SYMBOL_BATTERY_1);
#endif
	}
}

static void status_bar_bluetooth_load(lv_obj_t *lv_bt)
{
#if (CONFIG_C20X_SCREENS_OLED)
	lv_img_set_src(lv_bt, &bluetooth);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_img_set_src(lv_bt, LV_SYMBOL_BLUETOOTH);
#endif
}

static void status_bar_wifi_load(lv_obj_t *lv_wifi)
{
#if (CONFIG_C20X_SCREENS_OLED)
	lv_img_set_src(lv_wifi, &wifi);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_img_set_src(lv_wifi, LV_SYMBOL_WIFI);
#endif
}

/* Format times as: YYYY-MM-DD HH:MM:SS DOW DOY */
static const char *format_time(struct tm *tp, long nsec)
{
	static char buf[64] = {0x00};
	char *bp = buf;
	char *const bpe = bp + sizeof(buf);

//	bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
//	bp += strftime(bp, bpe - bp, "%m-%d,%H:%M", tp);
//	bp += strftime(bp, bpe - bp, "%k:%M", tp);
#if (CONFIG_C20X_SCREENS_OLED)
	bp += strftime(bp, bpe - bp, "%H:%M", tp);
#elif (CONFIG_C20X_SCREENS_TFT)
	bp += strftime(bp, bpe - bp, "%d %b, %H:%M", tp);
#endif
	if (nsec >= 0) {
		bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
	}
//	bp += strftime(bp, bpe - bp, " %a %j", tp);
	return buf;
}

static void status_bar_time_load(lv_obj_t *lv_time) {
	struct tm time;
	app_time_value_get(&time);
	const char *tmfmt = format_time(&time, -1);
	LOG_INF("time: %s", (tmfmt));

	lv_label_set_text(lv_time, tmfmt);
}

static void status_bar_setup(uint32_t x_width, uint32_t sbar_height) {
	bg = lv_obj_create(lv_scr_act());
	lv_obj_set_width(bg, x_width);
	lv_obj_set_height(bg, sbar_height);
//	lv_obj_add_style(bg, LV_OBJ_PART_MAIN, &statbar_style);

#if CONFIG_C20X_SCREENS_OLED
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_radius(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_color(bg, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_width(bg, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_border_side(bg, LV_BORDER_SIDE_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
//	lv_obj_set_style_border_side(bg, LV_BORDER_SIDE_FULL, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_top(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_bottom(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_left(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_right(bg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

	/* https://docs.lvgl.io/master/overview/style-props.html#border */
//	lv_style_set_bg_opa(&statbar_style, LV_STATE_DEFAULT, LV_OPA_COVER);
//	lv_style_set_border_post(&statbar_style, LV_STATE_DEFAULT, false);
//	lv_style_set_border_color(&statbar_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
//	lv_style_set_border_opa(&statbar_style, LV_STATE_DEFAULT, LV_OPA_20);
//	lv_style_set_border_side(&statbar_style, LV_STATE_DEFAULT, LV_BORDER_SIDE_BOTTOM);
//	lv_style_set_radius(&statbar_style, LV_STATE_DEFAULT, 0);
//	lv_style_set_bg_color(&statbar_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
#endif
	lv_obj_align(bg, LV_ALIGN_TOP_MID, 0, 0);

//	lv_style_init(&statbar_style);
//	lv_style_set_text_font(statbar_style, &lv_font_montserrat_12);
//	lv_obj_set_style_text_font(datetime, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

	/* load date time */
	datetime = lv_label_create(bg);
	lv_obj_add_style(datetime, c20x_screen_cmn_style_textsqueezed_obj_get(), LV_PART_MAIN | LV_STATE_DEFAULT);	// add global style
	lv_obj_align_to(datetime, bg, LV_ALIGN_TOP_LEFT, 0, 0);
	status_bar_time_load(datetime);

	/* load battery level image and text */
	bat_percent = lv_label_create(bg);
//	lv_obj_add_style(bat_percent, c20x_screen_cmn_style_textsqueezed_obj_get(), LV_PART_MAIN | LV_STATE_DEFAULT);	// add global style
	ico_bat = lv_img_create(bg);
	status_bar_batt_load(bat_percent, ico_bat);
#if (CONFIG_C20X_SCREENS_OLED)
	lv_obj_align_to(bat_percent, bg, LV_ALIGN_TOP_RIGHT, 0, 0);
	lv_obj_align_to(ico_bat, bat_percent, LV_ALIGN_OUT_LEFT_MID, -1, 0);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_obj_align_to(bat_percent, bg, LV_ALIGN_TOP_RIGHT, -5, 0);
	lv_obj_align_to(ico_bat, bat_percent, LV_ALIGN_OUT_LEFT_MID, -5, 0);
#endif

	/* load wifi image */
	ico_wifi = lv_img_create(bg);
	status_bar_wifi_load(ico_wifi);
#if (CONFIG_C20X_SCREENS_OLED)
	lv_obj_align_to(ico_wifi, ico_bat, LV_ALIGN_OUT_LEFT_MID, -1, 0);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_obj_align_to(ico_wifi, ico_bat, LV_ALIGN_OUT_LEFT_MID, -5, 0);
#endif
//		lv_label_set_text(wifi, LV_SYMBOL_WIFI);
	if (m_wifi_stat) {
		lv_obj_set_style_img_opa(ico_wifi, LV_OPA_COVER, LV_PART_MAIN);
	} else {
		lv_obj_set_style_img_opa(ico_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
	}

	/* load bluetooth image */
	ico_blue = lv_img_create(bg);
	status_bar_bluetooth_load(ico_blue);
#if (CONFIG_C20X_SCREENS_OLED)
	lv_obj_align_to(ico_blue, ico_wifi, LV_ALIGN_OUT_LEFT_MID, -1, 0);
#elif (CONFIG_C20X_SCREENS_TFT)
	lv_obj_align_to(ico_blue, ico_wifi, LV_ALIGN_OUT_LEFT_MID, -5, 0);
#endif
//		lv_label_set_text(blue, LV_SYMBOL_BLUETOOTH);
	if (m_bt_stat) {
		lv_obj_set_style_img_opa(ico_blue, LV_OPA_COVER, LV_PART_MAIN);
	} else {
		lv_obj_set_style_img_opa(ico_blue, LV_OPA_TRANSP, LV_PART_MAIN);
	}
}

void c20x_screen_statusbar_reload()
{
	k_sem_take(&m_sbar_lock, K_MSEC(1));

	if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
		return;

//	if ((datetime == NULL) || (ico_bat == NULL) || (ico_wifi == NULL) || (ico_blue == NULL))	{
//		return;
//	}

	status_bar_time_load(datetime);
	status_bar_batt_load(bat_percent, ico_bat);
//	status_bar_wifi_load(ico_wifi);
//	status_bar_bluetooth_load(ico_blue);

	k_sem_give(&m_sbar_lock);
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
		break;
	}
	case LIB_EVENT_NET_WIFI_STARTED:
	case LIB_EVENT_NET_WIFI_CONNECTED:
	{
		m_wifi_stat = true;
		if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
			return;
		if (ico_wifi != NULL)
			lv_obj_set_style_img_opa(ico_wifi, LV_OPA_COVER, LV_PART_MAIN);
		break;
	}
	case LIB_EVENT_NET_WIFI_STOPPED:
	case LIB_EVENT_NET_WIFI_DISCONNECTED:
	{
		m_wifi_stat = false;
		if (c20x_screen_manager_curr_screen_get() != C20X_SCREEN_DASHBOARD)
			return;
		if (ico_wifi != NULL)
			lv_obj_set_style_img_opa(ico_wifi, LV_OPA_TRANSP, LV_PART_MAIN);
		break;
	}
	case LIB_EVENT_NET_HOTSPOT_STARTED:
	{
		m_hp_stat = true;
		break;
	}
	case LIB_EVENT_NET_HOTSPOT_STOPPED:
	{
		m_hp_stat = false;
		break;
	}
	case LIB_EVENT_NET_BT_STARTED:
	{
		m_bt_stat = true;
		if (ico_blue != NULL)
			lv_obj_set_style_img_opa(ico_blue, LV_OPA_COVER, LV_PART_MAIN);
		break;
	}
	case LIB_EVENT_NET_BT_STOPPED:
	{
		m_bt_stat = false;
		if (ico_blue != NULL)
			lv_obj_set_style_img_opa(ico_blue, LV_OPA_TRANSP, LV_PART_MAIN);
		break;
	}
	default:
		break;
	}
}

int c20x_screen_statusbar_init(uint32_t x_width, uint32_t y_height)
{
	int ret = 0;

//	status_bar_style_create();

	status_bar_setup(x_width, y_height);

	return ret;
}

void c20x_screen_statusbar_deinit()
{
	lv_obj_del(bg);
//	datetime = NULL;
//	ico_bat = NULL;
//	ico_wifi = NULL;
//	ico_blue = NULL;
}

/* IMPORTANT: this function should be called only once */
void c20x_screen_statusbar_init_onetime()
{
	lib_events_callback_add(&m_cb_settings_changed, app_event_handler, LIB_EVENT_SETTINGS_CHANGED);
	lib_events_callback_add(&m_evnt_cb_wifi_started, app_event_handler, LIB_EVENT_NET_WIFI_STARTED);
	lib_events_callback_add(&m_evnt_cb_wifi_stopped, app_event_handler, LIB_EVENT_NET_WIFI_STOPPED);
	lib_events_callback_add(&m_evnt_cb_bt_started, app_event_handler, LIB_EVENT_NET_BT_STARTED);
	lib_events_callback_add(&m_evnt_cb_bt_stopped, app_event_handler, LIB_EVENT_NET_BT_STOPPED);
}
/*
 *
 */

