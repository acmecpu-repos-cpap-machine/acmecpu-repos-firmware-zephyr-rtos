/*
 * Copyright (c) 2021 Acme CPU
 * c20x_screen_settings.c
 *
 *  Created on: 04-Nov-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#define LVGL_8_LIST_FIX	1
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <lvgl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define LOG_LEVEL CONFIG_C20X_SCREENS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(c20x_screens);

#include "app_time/app_time.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"
#include "c20x_screen_switch.h"
#include "c20x_screen_manager.h"
#include "c20x_screen_spinbox.h"
#include "c20x_screen_datetime_roller.h"
#include "c20x_screen_dynamic_roller.h"
#include "c20x_screen_common_styles.h"

#include "app_settings/app_settings.h"
//#include "app_settings/app_settings_display_data.h"
#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_value.h"
#include "app_display/app_display.h"
#include "app_display/app_display_menu.h"

#define MENU_SORT	CONFIG_APP_MENU_SORT_UTIL
#define ROOT_PATH	SETTINGS_KEY_ROOT //"root"

/* static variables */

static struct k_work m_load_menu_worker;
static c20x_screen_change_cb m_screen_change = NULL;

/* settings menu list data visible on the screen */
static sys_slist_t m_list_data = SYS_SLIST_STATIC_INIT(&m_list_data);
struct menu_list_data {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	/* Data */
	char pkg_name[SETTINGS_PKG_NAME_LEN_MAX];
//	char fullpath[SETTINGS_FULLPATH_LEN_MAX];
//	int lookup_idx;
//	uint8_t disp_stat;
//	char display_name[SETTINGS_DISP_NAME_LEN_MAX];
//	const char *display_name;
	struct app_display_menu const *display_menu;
};

/* LIFO to track button presses so that the correct button can be focused while traversing back the hierarchy */
static K_LIFO_DEFINE(m_focus_track_lifo);
struct focus_track_data {
	void *LIFO_reserved;	/* 1st word reserved for use by LIFO */
	int32_t btn_idx;		/* Data */
};

typedef enum {
	CLICK_TYPE_BACK,
	CLICK_TYPE_FORWARD,
} CLICK_TYPE;

static lv_style_t m_list_style;
static lv_style_t m_sb_style;
static lv_style_t m_listbtn_style;
static lv_style_t m_listbtn_fo_style;	// focused style
static lv_style_t m_listbtn_pr_style;	// pressed pressed
static lv_obj_t *m_bglabel = NULL;
static lv_obj_t *m_label = NULL;
static lv_obj_t * m_list = NULL;
static char m_curr_path[SETTINGS_FULLPATH_LEN_MAX] = ROOT_PATH;
int32_t m_focus_btn_idx = -1;
uint8_t m_click_type = CLICK_TYPE_FORWARD;

/* static functions */
static void swap_data(struct menu_list_data *mld1, struct menu_list_data *mld2)
{
	struct menu_list_data tmp;

	tmp.display_menu = mld2->display_menu;
	strcpy(tmp.pkg_name, mld2->pkg_name);

	mld2->display_menu = mld1->display_menu;
	strcpy(mld2->pkg_name, mld1->pkg_name);

	mld1->display_menu = tmp.display_menu;
	strcpy(mld1->pkg_name, tmp.pkg_name);
}

static void menulist_sort(sys_slist_t *list, int order)
{
	int swapped;
	sys_snode_t *ptr1;
	sys_snode_t *lptr = NULL;
	struct menu_list_data *mld1, *mld2;

	/* Checking for empty list */
	if (sys_slist_peek_head(list) == NULL)
		return;

	do {
		swapped = 0;
		ptr1 = sys_slist_peek_head(list);

		while (ptr1->next != lptr) {
			mld1 = SYS_SLIST_CONTAINER(ptr1, mld1, node);
			mld2 = SYS_SLIST_CONTAINER(ptr1->next, mld2, node);
			if (mld1->display_menu->settings_data->display_order > mld2->display_menu->settings_data->display_order) {
				swap_data(mld1, mld2);
				swapped = 1;
			}
			ptr1 = ptr1->next;
		}
		lptr = ptr1;
	} while (swapped);
}

static inline int menulist_add_data(sys_slist_t *list,
		struct menu_list_data *menu_data) {
	sys_slist_append(list, &menu_data->node);

	return 0;
}

static inline void menulist_remove(sys_slist_t *list) {
	struct menu_list_data *mdata, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, mdata, tmp, node)
	{
		if (mdata) {
			sys_slist_remove(list, NULL, &mdata->node);
			free(mdata);
		}
	}
}

static inline int menulist_check_duplicates(sys_slist_t *list, const char *name) {
	struct menu_list_data *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node)
	{
		if (!strcmp(cb->pkg_name, name)) {
			return -1;
		}
	}
	return 0;
}

static inline int lookup_array_idx_get(int num_settings, const char *fullpath,
				const struct app_display_menu *adm) {
//	int num_settings = sizeof(g_display_menu) / sizeof(struct app_display_menu);
	int idx = -1;
	for (int i=0; i<num_settings; i++) {
//		if (!strcmp(fullpath, g_display_menu[i].settings_data->fullpath)) {
		if (!strcmp(fullpath, (adm+i)->settings_data->fullpath)) {
			idx = i;
			break;
		}
	}
	return idx;
}

static int settings_subtree_get_handler(const char *prev_path, const char *pkg_key) {
	int ret = 0;

	/* if the key is NULL */
	if ((prev_path == NULL) || (pkg_key == NULL)) {
		return 0;
	}

	LOG_DBG("pkg_key = %s", pkg_key);

	/* extract the package name from the key */
	char *key = (char *) pkg_key;
	char* name = strtok(key, "/");

	LOG_DBG("pkg_name = %s", name);

	/* search the menu list if this name is already present */
	ret = menulist_check_duplicates(&m_list_data, name);
	if (ret != 0) {
		/* duplicate entry */
		return 0;
	}

	/* create the fullpath */
	char fullpath[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
	strcat(fullpath, prev_path);
	strcat(fullpath, "/");
	strcat(fullpath, name);

	/* get the index from the lookup table array */
	int idx = lookup_array_idx_get(	(sizeof(g_display_menu) / sizeof(struct app_display_menu)),
									fullpath, g_display_menu);

	/* if we found the item on the lookup array, we add it to the display slist */
	if ((idx >= 0) && (g_display_menu[idx].settings_data->displayable != 0)) {
		struct menu_list_data *mld = (struct menu_list_data*) calloc(1,
				sizeof(struct menu_list_data));
		if (mld == NULL) {
			LOG_ERR("No memory, calloc failed at %d", __LINE__);
			return -ENOMEM;
		}

		/* add values to the slist node */
//		strcpy(mld->fullpath, fullpath);
		strcpy(mld->pkg_name, name);
//		mld->lookup_idx = idx;
//		mld->disp_stat = g_display_menu[idx].display_stat;
//		strcpy(mld->display_name, g_display_menu[idx].disp_name);
//		mld->display_name = g_display_menu[idx].disp_name;
		mld->display_menu = &g_display_menu[idx];

		/* add an item to the display list */
		ret = menulist_add_data(&m_list_data, mld);
	}

	return ret;
}


//static void list_btn_handler(lv_obj_t *obj, lv_event_t event) {
static void list_btn_handler(lv_event_t *event) {
	lv_event_code_t code = lv_event_get_code(event);
	lv_obj_t * obj = lv_event_get_target(event);
//	LOG_DBG("code: %d", code);

	if (code == LV_EVENT_CLICKED) {
		/* set click type */
		m_click_type = CLICK_TYPE_FORWARD;

		/* get the button index and PUT it to the LIFO */
		struct focus_track_data *ftd = (struct focus_track_data*) calloc(1, sizeof(struct focus_track_data));
		if (ftd == NULL) {
			LOG_ERR("No memory, calloc failed at %d", __LINE__);
			return;
		}

//		ftd->btn_idx = lv_list_get_btn_index(m_list, obj);
//		ftd->btn_idx = lv_list_get_btn_index(m_list, event->current_target);
//		ftd->btn_idx = lv_obj_get_child_id(event->current_target);
		ftd->btn_idx = lv_obj_get_index(obj);
		k_lifo_put(&m_focus_track_lifo, ftd);


		/* get the button data object and text */
		struct app_display_menu const *display_menu = lv_event_get_user_data(event);
//		const char * btxt = lv_list_get_btn_text(obj);
		const char * btxt = lv_list_get_btn_text(m_list, event->current_target);
		LOG_INF("Clicked: %s, path = %s", btxt, display_menu->settings_data->fullpath);

#if 0
		/* Search clicked button and load submenu */
		struct menu_list_data *mdata, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_list_data, mdata, tmp, node)
		{
			if (!strcmp(btxt, mdata->display_name)) {
				memset(m_curr_path, 0x00, sizeof(m_curr_path));
				strcpy(m_curr_path, mdata->fullpath);
				break;
			}
		}
#else
		strcpy(m_curr_path, display_menu->settings_data->fullpath);
#endif
		LOG_DBG("submenu current path: %s\n", m_curr_path);

		/* delete the existing slist */
		menulist_remove(&m_list_data);

		/* remove all button from the lv_list */
//		lv_list_clean(m_list);
		lv_obj_clean(m_list);

		/* submit a worker thread to load the required menu */
		k_work_submit(&m_load_menu_worker);
	}
}

static void extra_btn_handler(uint32_t key) {
	switch (key) {
	case LV_KEY_ESC:
	{
		/* set click type */
		m_click_type = CLICK_TYPE_BACK;

		/* GET the previous clicked button index from the LIFO */
		struct focus_track_data *ftd = k_lifo_get(&m_focus_track_lifo, K_MSEC(5));
		if (ftd != NULL) {
			m_focus_btn_idx = ftd->btn_idx;
			free(ftd);
		} else if (ftd == NULL) {
			/* No more parent list, settings menu should be exited */
			/* delete the existing slist */
			menulist_remove(&m_list_data);

			/* remove all button from the lv_list */
//			lv_list_clean(m_list);
			lv_obj_clean(m_list);

			m_screen_change(C20X_SCREEN_DASHBOARD);
			return;
		}

		/* create the new path */
		int len = strlen(m_curr_path);
		while ((len >= 0) && (m_curr_path[len] != '/')) {
			len--;
		}
		if (len > 0)	m_curr_path[len] = 0x00;

		/* delete the existing slist */
		menulist_remove(&m_list_data);

		/* remove all button from the lv_list */
//		lv_list_clean(m_list);
		lv_obj_clean(m_list);
#if (!LVGL_8_LIST_FIX)
		/* set the lvgl group for navigation */
		app_display_lvgl_group_set_current(m_list);
#endif
		/* set the key press callback for handling back button functionality */
		app_display_key_cb_set(extra_btn_handler);

		/* submit a worker thread to load the required menu */
		k_work_submit(&m_load_menu_worker);
		break;
	}
	case LV_KEY_HOME:
		break;
#if (LVGL_8_LIST_FIX)
	case LV_KEY_DOWN:
		lv_group_focus_next(app_display_lvgl_group_instance_get());
		break;
	case LV_KEY_UP:
		lv_group_focus_prev(app_display_lvgl_group_instance_get());
		break;
#endif
	default:
		break;
	}
}

/*
 * Function to get array index of a setting value from the saved value
 * This function only works for settings having datatype of struct setting_value
 */
//static int find_option_idx_from_value(struct gui_param_metadata *gpm, const char* settings_path)
static int find_option_idx_from_value(const char* settings_path, struct setting_value_options *options,
											int16_t *pidx)
{
	struct setting_value val;
	*pidx = 0;
	int ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
	if (ret == 0) {
		for (int i = 0; i < options->num_options; i++) {
//			if (memcmp(&gpm->data[i].val, &val, sizeof(struct setting_value)) == 0) {
			if (memcmp(&options->op_val[i].val, &val, sizeof(struct setting_value)) == 0) {
				*pidx = i;
				break;
			}
		}
	}
	return ret;
}

static int get_settings_curr_val(struct app_display_menu const *pdm, char* str_val, size_t len)
{
	int ret=0;
	struct gui_param_metadata *gpm = pdm->gui_data;
	struct app_settings_data const *asd = pdm->settings_data;
	uint8_t datatype = asd->datatype;
	uint32_t size = asd->size;

	if (str_val == NULL)	return -1;

	if (datatype == SETTING_DATATYPE_SETTING_VALUE) {
		if (gpm == NULL)		return -1;
//		ret = find_option_idx_from_value(gpm, pdm->fullpath);
		ret = find_option_idx_from_value(asd->fullpath, asd->options, &gpm->selected_idx);
		if (ret == 0) {
			struct app_settings_value const *data = asd->options->op_val;
			int idx = gpm->selected_idx;
			strcpy(str_val, data[idx].key);
		}
	} else if (datatype == SETTING_DATATYPE_STRING) {
		ret = app_settings_load_single(asd->fullpath, str_val, size);
	} else {
		memset(str_val, 0x00, len);
	}
	return ret;
}

static void strip_or_append_listbtn_text(char *txt)
{
	uint8_t max_len = C20x_SCREEN_LISTBTN_TEXT_LEN_MAX;
	size_t txt_len = strlen(txt);

	if (txt_len >= max_len) {
		txt[max_len-2] = ' ';	// second last character on the list btn should be space
	} else {
		uint8_t space_count = (max_len - txt_len) - 1;	// number of spaces to append, last char to be >
		if (space_count == 0) space_count = 1;			// at least 1 space is needed here
		for (uint8_t i=0; i<space_count; i++) {
			txt[txt_len+i] = ' ';
		}
	}
	txt[max_len-1] = '>';	// last character on the list btn should be >
	txt[max_len] = '\0';	// end with null
}

static int settings_menu_load(const char *settings_path) {
	int ret = 0;
	int idx = -1;

	ret = app_settings_subtree_get(settings_path, settings_subtree_get_handler);
	if (ret == 0) {
		/* Set the header label */
		/* get the index from the lookup table array */
//		idx = lookup_array_idx_get(settings_path);
		idx = lookup_array_idx_get((sizeof(g_display_menu) / sizeof(struct app_display_menu)),
										settings_path, g_display_menu);


		struct app_settings_data const *asd = g_display_menu[idx].settings_data;

		if (idx < 0)
			c20x_screen_settings_text_set("Settings");
		else
			c20x_screen_settings_text_set(asd->name);

#if LVGL_8_LIST_FIX
	    /* remove all other items from group */
	    lv_group_remove_all_objs(app_display_lvgl_group_instance_get());
#endif

	    /* check for special cases */
	    if ((asd->datatype == SETTING_DATATYPE_DATE) ||
	    		(asd->datatype == SETTING_DATATYPE_TIME)) {
	    	goto handle_special;
	    }

#if (MENU_SORT)
	    /* sort the menu list */
	    menulist_sort(&m_list_data, 0);
#endif

		/* Add buttons to the list*/
		lv_obj_t *list_btn = NULL, *focus_btn = NULL;
		idx = -1;
		char btn_txt[SETTINGS_DISP_NAME_LEN_MAX + SETTING_VAL_STR_LEN_MAX + 1] = {0x00};

		struct menu_list_data *mdata, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_list_data, mdata, tmp, node)
		{
			/* get the current value of a setting item, this will be appended to the display name */
			char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
			ret = get_settings_curr_val(mdata->display_menu, val, SETTING_VAL_STR_LEN_MAX);
			if (ret < 0) {
				LOG_ERR("get_settings_curr_val failed");
//				return -1;
				continue;
			}
			LOG_DBG("val = %s", val);

			struct app_settings_data const *asd = mdata->display_menu->settings_data;

			/* add button to list */
			if ((asd->displayable == SETTING_DISP_YES) || (asd->displayable == SETTING_DISP_MULTILEVEL)) {
				if (	(asd->datatype == SETTING_DATATYPE_DATE) ||
						(asd->datatype == SETTING_DATATYPE_TIME) ||
						(asd->datatype == SETTING_DATATYPE_NONE)
						)
					sprintf(btn_txt, "%s", asd->name);
				else
					sprintf(btn_txt, "%s: %s", asd->name, val);
#if (CONFIG_C20X_SCREENS_OLED)
				/* strip / append text to fit on a button with arrow sign */
				strip_or_append_listbtn_text(btn_txt);
				LOG_DBG("btn_txt = %s", btn_txt);
				list_btn = lv_list_add_btn(m_list, NULL, /*mdata->display_name*/btn_txt);
#elif (CONFIG_C20X_SCREENS_TFT)
				list_btn = lv_list_add_btn(m_list, LV_SYMBOL_RIGHT, /*mdata->display_name*/btn_txt);
#endif
			} else if (asd->displayable == SETTING_DISP_COND_MODE) {
				uint8_t count = asd->params->num_params;
				uint16_t curr_mode_bitmask = app_settings_curr_mode_get();
				for (int i=0; i<count; i++) {
					struct app_settings_param_value const *aspv = &asd->params->param_val[i];
					if (curr_mode_bitmask & aspv->mode_bitmask) {
						if (strlen(aspv->override_name) > 0)
							sprintf(btn_txt, "%s: %s", aspv->override_name, val);
						else
							sprintf(btn_txt, "%s: %s", asd->name, val);
#if (CONFIG_C20X_SCREENS_OLED)
						/* strip / append text to fit on a button with arrow sign */
						strip_or_append_listbtn_text(btn_txt);
						LOG_DBG("btn_txt = %s", btn_txt);
						list_btn = lv_list_add_btn(m_list, NULL, btn_txt);
#elif (CONFIG_C20X_SCREENS_TFT)
						list_btn = lv_list_add_btn(m_list, LV_SYMBOL_RIGHT, btn_txt);
#endif
						break;
					}
				}
			} else if (asd->displayable == SETTING_DISP_COND_WIFI) {
				uint16_t wifi_stat = app_settings_wifi_stat_get();
				LOG_DBG("wifi_stat = %d", wifi_stat);
				if ((wifi_stat == 1) && (strlen(val) >= 2)) { // wifi is on and connected to a ssid
					sprintf(btn_txt, "%s: %s", asd->name, val);
#if (CONFIG_C20X_SCREENS_OLED)
				/* strip / append text to fit on a button with arrow sign */
				strip_or_append_listbtn_text(btn_txt);
				LOG_DBG("btn_txt = %s", btn_txt);
				list_btn = lv_list_add_btn(m_list, NULL, /*mdata->display_name*/btn_txt);
#elif (CONFIG_C20X_SCREENS_TFT)
				list_btn = lv_list_add_btn(m_list, LV_SYMBOL_RIGHT, /*mdata->display_name*/btn_txt);
#endif
				}
			}


			/* this object is used by the button's event handler to
			 * load submenus from the fullpath of a button */
			struct app_display_menu const *display_menu = mdata->display_menu;

			/* add button event handler */
//			lv_obj_set_event_cb(list_btn, list_btn_handler);
			if (list_btn == NULL) {
				LOG_DBG("list_btn == NULL");
				continue;
			}
			struct _lv_event_dsc_t * evnt = lv_obj_add_event_cb(list_btn, list_btn_handler, LV_EVENT_ALL, (void*) display_menu);
			if (evnt == NULL) {
				LOG_ERR("lv_obj_add_event_cb failed");
			}
#if LVGL_8_LIST_FIX
			/* add button to lvgl group for navigation */
			lv_group_add_obj(app_display_lvgl_group_instance_get(), list_btn);
#endif
			/* add style to the buttons */
#if (CONFIG_C20X_SCREENS_OLED)
//			lv_obj_remove_style_all(list_btn);
			lv_obj_remove_style(list_btn, &m_listbtn_style, LV_STATE_DEFAULT);
			lv_obj_remove_style(list_btn, &m_listbtn_fo_style, LV_STATE_FOCUS_KEY);
			lv_obj_remove_style(list_btn, &m_listbtn_pr_style, LV_STATE_PRESSED);
			lv_obj_remove_style(list_btn, c20x_screen_cmn_style_textSqueezedHigh_obj_get(), LV_PART_MAIN | LV_STATE_DEFAULT);

			lv_obj_add_style(list_btn, &m_listbtn_style, LV_STATE_DEFAULT);
			lv_obj_add_style(list_btn, &m_listbtn_fo_style, LV_STATE_FOCUS_KEY);
			lv_obj_add_style(list_btn, &m_listbtn_pr_style, LV_STATE_PRESSED);
			lv_obj_add_style(list_btn, c20x_screen_cmn_style_textSqueezedHigh_obj_get(), LV_PART_MAIN | LV_STATE_DEFAULT);	// add global style
			lv_obj_invalidate(list_btn);
#endif
			idx++;
			if (idx == m_focus_btn_idx) {
				focus_btn = list_btn;
			}
#if LVGL_8_LIST_FIX
			/* focus for first time load */
			if ((m_click_type != CLICK_TYPE_BACK) && (idx == 0)) {
				lv_group_focus_obj(list_btn);
			}
#endif
		}

		if (focus_btn && (m_click_type == CLICK_TYPE_BACK)) {
//			lv_list_focus_btn(m_list, focus_btn);
			lv_group_focus_obj(focus_btn);
//			lv_obj_add_state(focus_btn, LV_STATE_PRESSED | LV_STATE_FOCUSED);
		}
	} else if (ret == -ENOENT) {	/* no more subtree of this item, get the value */
handle_special:
		/* get the index from the lookup table array */
//		idx = lookup_array_idx_get(settings_path);
		idx = lookup_array_idx_get((sizeof(g_display_menu) / sizeof(struct app_display_menu)),
										settings_path, g_display_menu);

		const struct app_display_menu *adm = &g_display_menu[idx];

		/* TODO: change the below switch case logic to make it more memory efficient */
		/* get the stored value against this settings and display */
		switch (adm->gui_obj_type) {
		case GUI_OBJ_ROLLER:
		{
			struct gui_param_metadata *gpm = adm->gui_data;
			uint8_t roller_count=1;
//			if (adm->datatype == SETTING_DATATYPE_DATETIME)	roller_count=3;
			uint8_t visible_row_count = 0;
#if (CONFIG_C20X_SCREENS_OLED)
			if (adm->settings_data->options->num_options >= 3)	visible_row_count = 3;
			else						visible_row_count=3;//gpm->num_items;	// TODO: fix this
#elif (CONFIG_C20X_SCREENS_TFT)
			if (adm->settings_data->options->num_options >= 3)	visible_row_count = 5;
			else						visible_row_count=3;//gpm->num_items;	// TODO: fix this
#endif
			const char *disp_name = adm->settings_data->name;

			int16_t range_min_idx=-1, range_max_idx=-1;
			struct setting_mode_params *smp = adm->settings_data->params;
//			if (gpm->has_range) {
			if ((smp) && (smp->param_val->range_min_idx >= 0)) {
				uint16_t curr_mode_bitmask = app_settings_curr_mode_get();
				uint8_t count = smp->num_params;
				for (int i = 0; i < count; i++) {
					struct app_settings_param_value const *aspv = &smp->param_val[i];
					if (curr_mode_bitmask & aspv->mode_bitmask) {
						range_min_idx = aspv->range_min_idx;
						range_max_idx = aspv->range_max_idx + 1;
						if (strlen(aspv->override_name) > 0)
							disp_name = aspv->override_name;
						break;
					}
				}
			}

			gpm->settings_path = settings_path;
			/* read settings value and find out selected index */
			find_option_idx_from_value(settings_path, adm->settings_data->options, &gpm->selected_idx);

			switch (adm->settings_data->datatype) {
			case SETTING_DATATYPE_DATE:
			{
				roller_count=3;
//				const struct app_display_menu *adm = &g_display_menu[idx];
				gpm = adm->gui_data;
				for (int i=0; i<roller_count; i++) {
					(gpm+i)->settings_path = (adm + i + 1)->settings_data->fullpath; // next 3 indexes has settings path information
//					find_option_idx_from_value((gpm+i), (gpm+i)->settings_path);
					find_option_idx_from_value((gpm+i)->settings_path,
												(adm->settings_data->options+i),
												&(gpm+i)->selected_idx);
				}
			}
				break;
			case SETTING_DATATYPE_TIME:
			{
				roller_count=2;
//				const struct app_display_menu *disp = &g_display_menu[idx];
				gpm = adm->gui_data;
				for (int i=0; i<roller_count; i++) {
					(gpm+i)->settings_path = (adm + i + 1)->settings_data->fullpath; // next 2 indexes has settings path information
//					find_option_idx_from_value((gpm+i), (gpm+i)->settings_path);
					find_option_idx_from_value((gpm+i)->settings_path,
												(adm->settings_data->options+i),
												&(gpm+i)->selected_idx);
				}
			}
				break;
			default:
				break;
			}

//			gpm->selected_idx = 0;
//			struct setting_value val;
//			ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
//			if (ret == 0) {
//				for (int i = 0; i < gpm->num_items; i++) {
//					if (memcmp(&gpm->data[i].val, &val, sizeof(struct setting_value)) == 0) {
//						gpm->selected_idx = i;
//						break;
//					}
//				}
//			}

			c20x_screen_dynamic_roller_init(
											disp_name,
											gpm,
											roller_count,
											visible_row_count,
											range_min_idx, range_max_idx,
											extra_btn_handler,
											adm->extra_func
											);
		}
			break;
		case GUI_OBJ_LABEL:
			break;
		default:
			break;
		}
	} else {

	}

	return ret;
}

static void load_menu_worker(struct k_work *work) {
	settings_menu_load(m_curr_path);
}

static void list_style_create() {

#if (CONFIG_C20X_SCREENS_OLED)
	/* Border - https://docs.lvgl.io/master/overview/style-props.html#border */
	lv_style_set_border_opa(&m_list_style, LV_OPA_COVER);
	lv_style_set_radius(&m_list_style, 0);
    lv_style_set_border_width(&m_list_style, C20x_SCREEN_LIST_BORDER_WIDTH);
    lv_style_set_border_color(&m_list_style, lv_color_white());
//    lv_style_set_border_width(&m_list_style, LV_STATE_FOCUSED, C20x_SCREEN_LIST_BORDER_WIDTH);
//    lv_style_set_border_width(&m_list_style, LV_STATE_FOCUSED | LV_STATE_EDITED, C20x_SCREEN_LIST_BORDER_WIDTH);
    lv_style_set_border_side(&m_list_style, LV_BORDER_SIDE_FULL);

	/* padding */
	lv_style_set_pad_left(&m_list_style, /*C20x_SCREEN_LIST_PADDING*/1);
	lv_style_set_pad_right(&m_list_style, /*C20x_SCREEN_LIST_PADDING_RIGHT*/1);
	lv_style_set_pad_top(&m_list_style, C20x_SCREEN_LIST_PADDING);
	lv_style_set_pad_bottom(&m_list_style, C20x_SCREEN_LIST_PADDING);
//	lv_style_set_pad_inner(&m_list_style, C20x_SCREEN_LIST_PADDING);

	/* scrollbar */

    lv_style_set_bg_opa(&m_sb_style, LV_OPA_COVER);
//    lv_style_set_bg_color(&m_sb_style, LV_STATE_DEFAULT, FG_COLOR);
    lv_style_set_radius(&m_sb_style, 0);
//    lv_style_set_pad_right(&m_sb_style, LV_STATE_DEFAULT, /*LV_DPI / 30*/1);
//    lv_style_set_pad_bottom(&m_sb_style, LV_STATE_DEFAULT, /*LV_DPI / 30*/1);
    lv_style_set_size(&m_sb_style, C20X_SCREEN_SCROLLBAR_WIDTH);
    lv_style_set_bg_color(&m_sb_style, lv_color_white());
#endif
}

static void listbtn_style_create(uint32_t x_width, uint32_t y_height) {
#if (CONFIG_C20X_SCREENS_OLED)
	/* Border - https://docs.lvgl.io/master/overview/style-props.html#border */

	/* default */
	lv_style_set_width(&m_listbtn_style, (x_width-C20x_SCREEN_LIST_PADDING_RIGHT));
	lv_style_set_border_opa(&m_listbtn_style, LV_OPA_COVER);
    lv_style_set_border_width(&m_listbtn_style, C20x_SCREEN_LISTBTN_BORDER_WIDTH);
    lv_style_set_border_color(&m_listbtn_style, lv_color_white());
	lv_style_set_border_side(&m_listbtn_style, LV_BORDER_SIDE_NONE);
	lv_style_set_pad_left(&m_listbtn_style, /*C20x_SCREEN_LISTBTN_PADDING*/1);
	lv_style_set_pad_right(&m_listbtn_style, C20x_SCREEN_LISTBTN_PADDING);
	lv_style_set_pad_top(&m_listbtn_style, C20x_SCREEN_LISTBTN_PADDING);
	lv_style_set_pad_bottom(&m_listbtn_style, C20x_SCREEN_LISTBTN_PADDING);
	lv_style_set_bg_opa(&m_listbtn_style, LV_OPA_COVER);

	/* focused */
	lv_style_set_border_opa(&m_listbtn_fo_style, LV_OPA_COVER);
	lv_style_set_border_width(&m_listbtn_fo_style, C20x_SCREEN_LISTBTN_BORDER_WIDTH);
	lv_style_set_border_color(&m_listbtn_fo_style, lv_color_white());
	lv_style_set_border_side(&m_listbtn_fo_style, LV_BORDER_SIDE_FULL);
	lv_style_set_bg_color(&m_listbtn_fo_style, lv_color_white());
	lv_style_set_text_color(&m_listbtn_fo_style, lv_color_black());

	/* pressed */
	lv_style_set_bg_color(&m_listbtn_pr_style, lv_color_black());
	lv_style_set_text_color(&m_listbtn_pr_style, lv_color_white());
#endif
}

static void settings_label_init(uint32_t x_width, uint32_t y_height) {
	m_bglabel = lv_obj_create(lv_scr_act());
	lv_obj_set_width(m_bglabel, x_width);
	lv_obj_set_height(m_bglabel, y_height);
#if (CONFIG_C20X_SCREENS_OLED)
//	lv_obj_set_style_bg_color( m_bglabel, lv_color_white(), LV_PART_MAIN);
	lv_obj_set_style_radius(m_bglabel, 0, LV_PART_MAIN);
	lv_obj_set_style_border_width(m_bglabel, 1, LV_PART_MAIN);
	lv_obj_set_style_border_color(m_bglabel, lv_color_white(), LV_PART_MAIN);
//	lv_obj_set_style_border_side(m_bglabel, LV_BORDER_SIDE_NONE, LV_PART_MAIN);
	lv_obj_set_style_border_side(m_bglabel, LV_BORDER_SIDE_FULL, LV_PART_MAIN);
	lv_obj_set_style_pad_all(m_bglabel, 0, LV_PART_MAIN);
#endif

	m_label = lv_label_create(m_bglabel);
//	lv_obj_set_style_text_color( m_label, lv_color_black(), LV_PART_MAIN);

	lv_obj_align(m_bglabel, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_align_to(m_label, m_bglabel, LV_ALIGN_LEFT_MID, 0, 0);
}

/* global functions */
int c20x_screen_settings_init(uint32_t x_width, uint32_t y_height, uint32_t label_height, lv_group_t * lvgl_grp) {
	int ret = 0;

	/* initialize static variables */
	memset(m_curr_path, 0x00, sizeof(m_curr_path));
	strcpy(m_curr_path, ROOT_PATH);
	m_focus_btn_idx = -1;
	m_click_type = CLICK_TYPE_FORWARD;

	/* create a new list, this should happen only once, unless the settings menu is being exited
	 * this object will be used by the lvgl group to navigate the menu
	 *  */
	if (m_list == NULL) {
		settings_label_init(x_width, label_height);
//		c20x_screen_settings_text_set("Main Settings");

		m_list = lv_list_create(lv_scr_act());
		if (m_list == NULL) {
			return -1;
		}

		/* styles */
		list_style_create();
		listbtn_style_create(x_width, y_height);
//		lv_obj_remove_style_all(m_list);
		lv_obj_add_style(m_list, &m_list_style, LV_PART_MAIN | LV_STATE_DEFAULT);
		lv_obj_add_style(m_list, &m_sb_style, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
//		lv_obj_set_style_size(m_list, 2, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);

//		lv_style_list_t * list;
//	    list = lv_obj_get_style_list(m_list, LV_LIST_PART_SCROLLBAR);
//	    _lv_style_list_add_style(list, &m_sb_style);

		/* TODO */
		lv_obj_set_size(m_list, x_width, y_height);
		lv_obj_align_to(m_list, m_bglabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

//		lv_list_set_edge_flash(m_list, true);

		/* set the lvgl group for navigation */
//		app_display_lvgl_group_set_current(m_list);
#if (!LVGL_8_LIST_FIX)
		lv_group_add_obj(lvgl_grp, m_list);
#endif
//		lv_group_set_editing(lvgl_grp,false);
//		lv_group_focus_obj(m_list);

		/* set the key press callback for handling back button functionality */
		app_display_key_cb_set(extra_btn_handler);
	}

	/* Prepare worker thread */
	k_work_init(&m_load_menu_worker, load_menu_worker);

	return ret;
}

void c20x_screen_settings_deinit() {
	if (m_list != NULL) {
		lv_obj_del(m_label);
		lv_obj_del(m_bglabel);
//		lv_list_clean(m_list);
		lv_obj_clean(m_list);
		lv_obj_del(m_list);
		m_list = NULL;
	}
}

void c20x_screen_settings_text_set(const char* text) {
	lv_label_set_text(m_label, text);
}

/*
int c20x_screen_settings_obj_get(lv_obj_t **pobj) {
	if (m_list == NULL) {
		*pobj = NULL;
		return -1;
	}
	*pobj = m_list;
	return 0;
}

int c20x_screen_setings_key_cb_set(c20x_screen_settings_key_cb *key_cb) {
	*key_cb = extra_btn_handler;
	return 0;
}
*/

void c20x_screen_settings_start() {
	/* submit a worker thread to load the required menu */
	k_work_submit(&m_load_menu_worker);
}

void c20x_screen_settings_cb_set(c20x_screen_change_cb screen_change_cb) {
	m_screen_change = screen_change_cb;
}

/* IMPORTANT: this function should be called only once */
void c20x_screen_settings_styles_init()
{
	lv_style_init(&m_list_style);
	lv_style_init(&m_sb_style);
	lv_style_init(&m_listbtn_style);
	lv_style_init(&m_listbtn_fo_style);
	lv_style_init(&m_listbtn_pr_style);
}

