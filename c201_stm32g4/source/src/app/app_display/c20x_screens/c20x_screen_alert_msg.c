/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 27-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <zephyr/kernel.h>
#include <lvgl.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
//#include "app_settings/app_settings_display_data.h"

#include "c20x_screen_config.h"
#include "c20x_screen_manager.h"
#include "c20x_screen_alert_msg.h"

#define BTN_MATRIX_HEIGHT		C20X_SCREEN_ALERT_MSG_BTN_MATRIX_HEIGHT_PX

static void msg_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(m_msg_timer, msg_timer_handler, NULL);

static lv_style_t m_mbox_style_def;			// mbox objects other than button matrix
static lv_style_t m_mboxbtnmat_style_def;	// mbox button matrix style
static lv_style_t m_mboxbtn_style_def;		// mbox button matrix's button default
static lv_style_t m_mboxbtn_style_fo;		// mbox button matrix's button focussed
static lv_style_t m_mboxbtn_style_pr;		// mbox button matrix's button pressed

static lv_obj_t * m_mbox = NULL;
static uint8_t m_prev_screen_id = C20X_SCREEN_NONE;
static c20x_screen_change_cb m_screen_change = NULL;
static alert_msg_caller_cb m_user_cb = NULL;
static void *m_user_data = NULL;

static void msg_timer_handler(struct k_timer *timer) {
	k_timer_stop(timer);
	lv_msgbox_close(m_mbox);
	m_mbox = NULL;

	if (m_user_cb != NULL) {
		m_user_cb(NULL, m_user_data);
	}
	if (c20x_screen_manager_curr_screen_get() == C20X_SCREEN_ALERT_MSG)
		m_screen_change(m_prev_screen_id);
}

static void event_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_current_target(e);
//    LV_UNUSED(obj);
    const char *txt = lv_msgbox_get_active_btn_text(obj);
    LOG_INF("Button %s clicked", txt);

    lv_msgbox_close(m_mbox);
    m_mbox = NULL;

	if (m_user_cb != NULL) {
		m_user_cb(txt, lv_event_get_user_data(e));
	}

    if (c20x_screen_manager_curr_screen_get() == C20X_SCREEN_ALERT_MSG)
    	m_screen_change(m_prev_screen_id);
}

static void msgbox_style_set(lv_obj_t * obj)
{
#if (CONFIG_C20X_SCREENS_OLED)
	/* default */
	lv_style_set_border_opa(&m_mbox_style_def, LV_OPA_COVER);
	lv_style_set_radius(&m_mbox_style_def, 0);
    lv_style_set_border_width(&m_mbox_style_def, 1);
    lv_style_set_border_color(&m_mbox_style_def, lv_color_white());
    lv_style_set_pad_all(&m_mbox_style_def, 0);
    lv_style_set_pad_gap(&m_mbox_style_def, 0);

    lv_obj_add_style(obj, &m_mbox_style_def, LV_PART_MAIN | LV_STATE_DEFAULT);
#endif
}

static void msgbox_btn_style_set(lv_obj_t * btn)
{
#if (CONFIG_C20X_SCREENS_OLED)
	/* button matrix main */
	lv_style_set_pad_top(&m_mboxbtnmat_style_def, 1);
	lv_style_set_pad_bottom(&m_mboxbtnmat_style_def, 1);

	/* default */
	lv_style_set_bg_opa(&m_mboxbtn_style_def, LV_OPA_COVER);

	/* focused */
	lv_style_set_bg_color(&m_mboxbtn_style_fo, lv_color_white());
	lv_style_set_text_color(&m_mboxbtn_style_fo, lv_color_black());

	/* pressed */
	lv_style_set_bg_color(&m_mboxbtn_style_pr, lv_color_black());
	lv_style_set_text_color(&m_mboxbtn_style_pr, lv_color_white());

	lv_obj_add_style(btn, &m_mboxbtnmat_style_def, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(btn, &m_mboxbtn_style_def, LV_PART_ITEMS | LV_STATE_DEFAULT);
	lv_obj_add_style(btn, &m_mboxbtn_style_fo, LV_PART_ITEMS | LV_STATE_FOCUS_KEY);
	lv_obj_add_style(btn, &m_mboxbtn_style_pr, LV_PART_ITEMS | LV_STATE_PRESSED);
#endif
}

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
								)
{
	int ret = 0;

	if (m_mbox != NULL)
		lv_msgbox_close(m_mbox);

	m_prev_screen_id = prev_screen_id;
	m_user_cb = user_cb;
	m_user_data = user_data;

	lv_obj_t * mbox1 = lv_msgbox_create(parent, title, text, btns, close_btn);
    m_mbox = mbox1;

    msgbox_style_set(mbox1);

    lv_obj_t * mbox_title = lv_msgbox_get_title(mbox1);
    msgbox_style_set(mbox_title);

    lv_obj_t * mbox_text = lv_msgbox_get_text(mbox1);
    msgbox_style_set(mbox_text);

    lv_obj_t * btnmat = lv_msgbox_get_btns(mbox1);
    if (btnmat != NULL) {
    	msgbox_btn_style_set(btnmat);
    	lv_obj_set_height(btnmat, BTN_MATRIX_HEIGHT);
    }

    lv_obj_center(mbox1);


    if (timeout_sec == 0)	/* if no timeout provided add an event */
    	lv_obj_add_event_cb(mbox1, event_cb, LV_EVENT_VALUE_CHANGED, user_data);
    else {
    	k_timer_start(&m_msg_timer, K_SECONDS(timeout_sec), K_SECONDS(timeout_sec));
    }


    if (btnmat != NULL) {
    	/* add group */
    	lv_group_add_obj(app_display_lvgl_group_instance_get(), btnmat);
    	lv_group_focus_obj(btnmat);
    }

    return ret;
}

void c20x_screen_alert_msg_cb_set(c20x_screen_change_cb screen_change_cb)
{
	m_screen_change = screen_change_cb;
}


/* IMPORTANT: this function should be called only once */
void c20x_screen_alert_msg_styles_init()
{
	lv_style_init(&m_mbox_style_def);
	lv_style_init(&m_mboxbtnmat_style_def);
	lv_style_init(&m_mboxbtn_style_def);
	lv_style_init(&m_mboxbtn_style_fo);
	lv_style_init(&m_mboxbtn_style_pr);
}
