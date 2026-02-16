/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DISPLAY_DATA_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DISPLAY_DATA_H_

#include <stdint.h>
//#include "app_setting_values.h"
#include "app_settings_paths.h"
#include "app_settings_value.h"

#define SETTINGS_COUNT_MAX				45
#define SETTINGS_MODE_PARAM_MAX			14

#define SETTINGS_PKG_NAME_LEN_MAX		15
#define SETTINGS_DISP_NAME_LEN_MAX		32
#define SETTINGS_STR_VAL_LEN_MAX		32

typedef enum {
	SETTING_DATATYPE_NONE,
	SETTING_DATATYPE_UINT8,
	SETTING_DATATYPE_UINT16,
	SETTING_DATATYPE_UINT32,
	SETTING_DATATYPE_INT,
	SETTING_DATATYPE_DOUBLE,
	SETTING_DATATYPE_CHAR,
	SETTING_DATATYPE_STRING,
	SETTING_DATATYPE_DATE,
	SETTING_DATATYPE_TIME,
	SETTING_DATATYPE_SETTING_VALUE,
} SETTING_DATATYPE;

typedef enum {
	GUI_OBJ_NONE,
	GUI_OBJ_SWITCH,
	GUI_OBJ_SLIDER,
	GUI_OBJ_SPINBOX,
	GUI_OBJ_ROLLER,
	GUI_OBJ_TEXTBOX,
	GUI_OBJ_LABEL,
	GUI_OBJ_DROPDOWN
} SETTING_LVOBJ;

typedef enum {
	DISPLAY_STAT_NO=0,			// do not display
	DISPLAY_STAT_YES,			// display
	DISPLAY_STATE_CONDITIONAL	// display depending on some condition
} DISPLAY_STATUS;

//struct settings_display_item {
////	const char pkg_name[SETTINGS_PKG_NAME_LEN_MAX];
//	const char fullpath[SETTINGS_FULLPATH_LEN_MAX];
//	const char disp_name[SETTINGS_DISP_NAME_LEN_MAX];
//	bool display_stat;
//	uint8_t datatype;
//	uint8_t lvobj_type;
//};

struct menu_mode_param {
	uint16_t mode_bitmask;
	char override_name[SETTINGS_DISP_NAME_LEN_MAX];
//	struct setting_value range_min;
//	struct setting_value range_max;
	uint8_t range_min_idx;
	uint8_t range_max_idx;
};

struct settings_display_item {
	const char fullpath[SETTINGS_FULLPATH_LEN_MAX];
	char disp_name[SETTINGS_DISP_NAME_LEN_MAX];
	uint8_t display_stat;
	bool editable;
	uint8_t datatype;
	uint8_t lvobj_type;
	uint8_t param_count;
	struct menu_mode_param const *param_data;
	struct gui_param_metadata *gui_data;
	settings_extra_func extra_func;
};

extern const struct settings_display_item g_display_item[SETTINGS_COUNT_MAX];

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DISPLAY_DATA_H_ */
