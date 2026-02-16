/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_H_

#include <stdio.h>
#include <stdint.h>
#include "app_settings_paths.h"
#include "app_settings/app_settings_data.h"
//#include "app_setting_values.h"
#include "app_storage/app_storage.h"

#define SETTINGS_ROOT	0
#define SETTINGS_NEEDED	0

/* log file constants */
#define SETTINGS_LOG_DIRECTORY_NAME				CONFIG_APP_SETTINGS_LOG_DIR_NAME //"settings"
#define SETTINGS_LOG_DIRECTORY_PATH				APP_STORAGE_MOUNT_POINT_FLASH "/" SETTINGS_LOG_DIRECTORY_NAME
#define SETTINGS_LOG_CURR_FILE_NAME				CONFIG_APP_SETTINGS_LOG_CURR_FILE_NAME //"settings.dat"
#define SETTINGS_LOG_CURR_FILE_PATH				SETTINGS_LOG_DIRECTORY_PATH "/" SETTINGS_LOG_CURR_FILE_NAME
#define SETTINGS_LOG_CURR_FILE_MAX_SIZE_BYTES	(CONFIG_APP_SETTINGS_LOG_CURR_FILE_SIZE_MAX_MB * 1000000)
#define SETTINGS_LOG_MAX_FILE_COUNT				CONFIG_APP_SETTINGS_LOG_MAX_FILE_COUNT

/* settings all download file constants */
#define SETTINGS_DNL_DIRECTORY_NAME				CONFIG_APP_SETTINGS_DNL_DIR_NAME //"settings"
#define SETTINGS_DNL_DIRECTORY_PATH				APP_STORAGE_MOUNT_POINT_FLASH "/" SETTINGS_DNL_DIRECTORY_NAME
#define SETTINGS_DNL_CURR_FILE_NAME				CONFIG_APP_SETTINGS_DNL_CURR_FILE_NAME //"settingsdnl.csv"
#define SETTINGS_DNL_CURR_FILE_PATH				SETTINGS_DNL_DIRECTORY_PATH "/" SETTINGS_DNL_CURR_FILE_NAME
#define SETTINGS_DNL_CURR_FILE_MAX_SIZE_BYTES	(CONFIG_APP_SETTINGS_DNL_FILE_SIZE_MAX_MB * 1000000)
#define SETTINGS_DNL_MAX_FILE_COUNT				CONFIG_APP_SETTINGS_DNL_MAX_FILE_COUNT

/**
 * Callback function used for getting a subtree by calling app_settings_subtree_get()
 */
typedef int (*settings_subtree_get_cb)(const char *prev_path, const char *pkg_key);

int app_settings_init();

int app_settings_load();

int app_settings_save();

int app_settings_load_single(const char *name, void *dest, size_t len);

/**
 * @brief	Save a setting value into storage, if CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM
 * 			is set then the value is saved to appropriate variable of g_disp_val
 * @param name	fullpath of the setting
 * @param dest	data to be saved
 * @param len	length of the data to be saved
 * @param report_event	if true then LIB_EVENT_SETTINGS_CHANGED event will be fired
 * @return
 * 		0 if success
 * 		-ENOENT	if g_disp_val was not found
 * 		other -ve if could not save into storage
 */
int app_settings_save_single(const char *name, void *dest, size_t len, bool report_event);

/**
 * @brief	Save a setting value into storage, if CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM
 * 			is set then the value is saved to appropriate variable of g_disp_val. This function
 * 			retries a number of times if saving fails
 * @param name	fullpath of the setting
 * @param dest	data to be saved
 * @param len	length of the data to be saved
 * @param retry_max	number of times to be retried
 * @param report_event if true then LIB_EVENT_SETTINGS_CHANGED event will be fired
 * @return
 * 		0 if success
 * 		-ENOENT	if g_disp_val was not found
 * 		other -ve if could not save into storage
 */
int app_settings_save_single_with_retry(const char *name, void *dest, size_t len, int retry_max, bool report_event);

int app_settings_subtree_get(const char *name, settings_subtree_get_cb cb);

int app_settings_array_idx_get(const char *fullpath);

int app_settings_datatype_get(int idx);

int app_settings_displayable_get(int idx);

int app_settings_option_key_to_val(struct setting_value_options *options,
									const char *key,
									struct setting_value *out_val);

int app_settings_option_val_to_key(struct setting_value_options *options,
									struct setting_value *in_val,
									char *out_key);

/*
 * Function to get array index of a setting value from the saved value
 * This function only works for settings having datatype of struct setting_value
 */
int app_settings_value_to_option_idx(const char* settings_path, struct setting_value_options *options,
											int *pidx);

struct app_settings_data const* app_settings_data_obj_get(int idx);

/**
 * @brief	the latest changed setting
 *
 * @param	settings_path[out]	the settings path which was changed last will be copied here
 * 								buffer must be allocated by caller
 *
 * @return
 * 			0		SUCCESS
 * 			-ve		FAIL
 */
int app_settings_changed_latest_get(char *settings_path);

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_H_ */
