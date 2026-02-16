/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 2-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/sys/timeutil.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_time/app_time.h"

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_display_data.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"
#include "c20x_screen_roller.h"
#include "c20x_screen_datetime_roller.h"


#define ROLLER_DATA_DAY		"1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n"				\
							"11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n"		\
							"21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n"		\
							"31"

#define ROLLER_DATA_MON		"Jan\nFeb\nMar\nApr\nMay\nJun\n"		\
							"Jul\nAug\nSep\nOct\nNov\nDec"

#define ROLLER_DATA_YEAR	// make data in runtime based on range configured

#define ROLLER_DATA_HOUR	"00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"	\
							"12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23"

#define ROLLER_DATA_MIN		ROLLER_DATA_HOUR							\
							"\n24\n25\n26\n27\n28\n29\n30\n"				\
							"31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n"	\
							"41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n"	\
							"51\n52\n53\n54\n55\n56\n57\n58\n59"

#define ROLLER_DATA_SEC		ROLLER_DATA_MIN


/* static variables */
static const char *m_settings_save_path = NULL;
static app_display_key_cb m_prev_screen = NULL;
static uint8_t m_screen_date_or_time = 0;
static uint8_t m_data_saved = 0;	// page can be exited only if this variable is 3, this means
									// the user has pressed enter on all 3 rollers and
									// we have saved the data
static lv_obj_t * m_screen_bg;
static struct tm m_time;

//static void save_datetime(const char* path, struct tm* dt)
//{
//   	time_t ts;
//   	ts = timeutil_timegm(dt);
//   	app_settings_save_single(path, &ts, sizeof(time_t), true);
//}

static void event_handler_day_hr(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
//        char buf[32];
//        lv_roller_get_selected_str(obj, buf, sizeof(buf));
//        LOG_INF("Selected day: %s\n", buf);

        uint16_t idx = lv_roller_get_selected(obj);
        if (m_screen_date_or_time == C20X_SCREEN_DATE) {
        	m_time.tm_mday = idx + 1;	// day starts from 1
        	LOG_INF("day = %d", m_time.tm_mday);
        } else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
        	m_time.tm_hour = idx;		// hour starts from 0
        	LOG_INF("hour = %d", m_time.tm_hour);
        }

        /* save data */
        if (e->user_data != NULL) {
//        	save_datetime((const char *)e->user_data, &m_time);
        	m_data_saved = 1;	// day hour data saved
        }

        /* focus to next item*/
        lv_group_focus_next(app_display_lvgl_group_instance_get());
    }
}

static void event_handler_mon_min(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
//        char buf[32];
//        lv_roller_get_selected_str(obj, buf, sizeof(buf));
//        LOG_INF("Selected day: %s\n", buf);

        uint16_t idx = lv_roller_get_selected(obj);
        if (m_screen_date_or_time == C20X_SCREEN_DATE) {
        	m_time.tm_mon = idx + 1;	// mon starts from 1
        	LOG_INF("mon = %d", m_time.tm_mon);
        } else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
        	m_time.tm_min = idx;		// min starts from 0
        	LOG_INF("min = %d", m_time.tm_min);
        }

        /* save data */
        if (e->user_data != NULL) {
//        	save_datetime((const char *)e->user_data, &m_time);
        	m_data_saved = 2;	// mon min data saved
        }

        /* focus to next item*/
        lv_group_focus_next(app_display_lvgl_group_instance_get());
    }
}

static void event_handler_year_sec(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
//        char buf[32];
//        lv_roller_get_selected_str(obj, buf, sizeof(buf));
//        LOG_INF("Selected day: %s\n", buf);

        uint16_t idx = lv_roller_get_selected(obj);
        if (m_screen_date_or_time == C20X_SCREEN_DATE) {
        	m_time.tm_year = (C20X_SCREEN_DATETIME_YEAR_MIN + idx) - 1900;	// year
        	LOG_INF("year = %d", m_time.tm_year);
        } else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
        	m_time.tm_sec = idx;		// sec starts from 0
        	LOG_INF("sec = %d", idx);
        }

        /* save data */
        if (e->user_data != NULL) {
//        	save_datetime((const char *)e->user_data, &m_time);
        	app_time_value_set(&m_time);
        	m_data_saved = 3;	// mon min data saved
        }

        /* focus to next item*/
        lv_group_focus_next(app_display_lvgl_group_instance_get());
    }
}

static void extra_btn_handler(uint32_t key) {
	if (m_data_saved != 3)	// all 3 data needs to be saved before exiting
		return;
	switch (key) {
	case LV_KEY_ESC: {
		/* going to the previous screen, so delete the objects */
		lv_obj_del(m_screen_bg);

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

static void setup_labels(lv_obj_t *parent, const char *display_name)
{
	/* page top */
	lv_obj_t *roller_label = lv_label_create(parent);
	lv_obj_set_style_pad_all(roller_label, 0, LV_PART_MAIN);
	lv_label_set_text(roller_label, display_name);
	lv_obj_align(roller_label, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

	/* roller left */
	roller_label = lv_label_create(parent);
	lv_obj_set_style_pad_all(roller_label, 0, LV_PART_MAIN);
	if (m_screen_date_or_time == C20X_SCREEN_DATE) {
		lv_label_set_text(roller_label, "Day");
	} else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
		lv_label_set_text(roller_label, "Hour");
	}
	lv_obj_align(roller_label, LV_ALIGN_TOP_LEFT, 0, 0);

	/* roller mid */
	roller_label = lv_label_create(parent);
	lv_obj_set_style_pad_all(roller_label, 0, LV_PART_MAIN);
	if (m_screen_date_or_time == C20X_SCREEN_DATE) {
		lv_label_set_text(roller_label, "Mon");
	} else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
		lv_label_set_text(roller_label, "Min");
	}
	lv_obj_align(roller_label, LV_ALIGN_TOP_MID, 0, 0);

	/* roller right */
	roller_label = lv_label_create(parent);
	lv_obj_set_style_pad_all(roller_label, 0, LV_PART_MAIN);
	if (m_screen_date_or_time == C20X_SCREEN_DATE) {
		lv_label_set_text(roller_label, "Year");
	} else if (m_screen_date_or_time == C20X_SCREEN_TIME) {
		lv_label_set_text(roller_label, "Sec");
	}
	lv_obj_align(roller_label, LV_ALIGN_TOP_RIGHT, 0, 0);
}

int c20x_screen_datetime_roller_init(	const char *display_name,
										uint8_t screen_date_or_time,
										uint16_t x_width, uint16_t y_height,
										uint16_t top_label_height,
										struct tm *time,
										const char *settings_save_path,
										app_display_key_cb prev_screen_cb

		)
{
	int ret = 0;
	m_data_saved=0;

	/* copy essentials */
	m_settings_save_path = settings_save_path;
	m_prev_screen = prev_screen_cb;
	m_screen_date_or_time = screen_date_or_time;

//	memcpy(&m_time, gmtime(&curr_unix_time), sizeof (struct tm));
	memcpy(&m_time, time, sizeof (struct tm));
	LOG_INF("obtained date: %d / %d / %d", m_time.tm_mday, m_time.tm_mon, m_time.tm_year+1900);
	LOG_INF("obtained time: %d : %d : %d", m_time.tm_hour, m_time.tm_min, m_time.tm_sec);

	/* setup screen background */
	m_screen_bg = lv_obj_create(lv_scr_act());
	lv_obj_set_width(m_screen_bg, x_width);
	lv_obj_set_height(m_screen_bg, y_height);
	lv_obj_set_style_bg_img_opa(m_screen_bg, LV_OPA_COVER, LV_PART_MAIN);
	lv_obj_set_style_radius(m_screen_bg, 0, LV_PART_MAIN);
	lv_obj_set_style_border_color(m_screen_bg, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_border_width(m_screen_bg, 1, LV_PART_MAIN);
	lv_obj_set_style_border_side(m_screen_bg, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
	lv_obj_set_style_pad_all(m_screen_bg, 0, LV_PART_MAIN);
	lv_obj_align(m_screen_bg, LV_ALIGN_TOP_MID, 0, top_label_height);

	/* set labels */
	c20x_screen_settings_text_set("");
	setup_labels(m_screen_bg, display_name);

    /* remove all other items from group */
    lv_group_remove_all_objs(app_display_lvgl_group_instance_get());

	/* ****************
	 * roller day hour
	 * ****************
	 * */
    lv_obj_t * roller_day_hr = lv_roller_create(m_screen_bg);
    lv_obj_set_width(roller_day_hr, C20X_SCREEN_DATETIME_ROLLER_WIDTH_PX);
//    lv_obj_set_height(roller_day_hr, 50);
    if (screen_date_or_time == C20X_SCREEN_DATE) {
    	lv_roller_set_options(roller_day_hr, ROLLER_DATA_DAY, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_day_hr, m_time.tm_mday-1, LV_ANIM_OFF);
    } else if (screen_date_or_time == C20X_SCREEN_TIME) {
    	lv_roller_set_options(roller_day_hr, ROLLER_DATA_HOUR, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_day_hr, m_time.tm_hour, LV_ANIM_OFF);
    }
    lv_roller_set_visible_row_count(roller_day_hr, 2);

    c20x_screen_roller_styles_set(roller_day_hr);

    lv_obj_align(roller_day_hr, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_event_cb(roller_day_hr, event_handler_day_hr, LV_EVENT_ALL, (void*)settings_save_path);

    lv_group_add_obj(app_display_lvgl_group_instance_get(), roller_day_hr);

	/* ****************
	 * roller month min
	 * ****************
	 * */
    lv_obj_t * roller_mon_min = lv_roller_create(m_screen_bg);
    lv_obj_set_width(roller_mon_min, C20X_SCREEN_DATETIME_ROLLER_WIDTH_PX);
    if (screen_date_or_time == C20X_SCREEN_DATE) {
    	lv_roller_set_options(roller_mon_min, ROLLER_DATA_MON, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_mon_min, m_time.tm_mon-1, LV_ANIM_OFF);
    } else if (screen_date_or_time == C20X_SCREEN_TIME) {
    	lv_roller_set_options(roller_mon_min, ROLLER_DATA_MIN, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_mon_min, m_time.tm_min, LV_ANIM_OFF);
    }

    c20x_screen_roller_styles_set(roller_mon_min);

    lv_roller_set_visible_row_count(roller_mon_min, C20X_SCREEN_DATETIME_ROLLER_VISIBLE);
    lv_obj_align(roller_mon_min, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(roller_mon_min, event_handler_mon_min, LV_EVENT_ALL, (void*)settings_save_path);

    lv_group_add_obj(app_display_lvgl_group_instance_get(), roller_mon_min);

	/* ****************
	 * roller year sec
	 * ****************
	 * */
    lv_obj_t * roller_year_sec = lv_roller_create(m_screen_bg);
    lv_obj_set_width(roller_year_sec, C20X_SCREEN_DATETIME_ROLLER_WIDTH_PX);

    char year[100] = {0x00};
    int i=0, r=0;
    for (int j=C20X_SCREEN_DATETIME_YEAR_MIN; j<C20X_SCREEN_DATETIME_YEAR_MAX; j++) {
    	r = sprintf(year+i, "%d\n", j);
    	i += r;
    }
    year[i-1] = '\0';	// delete the last \n
    printk("year len = %d", strlen(year));
    printk("year = %s", year);

    if (screen_date_or_time == C20X_SCREEN_DATE) {
    	lv_roller_set_options(roller_year_sec, year, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_year_sec, (m_time.tm_year + 1900)-C20X_SCREEN_DATETIME_YEAR_MIN, LV_ANIM_OFF);
    } else if (screen_date_or_time == C20X_SCREEN_TIME) {
    	lv_roller_set_options(roller_year_sec, ROLLER_DATA_SEC, LV_ROLLER_MODE_INFINITE);
    	lv_roller_set_selected(roller_year_sec, m_time.tm_sec, LV_ANIM_OFF);
    }

    c20x_screen_roller_styles_set(roller_year_sec);

    lv_roller_set_visible_row_count(roller_year_sec, C20X_SCREEN_DATETIME_ROLLER_VISIBLE);
    lv_obj_align(roller_year_sec, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(roller_year_sec, event_handler_year_sec, LV_EVENT_ALL, (void*)settings_save_path);

    lv_group_add_obj(app_display_lvgl_group_instance_get(), roller_year_sec);


    /* focus on first item */
    lv_group_focus_obj(roller_day_hr);

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(extra_btn_handler);

	return ret;
}

