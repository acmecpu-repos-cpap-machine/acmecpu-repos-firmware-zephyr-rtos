/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 16-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_MENU_H_
#define SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_MENU_H_

#include "app_display.h"
#include "app_settings/app_settings_data.h"

#define SETTINGS_DISP_NAME_LEN_MAX		32
#define SETTINGS_STR_VAL_LEN_MAX		32

typedef enum {
	GUI_OBJ_NONE,
	GUI_OBJ_SWITCH,
	GUI_OBJ_SLIDER,
	GUI_OBJ_SPINBOX,
	GUI_OBJ_ROLLER,
	GUI_OBJ_TEXTBOX,
	GUI_OBJ_LABEL,
	GUI_OBJ_DROPDOWN
} MENU_GUI_OBJ;

/**
 * @brief: 	data structure that application should pass to GUI init functions
 * 			this structure contains essential data to build a GUI,
 *
 * 			e.g. the dynamic roller module can use this structure and
 * 			make 1 or more roller objects with options
 * */
struct gui_param_metadata {
//	uint8_t num_items;		// number of option the gui objects will have, e.g. num_items=10 makes 10 options on a roller/list
//	bool has_range;
//	struct app_settings_value const *data;	// the data of each object, e.g. this has the options for a roller
	struct setting_value_options *options;			// options a setting value can have, i.e. list of values
	int16_t selected_idx;		// this gets populated once the user selects an option
	const char *settings_path;	// the gui module internally calls settings function to save the selected data
	uint16_t settings_len;	// length of the data to be saved (not used)
};

/**
 * @brief:
 * Function pointer prototype for settings having extra functionality than normal on/off/value change
 * e.g. The Hotspot setting after turning on, needs to show some messages to the user.
 * This function should get called when the value of a setting is changed
 * */
typedef int (*menu_extra_func)(
							const char *display_name,
							const char *setting_path,
							app_display_key_cb prev_screen_cb
							);


struct app_display_menu {
	struct app_settings_data const *settings_data;
	struct gui_param_metadata *gui_data;
	uint8_t gui_obj_type;
	menu_extra_func extra_func;
};

extern const struct app_display_menu g_display_menu[SETTINGS_COUNT_MAX];

/* Function declarations */
void app_display_menu_init();

#endif /* SRC_INCLUDE_APP_DISPLAY_APP_DISPLAY_MENU_H_ */
