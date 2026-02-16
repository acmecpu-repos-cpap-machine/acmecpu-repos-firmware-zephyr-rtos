/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 09-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_display/app_display_menu.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"
#include "c20x_screen_roller.h"
#include "c20x_screen_dynamic_roller.h"

#define SCRN_WIDTH 		C20X_SCREEN_ROLLER_SCREEN_WIDTH_PX
#define PADDING_SIDE	C20x_SCREEN_ROLLER_PADDING_SIDE
#define PADDING_BOTTOM	C20x_SCREEN_ROLLER_PADDING_BOTTOM

struct roller_local {
	lv_obj_t *roller_obj;
	char *option_str;		// DMA variable, must free
};
static struct roller_local *m_prl;	// DMA variable, must free

static const char *m_disp_name = NULL;
static app_display_key_cb m_prev_screen = NULL;
//static settings_extra_func m_extra_func = NULL;
static menu_extra_func m_extra_func = NULL;
static uint8_t m_roller_max = 0;
static uint8_t m_curr_roller = 1;
static int16_t m_range_min_idx = -1;
static int16_t m_range_max_idx = -1;

static void dealloc_screen(uint8_t count) {
	for (int i=0; i<count; i++) {
		struct roller_local* prl = &m_prl[i];
		lv_obj_del(prl->roller_obj);
		free(prl->option_str);
	}
	free(m_prl);
}

static void roller_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
    	/* get value */
    	struct gui_param_metadata *mdata = (struct gui_param_metadata *)e->user_data;

    	int16_t idx = lv_roller_get_selected(obj);
//    	if (mdata->has_range) {
    	if (m_range_min_idx >= 0) {
    		if ((idx = idx + m_range_min_idx) > m_range_max_idx)
    			idx = m_range_max_idx;
    	}
    	mdata->selected_idx = idx;

    	struct setting_value val;// = mdata->data[idx].val;
    	bool dup = false;

        /* check and save value */
        app_settings_load_single(mdata->settings_path, &val, sizeof(struct setting_value));
        if ((val.val1 == mdata->options->op_val[idx].val.val1) && (val.val2 == mdata->options->op_val[idx].val.val2)) {
        	/* duplicate value, do nothing */
        	dup = true;
        } else {
        	val = mdata->options->op_val[idx].val;
        	LOG_INF("idx: %d, val1 = %d, val2 = %d", idx, val.val1, val.val2);
        	if (app_settings_save_single(mdata->settings_path, &val, sizeof (struct setting_value), true) != 0 ) {
        		LOG_ERR("failed to save %s", mdata->settings_path);
        		return;
        	}
        }

        /* check exit condition */
        if (m_curr_roller++ == m_roller_max) {
        	/* deallocate memory */
        	dealloc_screen(m_roller_max);
//        	for (int i=0; i<m_roller_max; i++) {
//        		struct roller_local* prl = &m_prl[i];
//        		lv_obj_del(prl->roller_obj);
//        		free(prl->option_str);
//        	}
//        	free(m_prl);

        	if ((m_extra_func != NULL) && !dup) {
        		if ( m_extra_func(m_disp_name, mdata->settings_path, m_prev_screen) < 0 ) {
        			LOG_ERR("cannot load extra function screens");
        			m_prev_screen(LV_KEY_ESC);
        		}

        	} else {
        		/* exit screen - call the previous screen button handler to load it */
        		m_prev_screen(LV_KEY_ESC);	// TODO: for now, ESC means we are returning from a screen
        	}
    		return;
        }

        /* focus to next item */
        lv_group_focus_next(app_display_lvgl_group_instance_get());
    }
}

static void extra_btn_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ESC: {
		/* going to the previous screen, so delete the objects */
    	/* deallocate memory */
    	dealloc_screen(m_roller_max);

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

static size_t get_option_strlen(struct setting_value_options *opt, int start, int end)
{
	size_t len=0, option_strlen=0;
//	int start=0;
//	int end = mdata->num_items;

	for (int i=start; i < end; i++) {
		len = strlen(opt->op_val[i].key);	// get len of a key
		len = len + 1;				// add 1 byte for delimiter
		option_strlen += len;
	}
	option_strlen--;				// the last key does not need a delimiter
	return option_strlen;
}

int c20x_screen_dynamic_roller_init(
							const char *display_name,
							struct gui_param_metadata *mdata,
							uint8_t roller_count,
							uint8_t visible_row_count,
							int16_t range_min_idx, int16_t range_max_idx,
							app_display_key_cb prev_screen_cb,
							menu_extra_func extra_func
						)
{
	int ret = 0;
	int opstr_len=0, offset=0;

	/* copy essentials */
	m_roller_max = roller_count;
	m_curr_roller = 1;
	m_disp_name = display_name;
	m_prev_screen = prev_screen_cb;
	m_range_min_idx = range_min_idx;
	m_range_max_idx = range_max_idx;
	m_extra_func = extra_func;

	/* set the param name */
	c20x_screen_settings_text_set(display_name);

    /* remove all other items from group */
    lv_group_remove_all_objs(app_display_lvgl_group_instance_get());

	/* allocate memory for roller objects and option strigns */
	m_prl = (struct roller_local*)calloc(roller_count, sizeof(struct roller_local));
	if (m_prl == NULL) {
		LOG_ERR("calloc failed, aborting!");
		return -ENOMEM;
	}

	/* calculate width and padding */
	uint16_t total_w = (SCRN_WIDTH - PADDING_SIDE*2);	// width of roller space
	uint8_t num_gaps = roller_count-1;					// number of gaps to pad between the rollers
	uint8_t mid_pad_total = SCRN_WIDTH - total_w;		// total padding pixel
	uint8_t mid_pad_each=0; if (num_gaps > 0)	mid_pad_each = mid_pad_total / num_gaps;	// padding between 2 rollers in pixel
	uint16_t roller_w = (total_w - mid_pad_total) / roller_count;	// width of each roller

	for (int i=0; i<roller_count; i++) {
		struct roller_local* prl = &m_prl[i];
		struct gui_param_metadata *rd = &mdata[i];
		int16_t sel_idx = rd->selected_idx;

		/* create roller */
		prl->roller_obj = lv_roller_create(lv_scr_act());

		/* make & set options */
		int start=0, end=0;
//		if (rd->has_range) {
		if (range_min_idx >= 0) {
			start = range_min_idx; end = range_max_idx;
			if ((sel_idx = sel_idx-range_min_idx) < range_min_idx)
				sel_idx = range_min_idx;
		}
		else {
			start=0; end = rd->options->num_options;
		}
		opstr_len = get_option_strlen(rd->options, start, end);
//		prl->option_str = (char *)calloc(1, opstr_len+1);	// 1 extra byte for NULL
		prl->option_str = (char *)malloc(opstr_len+1);	// 1 extra byte for NULL
		if (prl->option_str == NULL) {
			LOG_ERR("calloc failed, aborting!");
			return -ENOMEM;
		}
		memset(prl->option_str, 0x00, opstr_len+1);

		offset=0;
		for (int opcnt = start; opcnt < end; opcnt++) {
			int keylen = strlen(rd->options->op_val[opcnt].key);
			strncpy(prl->option_str + offset, rd->options->op_val[opcnt].key, keylen);
			offset += keylen;
//			strncpy(prl->option_str + offset, "\n", 1);
			*(prl->option_str + offset) = '\n';
			offset += 1;
		}
//		strncpy(prl->option_str + offset -1, "\0", 1);
		*(prl->option_str + offset -1) = '\0';
//		LOG_INF("options: %s", prl->option_str);
		lv_roller_set_options(prl->roller_obj, prl->option_str, LV_ROLLER_MODE_INFINITE);
		lv_roller_set_selected(prl->roller_obj, sel_idx, LV_ANIM_ON);

		/* set size, styles and alignment */
		lv_obj_set_width(prl->roller_obj, roller_w);
		c20x_screen_roller_styles_set(prl->roller_obj);
//		if (visible_row_count < 3)
//			lv_obj_set_height(prl->roller_obj, 40);
		lv_roller_set_visible_row_count(prl->roller_obj, visible_row_count);
		if (roller_count == 1)
			lv_obj_align(prl->roller_obj, LV_ALIGN_BOTTOM_MID, 0, PADDING_BOTTOM);
		else {
			uint16_t x_off;
			if (i == 0)	// side padding on the first roller only
				x_off = PADDING_SIDE;
			else
				x_off = PADDING_SIDE + (i * roller_w) + (i * mid_pad_each);

			lv_obj_align(prl->roller_obj, LV_ALIGN_BOTTOM_LEFT, x_off, PADDING_BOTTOM);
		}

		/* add events */
		lv_obj_add_event_cb(prl->roller_obj, roller_event_handler, LV_EVENT_ALL, rd);

		/* add group */
		lv_group_add_obj(app_display_lvgl_group_instance_get(), prl->roller_obj);
	}

    /* focus on first item */
    lv_group_focus_obj(m_prl[0].roller_obj);

	/* set the key press callback for handling back button functionality */
	app_display_key_cb_set(extra_btn_handler);

	return ret;
}
