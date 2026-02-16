/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 30-Nov-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);


#include "lib_events/lib_events.h"
#include "c20x_screen_config.h"
#include "c20x_screen_splash.h"
#include "c20x_screen_settings.h"
#include "c20x_screen_switch.h"
#include "c20x_screen_statusbar.h"
#include "c20x_screen_dashboard.h"
#include "c20x_screen_manager.h"
#include "c20x_screen_spinbox.h"
#include "c20x_screen_roller.h"
#include "c20x_screen_datetime_roller.h"
#include "c20x_screen_common_styles.h"
#include "c20x_screen_alert_msg.h"
#include "c20x_screen_power_key.h"
#include "app_display/c20x_screens/c20x_screen_settings_extra_func.h"
#include "app_display/app_display.h"

#define APP_THREAD_STACK_SIZE_SCREEN_MGR	(2*1024)
#define APP_THREAD_PRIO_SCREEN_MGR			10

static lv_group_t * m_lvgl_group = NULL;
static uint8_t m_curr_screen = C20X_SCREEN_NONE;
static uint8_t m_next_screen = C20X_SCREEN_NONE;
static bool m_is_screen_changed = false;

/* Display thread static variables */
K_THREAD_STACK_DEFINE(screen_mgr_stack, APP_THREAD_STACK_SIZE_SCREEN_MGR);
struct k_thread screen_mgr_thread_data;
k_tid_t screen_mgr_tid;

static struct k_sem m_screen_change_lock;

static struct lib_events_callback m_cb_boot_done;

static void clear_whole_screen() {
	lv_obj_clean(lv_scr_act());
}

static void screen_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch(event) {
	case LIB_EVENT_SYSTEM_BOOTING_COMPLETE:
//		m_curr_screen = C20X_SCREEN_DASHBOARD;
//		m_curr_screen = C20X_SCREEN_SETTINGS;

		k_sem_take(&m_screen_change_lock, K_FOREVER);
		m_next_screen = C20X_SCREEN_DASHBOARD;
		m_is_screen_changed = true;
		k_sem_give(&m_screen_change_lock);
		break;
	default:
		break;
	}
}

static void screen_change_event_cb(uint8_t screen_id) {
	k_sem_take(&m_screen_change_lock, K_FOREVER);
	m_next_screen = screen_id;
	m_is_screen_changed = true;
	k_sem_give(&m_screen_change_lock);
}

static void load_screen(uint8_t screen_id, uint8_t prev_screen_id) {
	switch(screen_id) {
	case C20X_SCREEN_SPLASH:
		break;
	case C20X_SCREEN_DASHBOARD:
	{
		lv_group_remove_all_objs(m_lvgl_group);
		clear_whole_screen();
		c20x_screen_statusbar_init(APP_DISPLAY_WIDTH_PX, C20X_SCREEN_SBAR_HEIGHT_PX);
		c20x_screen_dashboard_init(
				C20X_SCREEN_DASH_WIDTH_PX,
				C20X_SCREEN_DASH_HEIGHT_PX,
				C20X_SCREEN_SBAR_HEIGHT_PX,
				m_lvgl_group);
		c20x_screen_db_cb_set(screen_change_event_cb);
		break;
	}
	case C20X_SCREEN_SETTINGS:
	{
		lv_group_remove_all_objs(m_lvgl_group);
		clear_whole_screen();
		c20x_screen_settings_init(
					C20X_SCREEN_SETTINGS_WIDTH_PX,
					C20X_SCREEN_SETTINGS_HEIGHT_PX,
					C20X_SCREEN_SETTINGS_LABEL_HEIGHT_PX,
					m_lvgl_group);
		c20x_screen_settings_cb_set(screen_change_event_cb);
		c20x_screen_settings_start();
		break;
	}
	case C20X_SCREEN_POWER_OFF:
	{
		lv_group_remove_all_objs(m_lvgl_group);
		clear_whole_screen();
		c20x_screen_poweroff_cb_set(screen_change_event_cb);
		c20x_screen_poweroff_load(prev_screen_id, m_lvgl_group);
		break;
	}
	case C20X_TFT_TEST:
		break;
	default:
		break;
	}
}

static void screen_mgr_thread(void *p1, void *p2, void *p3) {
	uint32_t sbar_reload_cnt=0, db_reload_cnt=0, slpdly_ms=C20X_SCREEN_MGR_THREAD_SLEEP_MSEC;
	LOG_INF("screen_mgr_thread started");
	while (1) {
		if (m_is_screen_changed) {
			k_sem_take(&m_screen_change_lock, K_FOREVER);
			m_is_screen_changed = false;
			k_sem_give(&m_screen_change_lock);

			switch(m_curr_screen) {
			case C20X_SCREEN_SPLASH:
			{
				/* clean up the current screen */

				/* load the next screen */
				load_screen(m_next_screen, C20X_SCREEN_SPLASH);

				k_sem_take(&m_screen_change_lock, K_FOREVER);
				m_curr_screen = m_next_screen;
				m_next_screen = C20X_SCREEN_NONE;
				k_sem_give(&m_screen_change_lock);
				break;
			}
			case C20X_SCREEN_DASHBOARD:
			{
				/* clean up the current screen */
				c20x_screen_dashboard_deinit();
				c20x_screen_statusbar_deinit();

				/* load the next screen */
				load_screen(m_next_screen, C20X_SCREEN_DASHBOARD);

				k_sem_take(&m_screen_change_lock, K_FOREVER);
				m_curr_screen = m_next_screen;
				m_next_screen = C20X_SCREEN_NONE;
				k_sem_give(&m_screen_change_lock);

				break;
			}
			case C20X_SCREEN_SETTINGS:
			{
				/* clean up the current screen */
				c20x_screen_settings_deinit();

				/* load the next screen */
				load_screen(m_next_screen, C20X_SCREEN_SETTINGS);

				k_sem_take(&m_screen_change_lock, K_FOREVER);
				m_curr_screen = m_next_screen;
				m_next_screen = C20X_SCREEN_NONE;
				k_sem_give(&m_screen_change_lock);

				break;
			}
			case C20X_SCREEN_ALERT_MSG:
			{
				/* load the next screen */
//				load_screen(m_next_screen, C20X_SCREEN_ALERT_MSG);

				k_sem_take(&m_screen_change_lock, K_FOREVER);
				m_curr_screen = m_next_screen;
				m_next_screen = C20X_SCREEN_NONE;
				k_sem_give(&m_screen_change_lock);
				break;
			}
			case C20X_TFT_TEST:
			{
				lv_obj_t *label = lv_label_create(lv_scr_act());
				lv_label_set_recolor(label, true);
				lv_label_set_text(label, "#0000ff TFT Test: #");
				lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
				break;
			}
			case C20X_SCREEN_POWER_OFF:
			{
				/* clean up the current screen */
				c20x_screen_poweroff_deinit();

				/* load the next screen */
				load_screen(m_next_screen, C20X_SCREEN_POWER_OFF);

				k_sem_take(&m_screen_change_lock, K_FOREVER);
				m_curr_screen = m_next_screen;
				m_next_screen = C20X_SCREEN_NONE;
				k_sem_give(&m_screen_change_lock);

				break;
			}
			default:
				break;
			}
		}

		if (++sbar_reload_cnt >= ((C20X_SCREEN_MGR_SBAR_RELOAD_SEC*1000) / slpdly_ms)) {
			sbar_reload_cnt=0;
			c20x_screen_statusbar_reload();

#if 0
			/* temporary */
//			k_sleep(K_MSEC(1000));
			m_curr_screen = C20X_SCREEN_ALERT_MSG;
			c20x_screen_alert_msg_cb_set(screen_change_event_cb);
			c20x_screen_alert_msg_show(NULL, 0, NULL, "Info", "Hello, this is a message",	false, 5, C20X_SCREEN_DASHBOARD, NULL);
//			k_sleep(K_MSEC(1000));
#endif
		}

		if (++db_reload_cnt >= ((C20X_SCREEN_MGR_DB_RELOAD_MSEC) / slpdly_ms)) {
			db_reload_cnt=0;
			c20x_screen_dashboard_reload();
		}

		k_sleep(K_MSEC(slpdly_ms));
	}
}

C20X_SCREENS c20x_screen_manager_curr_screen_get()
{
	return m_curr_screen;
}

static void special_key_handler(uint32_t key)
{
	switch (key) {
	case APP_DISPLAY_KEY_POWER:
	{
		/* detect key press type (normal / long) */
		c20x_screen_power_key_detect(key);
	}
		break;
	case APP_DISPLAY_KEY_HOME:
		break;
	case APP_DISPLAY_KEY_MIC:
		break;
	case APP_DISPLAY_KEY_VOL_UP:
		break;
	case APP_DISPLAY_KEY_VOL_DOWN:
		break;
	case APP_DISPLAY_KEY_MUTE:
		break;
	}
}

void c20x_screen_manager_init(lv_group_t * lvgl_grp) {
	int ret = 0;

	k_sem_init(&m_screen_change_lock, 1, 1);

	ret = lib_events_callback_add(&m_cb_boot_done, screen_event_handler, LIB_EVENT_SYSTEM_BOOTING_COMPLETE);

	/* store the lvgl group object to be used by each screen for navigation */
	m_lvgl_group = lvgl_grp;

	/* Initialize all styles */
	c20x_screen_cmn_styles_init();
	c20x_screen_dashboard_styles_init();
	c20x_screen_settings_styles_init();
//	c20x_screen_switch_styles_init();
//	c20x_screen_spinbox_styles_init();
	c20x_screen_roller_styles_init();
	c20x_screen_alert_msg_styles_init();

	/* Other initializations */
	c20x_screen_statusbar_init_onetime();
	c20x_screen_dashboard_init_onetime();
	c20x_screen_settings_extra_func_init();

	/* Splash screen */
	m_curr_screen = C20X_SCREEN_SPLASH;
//	c20x_screen_splash_start();

	/* Special keys */
	c20x_screen_power_key_init();
	c20x_screen_poweroff_cb_set(screen_change_event_cb);
	app_display_spl_key_cb_set(special_key_handler);

	/* Start screen manager thread */
	screen_mgr_tid = k_thread_create(&screen_mgr_thread_data, screen_mgr_stack,
			K_THREAD_STACK_SIZEOF(screen_mgr_stack), screen_mgr_thread, NULL, NULL, NULL,
			APP_THREAD_PRIO_SCREEN_MGR, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(screen_mgr_tid, "scrn");
#endif
}
