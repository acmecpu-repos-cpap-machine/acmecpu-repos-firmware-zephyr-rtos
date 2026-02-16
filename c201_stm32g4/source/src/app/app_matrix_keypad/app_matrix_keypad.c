/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 04-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_mat_key);

#include "app_matrix_keypad/app_matrix_keypad.h"
#include "lib_matrix_keypad/lib_matrix_keypad.h"


static struct libmk_key_data m_kd[MAX_ROWS][MAX_COLS] =
{
	{ {APP_MK_KEY_HOME, KP_ROW_0, KP_COL_0, false}, {APP_MK_KEY_BACK, KP_ROW_0, KP_COL_1, false}, {APP_MK_KEY_MIC, KP_ROW_0, KP_COL_2, false}, {APP_MK_KEY_UP, KP_ROW_0, KP_COL_3, false} },	// ROW 1
	{ {APP_MK_KEY_LEFT, KP_ROW_1, KP_COL_0, false}, {APP_MK_KEY_ENTER, KP_ROW_1, KP_COL_1, false}, {APP_MK_KEY_RIGHT, KP_ROW_1, KP_COL_2, false}, {APP_MK_KEY_DOWN, KP_ROW_1, KP_COL_3, false} },	// ROW 2
	{ {APP_MK_KEY_VOL_UP, KP_ROW_2, KP_COL_0, false}, {APP_MK_KEY_VOL_DOWN, KP_ROW_2, KP_COL_1, false}, {APP_MK_KEY_MUTE, KP_ROW_2, KP_COL_2, false}, {-1} },		// ROW 3
};


int app_matrix_keypad_init()
{
	int ret = lib_mk_init(m_kd);
	if (ret != 0) {
		LOG_ERR("lib_mk_init failed");
	}
	return ret;
}
