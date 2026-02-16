/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/settings/settings.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_settings);

#include "app_settings/app_settings.h"
#include "app_settings/app_settings_multi.h"
#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_utils.h"

static int find_and_save_setting(const char *setting_key, const char* setting_val)
{
	int ret = 0;
	/* get the index from the lookup table array */
	int idx = app_settings_lookup_array_idx_get(SETTINGS_COUNT_MAX, setting_key, g_sdata);
	if (idx < 0) {
		LOG_ERR("Key %s not found", setting_key);
		return -1;
	}

	const struct app_settings_data *asd = &g_sdata[idx];
	struct setting_value_options *options = asd->options;
	struct app_settings_value const *op_val = options->op_val;

	ret = -2;
	for (int i=0; i<options->num_options; i++) {
//		if (!strcmp(op_val[i].key, setting_val)) {
		if (!strcasecmp(op_val[i].key, setting_val)) {
			struct setting_value *val = (struct setting_value *) &op_val[i].val;

			/* pass the key and value object to save it */
			ret = app_settings_save_single_with_retry(setting_key, val,
					sizeof(struct setting_value), 10, false);
			if (ret < 0) {
				LOG_ERR(
						"app_settings_save_single_with_retry failed for key = %s, ret = %d",
						setting_key, ret);
			}
			break;
		}
	}
	if (ret != 0) {
		LOG_ERR("no match for key = %s, val = %s", setting_key, setting_val);
	}
	return ret;
}

int app_settings_multi_save()
{
	int ret = 0;
	/* open the file */
	LOG_INF("opening file %s", SETTINGS_DNL_CURR_FILE_PATH);
	struct fs_file_t zfp;
	fs_file_t_init(&zfp);
	ret = fs_open(&zfp, SETTINGS_DNL_CURR_FILE_PATH, FS_O_READ);
	if (ret != 0) {
		LOG_ERR("file %s open failed!", SETTINGS_DNL_CURR_FILE_PATH);
		return ret;
	}

	char ch;
	char line[SETTINGS_CSV_LINE_LEN_MAX] = {0x00};
	char key[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
	char value[SETTINGS_VALUE_LEN_MAX] = {0x00};
	ssize_t rd_len=0;
	int i=0, line_available=0;

	do {
		rd_len = fs_read(&zfp, &ch, 1);
		if (rd_len == 1) {
			if (ch != '\n') {	// read until new line character
				line[i] = ch;
				i++;
			} else if (ch == '\n') {	// copy the new line character also
				line[i] = ch;
				i++;
				line_available = 1;
			}

			if (line_available) {	// when line is copied, process it
				line_available = 0;

				char *tok = strtok(line, ",");	// key
				if (tok != NULL) {
					strcpy(key, tok);
					tok = strtok(NULL, "\r\n");	// value
					if (tok != NULL) {
						strcpy(value, tok);

						ret = find_and_save_setting(key, value);
						if (ret != 0) {
							LOG_ERR("find_and_save_setting failed, checking next, ret = %d", ret);
						}
					}
				}
				i = 0;
				memset(line, 0x00, sizeof(line));
				memset(key, 0x00, sizeof(key));
				memset(value, 0x00, sizeof(value));
			}
		} else {
			// no more data
			break;
		}
	} while (1);

	if (ret == 0)
		LOG_INF("*** all settings have been saved, now reboot to load them");

	fs_close(&zfp);
	return ret;
}




