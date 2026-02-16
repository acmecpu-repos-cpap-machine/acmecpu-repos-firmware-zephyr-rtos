/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 21-Nov-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_settings);

#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings_value.h"
#include "app_net/app_net.h"

/*
 * Function to get array index of a setting value from the saved value
 * This function only works for settings having datatype of struct setting_value
 */
static int find_option_idx_from_value(const char* settings_path,
										struct setting_value_options *options,
										int *pidx,
										settings_read_cb read_cb, void *cb_arg)
{
	struct setting_value val;
	*pidx = 0;
	int match = 0;
//	int ret = app_settings_load_single(settings_path, &val, sizeof(struct setting_value));
	int ret = read_cb(cb_arg, &val, sizeof(struct setting_value));
	if (ret >= 0) {
		for (int i = 0; i < options->num_options; i++) {
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

int app_settings_value_get(struct app_settings_data const *asd,
								struct settings_runtime_value *srv,
								settings_read_cb read_cb, void *cb_arg)
{
	int ret=0;

	if ((asd == NULL) || (srv == NULL))	return -1;

	uint8_t datatype = asd->datatype;
	uint32_t size = asd->size;

//	if (str_val == NULL)	return -1;

	if (datatype == SETTING_DATATYPE_SETTING_VALUE) {
		int idx;
		ret = find_option_idx_from_value(asd->fullpath, asd->options, &idx, read_cb, cb_arg);
		if (ret == 0) {
			struct app_settings_value const *data = asd->options->op_val;
			memcpy(srv->val, &data[idx].val, size);
			strcpy(srv->disp_val, data[idx].key);
//			strcpy(str_val, data[idx].key);
		}
	} else if (datatype == SETTING_DATATYPE_STRING) {
		ret = read_cb(cb_arg, srv->val, size);
		if (ret >= 0) {
			memcpy(srv->disp_val, srv->val, size);
//			ret = read_cb(cb_arg, str_val, size);
		}
	} else if ((datatype == SETTING_DATATYPE_DATE) || (datatype == SETTING_DATATYPE_TIME)) {
		struct setting_value val;
		ret = read_cb(cb_arg, &val, size);
		if (ret >= 0) {
			memcpy(srv->val, &val, size);
			sprintf(srv->disp_val, "%d", val.val1);
		}
//		ret = read_cb(cb_arg, &val, size);
//		sprintf(str_val, "%d", val.val1);
	} else if(datatype == SETTING_DATATYPE_WIFI_STA_CFG) {
		struct wifi_sta_config wsc;
		ret = read_cb(cb_arg, &wsc, size);
		if (ret >= 0) {
			memcpy(srv->val, &wsc, size);
			// the character ? is not allowed as a SSID name so we use it as a delimiter here.
			// Ref: https://www.cisco.com/assets/sol/sb/WAP321_Emulators/WAP321_Emulator_v1.0.0.3/help/Wireless05.html#:~:text=The%20SSID%20can%20be%20any,%5C%2C%20%5D%2C%20and%20%2B.
			sprintf(srv->disp_val, "%s?%s", wsc.ssid, wsc.pwd);
//			sprintf(str_val, "%s?%s", wsc.ssid, wsc.pwd);
		}
	} else if (datatype == SETTING_DATATYPE_UINT32) {
		ret = read_cb(cb_arg, srv->val, size);
		if (ret >= 0) {
			uint32_t val_i = *(uint32_t*)srv->val;
			sprintf(srv->disp_val, "%d", val_i);
		}
	}
	else {
//		memset(str_val, 0x00, len);
	}
	return ret;
}

int app_settings_mem_alloc_displayable_val(int num_settings, struct settings_runtime_value *sdv)
{
	int ret = 0, mem_disp=0, mem_val=0;
	for (int i=0; i<num_settings; i++) {
		struct settings_runtime_value *ptr = (sdv+i);
		struct app_settings_data const *asd = ptr->settings_data;

		if ((asd->displayable > 0) && (asd->size > 0)) {
			ptr->disp_val = (char*)calloc(1, ptr->len_max);
			if (ptr->disp_val == NULL) {
				LOG_ERR("%s, calloc failed !!!", __func__);
				return -ENOMEM;
			}
			ptr->val = calloc(1, asd->size);
			if (ptr->disp_val == NULL) {
				LOG_ERR("%s, calloc failed !!!", __func__);
				return -ENOMEM;
			}
			mem_disp += ptr->len_max;
			mem_val += asd->size;
		} else if ((	(asd->datatype == SETTING_DATATYPE_DATE) ||
						(asd->datatype == SETTING_DATATYPE_TIME) ||
						(asd->datatype == SETTING_DATATYPE_WIFI_STA_CFG))
						&& (asd->size > 0)) {
			ptr->disp_val = (char*)calloc(1, ptr->len_max);
			if (ptr->disp_val == NULL) {
				LOG_ERR("%s, calloc failed !!!", __func__);
				return -ENOMEM;
			}
			ptr->val = calloc(1, asd->size);
			if (ptr->disp_val == NULL) {
				LOG_ERR("%s, calloc failed !!!", __func__);
				return -ENOMEM;
			}
			mem_disp += ptr->len_max;
			mem_val += asd->size;
		}
	}
	LOG_INF("Allocated mem_disp = %d bytes, mem_val = %d bytes", mem_disp, mem_val);
	return ret;
}





