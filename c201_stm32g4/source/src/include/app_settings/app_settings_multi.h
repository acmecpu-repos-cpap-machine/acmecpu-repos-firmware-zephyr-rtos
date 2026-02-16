/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 27-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_MULTI_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_MULTI_H_

#define SETTINGS_VALUE_LEN_MAX		100
#define SETTINGS_CSV_LINE_LEN_MAX	200

/**
 * @brief	This function reads the saved settingsdnl.csv file at /lfs/settings
 * 			directory and saves each value against each keys present in the file
 * @return
 * 	0	success
 * 	-1	failed to save the setting
 */
int app_settings_multi_save();

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_MULTI_H_ */
