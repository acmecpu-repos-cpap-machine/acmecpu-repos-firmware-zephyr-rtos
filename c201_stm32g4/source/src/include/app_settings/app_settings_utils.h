/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 21-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_UTILS_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_UTILS_H_

#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "app_settings/app_settings_data.h"

/**
 * @brief	Searches and returns the array index of a setting data from the name (fullpath)
 * 			Here the array is "g_sdata"
 * @param num_settings	number of settings available in the g_sdata array
 * @param fullpath		name of the settings to search
 * @param asd			pointer to g_sdata
 * @return	0 ot +ve	index of the array
 * 			-1			if name not found
 */
static inline int app_settings_lookup_array_idx_get(int num_settings, const char *fullpath,
		struct app_settings_data const *asd) {
	int idx = -1;
	for (int i=0; i<num_settings; i++) {
		if (!strcmp(fullpath, (asd+i)->fullpath)) {
			idx = i;
			break;
		}
	}

	return idx;
}

/**
 * @brief	Gets the value of a setting from the storage and converts it into string.
 * 			This function is useful where the value should be printed as a human readable value
 *
 * @param asd[in]		pointer to a settings object whose value will be loaded
 * @param srv[out]		pointer to settings value object to be populated
 * @param read_cb[in]	function provided to read the data from the backend.
 * @param cb_arg[in]	arguments for the read function provided by the backend
 * @return	0	SUCCESS
 * 			-1	FAIL
 */
int app_settings_value_get(struct app_settings_data const *asd,
								struct settings_runtime_value *srv,
								settings_read_cb read_cb, void *cb_arg);

/**
 * @brief	Allocates memory for all settings which has displayable flag set to true
 *
 * @param num_settings[in]	total number of settings
 * @param sdv[in/out]		pointer to g_disp_val
 * @return
 */
int app_settings_mem_alloc_displayable_val(int num_settings, struct settings_runtime_value *sdv);

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_UTILS_H_ */
