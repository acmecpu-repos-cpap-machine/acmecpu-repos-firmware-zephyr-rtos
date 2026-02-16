/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 27-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(html_gen);

#include "app_settings/app_settings.h"
#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_value.h"
#include "app_net/app_net_html_gen.h"
#include "app_time/app_time.h"
#if (CONFIG_APP_MENU_SORT_UTIL)
	#include "app_utils/app_menu_util.h"
#endif

#include "app_thread_configs.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt_cmds.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_uart_m2m_callback.h"
#include "app_net/app_net_data_util.h"

#define MENU_SORT	CONFIG_APP_MENU_SORT_UTIL
#define ROOT_PATH	SETTINGS_KEY_ROOT

static char *html_str = NULL;
static int hsidx = 0;
static uint16_t m_curr_mode = 0;
static uint16_t m_wifi_stat = 0;

#if (MENU_SORT)
/* list to sort settings menu data */
static sys_slist_t m_sort_list = SYS_SLIST_STATIC_INIT(&m_sort_list);
#endif	/* (MENU_SORT) */

static inline int lookup_array_idx_get(int num_settings, const char *fullpath,
										struct html_menu_data *hmd) {
//	int num_settings = sizeof(g_html_menu) / sizeof(struct html_menu_data);
	int idx = -1;
	for (int i=0; i<num_settings; i++) {
//		if (!strcmp(fullpath, g_html_menu[i].settings_data->fullpath)) {
		if (!strcmp(fullpath, (hmd+i)->settings_data->fullpath)) {
			idx = i;
			break;
		}
	}
	return idx;
}

#define MAX_LEVELS	20
struct tree_level {
	char path[SETTINGS_FULLPATH_LEN_MAX];
};
//static struct tree_level lvl1[MAX_LEVELS];
static struct tree_level *m_lvl = NULL;;
static int m_idx_lvl = 0;

static int check_and_copy(struct tree_level* tl, int *pidx, const char *data)
{
	int match = 0;
	for (int i=0; i<MAX_LEVELS; i++) {
		if (strcmp(tl[i].path, data) == 0) {
			match = 1;
			break;
		}
	}
	if (match == 0) {
		strcpy(tl[*pidx].path, data);
		(*pidx)++;
	}
	return match;
}

static void add_space(char *space, int num)
{
	memset(space, 0x00, 50);
	for (int i=0; i<num; i++) {
		strcat(space, HTML_FOUR_SPACE);
	}
}

static void add_htag(char *htag_open, char *htag_close, int level)
{
	memset(htag_open, 0x00, 20);
	memset(htag_close, 0x00, 20);
#if 0
	switch (level) {
	case 1:
		strcpy(htag_open, HTML_HTAG_OPEN_1);
		strcpy(htag_close, HTML_HTAG_CLOSE_1);
		break;
	case 2:
		strcpy(htag_open, HTML_HTAG_OPEN_2);
		strcpy(htag_close, HTML_HTAG_CLOSE_2);
		break;
	case 3:
		strcpy(htag_open, HTML_HTAG_OPEN_3);
		strcpy(htag_close, HTML_HTAG_CLOSE_3);
		break;
	case 4:
		strcpy(htag_open, HTML_HTAG_OPEN_4);
		strcpy(htag_close, HTML_HTAG_CLOSE_4);
		break;
	}
#else
	strcpy(htag_open, HTML_HTAG_OPEN_4);
	strcpy(htag_close, HTML_HTAG_CLOSE_4);
#endif
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
	int match = 0;
	int ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
	if (ret == 0) {
		for (int i = 0; i < options->num_options; i++) {
//			if (memcmp(&gpm->data[i].val, &val, sizeof(struct setting_value)) == 0) {
			if (memcmp(&options->op_val[i].val, &val, sizeof(struct setting_value)) == 0) {
				*pidx = i;
				match = 1;
				break;
			}
		}
	}

	if (match == 1)	ret = 0;
	else			ret = -1;

	return ret;
}

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
static char* get_settings_disp_val(struct settings_runtime_value *svd)
{
	return svd->disp_val;
}
#endif

static int get_settings_curr_val(struct html_menu_data *hmd, char* str_val, size_t len)
{
	int ret=0;
//	struct gui_param_metadata *gpm = pdm->gui_data;
	struct app_settings_data const *asd = hmd->settings_data;
	uint8_t datatype = asd->datatype;
	uint32_t size = asd->size;

	if (str_val == NULL)	return -1;

	if (datatype == SETTING_DATATYPE_SETTING_VALUE) {
//		if (gpm == NULL)		return -1;
//		ret = find_option_idx_from_value(gpm, pdm->fullpath);
		ret = find_option_idx_from_value(asd->fullpath, asd->options, &hmd->selected_idx);
		if (ret == 0) {
			struct app_settings_value const *data = asd->options->op_val;
			int idx = hmd->selected_idx;
			strcpy(str_val, data[idx].key);
		}
	} else if (datatype == SETTING_DATATYPE_STRING) {
		ret = app_settings_load_single(asd->fullpath, str_val, size);
	} else if (datatype == SETTING_DATATYPE_DATE) {
		char date[11]= {0x00};	// format yyyy-mm-dd (10 bytes) needed for html
//		int idx=0;
//		struct setting_value dt_val;
//
//		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_YR, &dt_val, sizeof(struct setting_value));
//		idx += sprintf(date+idx, "%d-", dt_val.val1);	// year
//
//		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_MON, &dt_val, sizeof(struct setting_value));
//		if (dt_val.val1 < 10)
//			idx += sprintf(date+idx, "0%d-", dt_val.val1);	// mon
//		else
//			idx += sprintf(date+idx, "%d-", dt_val.val1);	// mon
//
//		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_DAY, &dt_val, sizeof(struct setting_value));
//		if (dt_val.val1 < 10)
//			idx += sprintf(date+idx, "0%d", dt_val.val1);	// day
//		else
//			idx += sprintf(date+idx, "%d", dt_val.val1);	// day
		app_time_html_formatted_date_get(date);
		strcpy(str_val, date);
	} else if (datatype == SETTING_DATATYPE_TIME) {
		char time[6]= {0x00};	// format hr:mn (6 bytes) needed for html
//		int idx=0;
//		struct setting_value tm_val;
//		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_TIM_HR, &tm_val, sizeof(struct setting_value));
//		if (tm_val.val1 < 10)
//			idx += sprintf(time+idx, "0%d:", tm_val.val1);	// mon
//		else
//			idx += sprintf(time+idx, "%d:", tm_val.val1);	// mon
//
//		ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_TIM_MIN, &tm_val, sizeof(struct setting_value));
//		if (tm_val.val1 < 10)
//			idx += sprintf(time+idx, "0%d", tm_val.val1);	// day
//		else
//			idx += sprintf(time+idx, "%d", tm_val.val1);	// day
		app_time_html_formatted_time_get(time);
		strcpy(str_val, time);
	} else if (datatype == SETTING_DATATYPE_UINT32) {
		uint32_t val_i = 0;
		ret = app_settings_load_single(asd->fullpath, &val_i, size);
		sprintf(str_val, "%d", val_i);
	} else {
		memset(str_val, 0x00, len);
	}
	return ret;
}

void settings_to_html(const char *prev_path, const char *pkg_key)
{
	char space[50] = {0x00};
	char htag_open[20] = {0x00};
	char htag_close[20] = {0x00};

	int ret = 0, level_cnt = 0, rtc_fetch = 0;

	/* create the fullpath */
	char fullpath[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
	strcat(fullpath, prev_path); strcat(fullpath, "/");
	strcat(fullpath, pkg_key);

	/* get the index from the lookup table array */
	int idx = lookup_array_idx_get(	(sizeof(g_html_menu) / sizeof(struct html_menu_data)),
									fullpath, g_html_menu);

	if (idx < 0)	return;

	struct app_settings_data const *asd = g_html_menu[idx].settings_data;
	struct html_menu_data *hmd = &g_html_menu[idx];
#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	struct settings_runtime_value *svd = &g_disp_val[idx];
#endif
//	char path[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
//	strcpy(path, asd->fullpath);

	char *ptr = (char*) pkg_key; //path;
	while (1) {
		ptr = strstr(ptr, "/");
		if (ptr == NULL) break;

		level_cnt++;

		char lvl[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
		strcat(lvl, prev_path); strcat(lvl, "/");
		strncat(lvl, pkg_key, (ptr-pkg_key));
		ptr = ptr + 1;
		if (check_and_copy(&m_lvl[0], &m_idx_lvl, lvl) == 0) {
			int tmpidx = lookup_array_idx_get((sizeof(g_html_menu) / sizeof(struct html_menu_data)),
												lvl, g_html_menu);
			if (tmpidx >= 0) {
				struct app_settings_data const *tmp = &g_sdata[tmpidx];

			    /* check for special cases */
			    if ((asd->datatype == SETTING_DATATYPE_DATE) ||
			    		(asd->datatype == SETTING_DATATYPE_TIME)) {
			    	asd = tmp;
			    	hmd = &g_html_menu[tmpidx];

			    	/*This variable is used to fetch RTC values; HIGH - Fetches the RTC values*/
			    	rtc_fetch = 1;
			    	goto handle_special;
			    }

				add_htag(htag_open, htag_close, level_cnt);
				add_space(space, level_cnt);
//				hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, tmp->fullpath,
//						HTML_LIST_ITEM_MIDFIX, space, tmp->name, HTML_LIST_ITEM_POSTFIX, htag_close);
				hsidx += sprintf(html_str+hsidx, "%s%s%s%s", htag_open, space, tmp->name, htag_close);
			}
		}
	}

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	/* get the current value of a setting item, this will be appended to the display name */
	char *val;
#else
	char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
#endif

handle_special:
//	int64_t sort_start, sort_delta=0;
//	sort_start = k_uptime_get();
#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	val = get_settings_disp_val(svd);

	/*If condition to fetch the value of date/time from the RTC and displays to the HTML web page*/
	if (rtc_fetch ==1) {
	ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
		if (ret < 0) {
			LOG_ERR("%s: get_settings_curr_val failed, %s", __func__, hmd->settings_data->name);
			return;
		}
	}
#else
	ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
	if (ret < 0) {
		LOG_ERR("%s: get_settings_curr_val failed, %s", __func__, hmd->settings_data->name);
		return;
	}
#endif
//	sort_delta = k_uptime_delta(&sort_start);
//	LOG_INF("sort gentime: %lld", sort_delta);
	LOG_DBG("val = %s", val);

	if ((asd->displayable == SETTING_DISP_YES) || (asd->displayable == SETTING_DISP_MULTILEVEL)) {
		add_htag(htag_open, htag_close, ++level_cnt);
		add_space(space, level_cnt);
		hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s: %s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, asd->fullpath,
				HTML_LIST_ITEM_MIDFIX, space, asd->name, val, HTML_LIST_ITEM_POSTFIX, htag_close);
	} else if (asd->displayable == SETTING_DISP_COND_MODE) {
		char name_txt[SETTINGS_NAME_LEN_MAX + 1] = {0x00};
		uint8_t count = asd->params->num_params;
		uint16_t curr_mode_bitmask = m_curr_mode; //app_settings_curr_mode_get();
		LOG_DBG("mode = 0x%x", curr_mode_bitmask);
		for (int i=0; i<count; i++) {
			struct app_settings_param_value const *aspv = &asd->params->param_val[i];
			if (curr_mode_bitmask & aspv->mode_bitmask) {
				if (strlen(aspv->override_name) > 0)
					sprintf(name_txt, "%s", aspv->override_name);
				else
					sprintf(name_txt, "%s", asd->name);

				add_htag(htag_open, htag_close, ++level_cnt);
				add_space(space, level_cnt);
				hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s: %s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, asd->fullpath,
						HTML_LIST_ITEM_MIDFIX, space, name_txt, val, HTML_LIST_ITEM_POSTFIX, htag_close);
				break;
			}
		}
	} else if (asd->displayable == SETTING_DISP_COND_WIFI/*SETTING_DISP_COND_WIFI_MULTI*/) {
		uint16_t wifi_stat = m_wifi_stat; //app_settings_wifi_stat_get();
		LOG_DBG("wifi_stat = %d", wifi_stat);
		if ((wifi_stat == 1) && (strlen(val) >= 2)) { // wifi is on and connected to a ssid
			add_htag(htag_open, htag_close, ++level_cnt);
			add_space(space, level_cnt);
			hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s%s: %s%s%s", htag_open, HTML_LIST_ITEM_PREFIX, asd->fullpath,
					HTML_LIST_ITEM_MIDFIX, space, asd->name, val, HTML_LIST_ITEM_POSTFIX, htag_close);
		}
	}
}

#if (MENU_SORT)
static int sorted_list_to_html(sys_slist_t *list)
{
//	int64_t sort_start, sort_delta=0;
//	struct sort_list_data *sld, *tmp;
	struct menusort_data *sld, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, sld, tmp, node)
	{
//		sort_start = k_uptime_get();
		settings_to_html(sld->prev_path, sld->pkg_key);
//		sort_delta = k_uptime_delta(&sort_start);
//		LOG_INF("sort gentime: %lld", sort_delta);
	}
	return 0;
}
#endif /* #if (MENU_SORT) */

static int settings_subtree_get_handler(const char *prev_path, const char *pkg_key)
{
	int ret = 0;

	/* if the key is NULL */
	if ((prev_path == NULL) || (pkg_key == NULL)) {
		return 0;
	}

	LOG_DBG("pkg_key = %s", pkg_key);

	if (hsidx >= (HTML_PAGE_SIZE_MAX - 100)) {
		LOG_ERR("html buffer almost full, %d bytes of %d bytes, cannot add more data!",
				hsidx+1, HTML_PAGE_SIZE_MAX);
		return ret;
	}
#if (MENU_SORT)
	/* add to list for sorting */
//	struct sort_list_data *sld = (struct sort_list_data *)calloc(1, sizeof(struct sort_list_data));
	struct menusort_data *sld = (struct menusort_data *)calloc(1, sizeof(struct menusort_data));
	if (sld == NULL) {
		LOG_ERR("No memory, calloc failed at %d", __LINE__);
		return -1;
	}
	/* create the fullpath */
	char fullpath[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
	strcat(fullpath, prev_path); strcat(fullpath, "/");
	strcat(fullpath, pkg_key);

	/* get the index from the lookup table array */
	int idx = lookup_array_idx_get(	(sizeof(g_html_menu) / sizeof(struct html_menu_data)),
									fullpath, g_html_menu);
	if (idx < 0) {
		LOG_ERR("incorrect array index %d", idx);
		return -1;
	}
	struct app_settings_data const *asd = g_html_menu[idx].settings_data;
	strcpy(sld->prev_path, prev_path);
	strcpy(sld->pkg_key, pkg_key);
	sld->data_idx = idx;
	sld->disp_order = asd->display_order;

	/* add to list */
//	sortlist_add_data(&m_sort_list, sld);
	app_menu_sortlist_add_data(&m_sort_list, sld);
#else
	settings_to_html(prev_path, pkg_key);
#endif	/* (MENU_SORT) */
	return ret;
}

static void options_to_html_select_list(char *html_str,
					int *phsidx,
					int16_t range_min_idx, int16_t range_max_idx,
					struct app_settings_data const *asd, int selidx)
{
	int hsidx = *phsidx;
	int start=0, end=0;
	struct setting_value_options *op = asd->options;
	if (range_min_idx >= 0) {
		start = range_min_idx; end = range_max_idx;
//		if ((sel_idx = sel_idx-range_min_idx) < range_min_idx)
//			sel_idx = range_min_idx;
	}
	else {
		start=0; end = op->num_options;
	}

	for (int opcnt = start; opcnt < end; opcnt++) {
		if (opcnt == selidx) {
			hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_SELECT_OPTION_SELECTED,
					op->op_val[opcnt].key, HTML_SELECT_OPTION_SEP, op->op_val[opcnt].key, HTML_SELECT_OPTION_END);
		} else {
			hsidx += sprintf(html_str+hsidx, "%s%s%s%s%s", HTML_SELECT_OPTION_START,
					op->op_val[opcnt].key, HTML_SELECT_OPTION_SEP, op->op_val[opcnt].key, HTML_SELECT_OPTION_END);
		}
	}

	*phsidx = hsidx;
}

static int settings_menu_to_html_convert(const char *settings_path, uint32_t *html_len)
{
	int ret = 0;
	int idx = -1;
	int64_t start, delta=0;

	/* copy html page prefix */
	hsidx += sprintf(html_str+hsidx, "%s", HTML_PAGE_PREFIX);
	hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, "Settings", HTML_HTAG_CLOSE_1);

	start = k_uptime_get();
	m_curr_mode = app_settings_curr_mode_get();
	m_wifi_stat = app_settings_wifi_stat_get();
#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
	app_settings_load();
#endif
	/* by calling this function the handler will be called
	 * with the entire subtree of "settings_path". The handler function
	 * will generate the relevant html code for the entire settings tree */
	ret = app_settings_subtree_get(settings_path, settings_subtree_get_handler);
	delta = k_uptime_delta(&start);
	LOG_INF("subtree gentime: %lld", delta);

	if (ret == 0) {
#if (MENU_SORT)
		start = k_uptime_get();
//		sortlist_sort(&m_sort_list, 0);
		app_menu_sortlist_sort(&m_sort_list, 0);
		sorted_list_to_html(&m_sort_list);
//		sortlist_remove(&m_sort_list);
		app_menu_sortlist_remove(&m_sort_list);
		delta = k_uptime_delta(&start);
		LOG_INF("sorted_list_to_html gentime: %lld", delta);
#endif	/* (MENU_SORT) */

		idx = lookup_array_idx_get(
				(sizeof(g_html_menu) / sizeof(struct html_menu_data)),
				settings_path, g_html_menu);
		if (idx >= 0) {
			struct app_settings_data const *asd = g_html_menu[idx].settings_data;

			/* check for special cases */
			if ((asd->datatype == SETTING_DATATYPE_DATE)
					|| (asd->datatype == SETTING_DATATYPE_TIME)) {
				goto handle_special;
			}
		}
	} else if (ret == -ENOENT) {	/* no more subtree of this item, get the value */
handle_special:
		/* Set the header label */
		idx = lookup_array_idx_get((sizeof(g_html_menu) / sizeof(struct html_menu_data)),
										settings_path, g_html_menu);
		if (idx < 0) {
			LOG_ERR("invalid index");
			return -1;
		}
		struct app_settings_data const *asd = g_html_menu[idx].settings_data;
		struct html_menu_data *hmd = &g_html_menu[idx];

		/* make html data entry object depending on datatype */
		hsidx += sprintf(html_str+hsidx, "%s", HTML_FORM_ACTION_START);		// form start

		switch(asd->datatype) {
		case SETTING_DATATYPE_SETTING_VALUE:	// html select list
		{
			const char *disp_name = asd->name;
			int16_t range_min_idx=-1, range_max_idx=-1;
			struct setting_mode_params *smp = asd->params;
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

			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, disp_name, HTML_HTAG_CLOSE_1);
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_SELECT_NAME_START, settings_path, HTML_SELECT_NAME_END);	// select start

			int16_t selidx=-1;
			find_option_idx_from_value(settings_path, asd->options, &selidx);
			options_to_html_select_list(html_str, &hsidx, range_min_idx, range_max_idx, asd, selidx);	// add options

			hsidx += sprintf(html_str+hsidx, "%s", HTML_SELECT_END);	// select end
		}
			break;
		case SETTING_DATATYPE_STRING:
		{
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, asd->name, HTML_HTAG_CLOSE_1);
			if (asd->editable == SETTING_EDIT_PASSWORD) {
				hsidx += sprintf(html_str+hsidx, "%s%s", HTML_PWD_INPUT_FIELD_START, settings_path);	// text field start
			} else {
				hsidx += sprintf(html_str+hsidx, "%s%s", HTML_TEXT_INPUT_FIELD_START, settings_path);	// text field start
			}
			char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
			ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
			if (ret < 0) {
				LOG_ERR("get_settings_curr_val failed, %s", hmd->settings_data->name);
//				return; ignore this error
			}
			LOG_DBG("val = %s", val);

			if (asd->editable == SETTING_EDIT_YES) {
				hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_TEXT_INPUT_FIELD_VALUE, val, HTML_TEXT_INPUT_FIELD_END);	// editable text field
			} else if (asd->editable == SETTING_EDIT_NO) {
				hsidx += sprintf(html_str+hsidx, "%s%s\" readonly>", HTML_TEXT_INPUT_FIELD_VALUE, val);	// readonly text field
			} else if (asd->editable == SETTING_EDIT_PASSWORD) {
				hsidx += sprintf(html_str+hsidx, "\" minlength=\"8\">");	// password text field
			}
		}
			break;
		case SETTING_DATATYPE_DATE:
		{
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, asd->name, HTML_HTAG_CLOSE_1);
			hsidx += sprintf(html_str+hsidx, "%s%s", HTML_DATE_INPUT_START, settings_path);	// date field start
			char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
			ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
			if (ret < 0) {
				LOG_ERR("get_settings_curr_val failed, %s", hmd->settings_data->name);
//				return; ignore this error
			}
			LOG_DBG("val = %s", val);
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_DATE_INPUT_VALUE, val, HTML_DATE_INPUT_END);	// text field start
		}
			break;
		case SETTING_DATATYPE_TIME:
		{
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, asd->name, HTML_HTAG_CLOSE_1);
			hsidx += sprintf(html_str+hsidx, "%s%s", HTML_TIME_INPUT_START, settings_path);	// time field start
			char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
			ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
			if (ret < 0) {
				LOG_ERR("get_settings_curr_val failed, %s", hmd->settings_data->name);
//				return; ignore this error
			}
			LOG_DBG("val = %s", val);
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_TIME_INPUT_VALUE, val, HTML_TIME_INPUT_END);	// text field start
		}
			break;
		case SETTING_DATATYPE_UINT32:
		{
			hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_HTAG_OPEN_1, asd->name, HTML_HTAG_CLOSE_1);
			hsidx += sprintf(html_str+hsidx, "%s%s", HTML_TEXT_INPUT_FIELD_START, settings_path);	// text field start
			char val[SETTING_VAL_STR_LEN_MAX] = {0x00};
			ret = get_settings_curr_val(hmd, val, SETTING_VAL_STR_LEN_MAX);
			if (ret < 0) {
				LOG_ERR("get_settings_curr_val failed, %s", hmd->settings_data->name);
//				return; ignore this error
			}
			LOG_DBG("val = %s", val);

			if (asd->editable == SETTING_EDIT_YES) {
				hsidx += sprintf(html_str+hsidx, "%s%s%s", HTML_TEXT_INPUT_FIELD_VALUE, val, HTML_TEXT_INPUT_FIELD_END);	// editable text field
			} else if (asd->editable == SETTING_EDIT_NO) {
				hsidx += sprintf(html_str+hsidx, "%s%s\" readonly>", HTML_TEXT_INPUT_FIELD_VALUE, val);	// readonly text field
			}
			break;
		}
		default:
			break;
		}

		hsidx += sprintf(html_str+hsidx, "%s%s", HTML_LINE_BREAK, HTML_LINE_BREAK);	// form end with Submit button
		hsidx += sprintf(html_str+hsidx, "%s", HTML_FORM_BUTTON_END);	// form end with Submit button
	} else {

	}

	/* copy html page postfix */
	hsidx += sprintf(html_str+hsidx, "%s", HTML_PAGE_POSTFIX);
	*html_len = strlen(html_str);

	return ret;
}

char* app_net_html_get(const char *path, uint32_t *html_len)
{
	if (path == NULL)
		strcpy((char*)path, ROOT_PATH);

	hsidx = 0;
	html_str = (char*)calloc(1, HTML_PAGE_SIZE_MAX);
	if (html_str == NULL) {
		LOG_ERR("calloc failed %s", __func__);
		return NULL;
	}

	m_lvl = (struct tree_level*)calloc(MAX_LEVELS, sizeof(struct tree_level));
	m_idx_lvl = 0;

//	int64_t start, delta=0;
//	start = k_uptime_get();
	settings_menu_to_html_convert(path, html_len);
//	delta = k_uptime_delta(&start);
//	LOG_INF("html gentime: %lld", delta);

	free(m_lvl);

	LOG_INF("html len = %d", *html_len);

//	char pbuf[501] = {0x00};
//	int len = (*html_len);
//	for (int i=0; i<len;) {
//		memset(pbuf, 0x00, 501);
//		memcpy(pbuf, html_str+i, 500);
//
//		LOG_INF("%s", pbuf);
//
//		i += 500;
//		if (i > len)
//			i = len;
//		k_sleep(K_MSEC(1000));
//	}

	return html_str;
}

/**
 * The below variables and functions deal with transmitting the data and
 * handling received acknowledgment
 */

/* thread static variables */
K_THREAD_STACK_DEFINE(m_htmlgen_stack, APP_THREAD_STACK_SIZE_HTMLGEN);
static struct k_thread m_htmlgen_data;
static k_tid_t m_htmlgen_tid;

struct ack_cb_data {
	struct k_sem lock;
	uint32_t last_sequence;
};
struct htmlgen_cmd_ctrl_data {
	struct app_uart_m2m_callback m2m_cb;
	struct ack_cb_data rcb_data;
	char path[SETTINGS_FULLPATH_LEN_MAX];
	int tot_payload_sent;
	uint8_t resp_stat;
};

static struct htmlgen_cmd_ctrl_data * hccd_alloc_and_init()
{
	struct htmlgen_cmd_ctrl_data *hccd = (struct htmlgen_cmd_ctrl_data*) calloc(1, sizeof(struct htmlgen_cmd_ctrl_data));
	if (hccd == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return NULL;
	}
	k_sem_init(&hccd->rcb_data.lock, 0, 1);

	return hccd;
}

static void data_ack_handler(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	struct htmlgen_cmd_ctrl_data *hccd = (struct htmlgen_cmd_ctrl_data *) cb->user_data;

	if (cmd == hccd->m2m_cb.cmd) {
		struct m2m_frame_t *frame = (struct m2m_frame_t*) data;
		uint32_t last_sequence = hccd->rcb_data.last_sequence;
		uint32_t ack = last_sequence + 1;
		if (ack == frame->ack) {
			LOG_DBG("ACK %d matched with SEQ %d", frame->ack, last_sequence);
			k_sem_give(&hccd->rcb_data.lock);
			return;
		}
		LOG_ERR("ACK %d not matched with SEQ %d", frame->ack, last_sequence);
	}
}

static void html_gen_and_send_thread(void *p1, void *p2, void *p3)
{
	struct htmlgen_cmd_ctrl_data *hccd = (struct htmlgen_cmd_ctrl_data *) p1;
	int ret = 0;
	/* make html data */
	uint32_t html_len;
	char *html = app_net_html_get(hccd->path, &html_len);

	/* transmit data and check acknowledgments */
	while (1) {
		/* TODO For now the entire html page gets generated at once so this loop runs
		 * only once. If the html page is generated in chunks, required logic
		 * needs to be implemented to send the data in chunks */

		ret = app_net_send_data_resp(UART_M2M_FRAME_DATA_RESP, hccd->m2m_cb.cmd, html,
				html_len, &hccd->tot_payload_sent, &hccd->rcb_data.last_sequence,
				&hccd->rcb_data.lock);
		if (ret < 0) {
			LOG_ERR("UART_M2M_FRAME_DATA_RESP failed");
		}
		ret = app_net_send_data_resp(UART_M2M_FRAME_DATA_RESP_ENDSTR, hccd->m2m_cb.cmd, html,
				html_len, &hccd->tot_payload_sent, &hccd->rcb_data.last_sequence,
				&hccd->rcb_data.lock);
		if (ret < 0) {
			LOG_ERR("UART_M2M_FRAME_DATA_RESP_ENDSTR failed");
		}

		break;
	}

	/* deallocate resources */
	app_uart_m2m_callback_remove(&hccd->m2m_cb, data_ack_handler, hccd->m2m_cb.cmd);
	free(hccd);
	free(html);
}

int app_net_html_gen_and_send(const char *path, uint32_t cmd)
{
	int ret = 0;

	/* allocate control data */
	struct htmlgen_cmd_ctrl_data *hccd = hccd_alloc_and_init();
	if (hccd == NULL)	return -ENOMEM;

	hccd->m2m_cb.user_data = hccd;
	hccd->m2m_cb.cmd = cmd;
	app_uart_m2m_callback_add(&hccd->m2m_cb, data_ack_handler, hccd->m2m_cb.cmd);

	if (path == NULL) {
		strcpy(hccd->path, ROOT_PATH);
		LOG_WRN("path is NULL, setting to ROOT_PATH %s", ROOT_PATH);
	}
	if (strlen(path) > SETTINGS_FULLPATH_LEN_MAX) {
		strcpy((char*)path, ROOT_PATH);
		LOG_WRN("path length is too big, setting to ROOT_PATH %s", ROOT_PATH);
	}

	strcpy(hccd->path, path);

	/* start html generate and send thread */
	m_htmlgen_tid = k_thread_create(&m_htmlgen_data,
			m_htmlgen_stack,
			K_THREAD_STACK_SIZEOF(m_htmlgen_stack),
			html_gen_and_send_thread, hccd, NULL, NULL, APP_THREAD_PRIO_HTMLGEN,
			0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_htmlgen_tid, APP_THREAD_NAME_HTMLGEN);
#endif

	return ret;
}
