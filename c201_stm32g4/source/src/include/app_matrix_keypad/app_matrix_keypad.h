/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 04-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_MATRIX_KEYPAD_APP_MATRIX_KEYPAD_H_
#define SRC_INCLUDE_APP_MATRIX_KEYPAD_APP_MATRIX_KEYPAD_H_

#include "lib_matrix_keypad/lib_matrix_keypad.h"

#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
#define MAX_ROWS	CONFIG_MATRIX_KEYPAD_MAX_ROWS
#define MAX_COLS	CONFIG_MATRIX_KEYPAD_MAX_COLS

#define KP_ROW_0	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_row1), gpios)
#define KP_ROW_1	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_row2), gpios)
#define KP_ROW_2	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_row3), gpios)

#define KP_COL_0	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_col1), gpios)
#define KP_COL_1	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_col2), gpios)
#define KP_COL_2	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_col3), gpios)
#define KP_COL_3	GPIO_DT_SPEC_GET(DT_NODELABEL(keymat_col4), gpios)

#define APP_MK_KEY_HOME		(1)
#define APP_MK_KEY_BACK		(2)
#define APP_MK_KEY_MIC		(3)
#define APP_MK_KEY_UP		(4)
#define APP_MK_KEY_LEFT		(5)
#define APP_MK_KEY_ENTER	(6)
#define APP_MK_KEY_RIGHT	(7)
#define APP_MK_KEY_DOWN		(8)
#define APP_MK_KEY_VOL_UP	(9)
#define APP_MK_KEY_VOL_DOWN	(10)
#define APP_MK_KEY_MUTE		(11)

/**
 * @brief initialize the matrix keypad application and the underlying lib_matrix_keypad
 * @return	0 SUCCESS
 * 			-ve FAIL
 */
int app_matrix_keypad_init();

#endif	/* (CONFIG_BOARD_E206) */

#endif /* SRC_INCLUDE_APP_MATRIX_KEYPAD_APP_MATRIX_KEYPAD_H_ */
