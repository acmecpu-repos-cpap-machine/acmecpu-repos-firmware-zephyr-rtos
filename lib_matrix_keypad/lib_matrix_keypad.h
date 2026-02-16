/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 31-Aug-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_LIB_LIB_MATRIX_KEYPAD_LIB_MATRIX_KEYPAD_H_
#define SRC_LIB_LIB_MATRIX_KEYPAD_LIB_MATRIX_KEYPAD_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

typedef enum {
	KEY_DEASSERTED = 0,				/* key is in released state */
	KEY_ASSERTED,					/* key is in pressed state */
	KEY_UNKNOWN,
} LIB_MK_PRESS_STATUS;

struct key_detect {
	bool cont_detection;			/* continue the detection process to validate a full key press */
	int64_t start_time;				/* tick time when a key press detection started */

};
struct libmk_key_data {
	int key_id;						/* unique ID of a key, this will be sent to the application when a key is pressed */
	struct gpio_dt_spec dev_row;	/* gpio_dt_spec of the pin connected to a row of the keypad */
	struct gpio_dt_spec dev_col;	/* gpio_dt_spec of the pin connected to a column of the keypad */
	bool has_intr;					/* whether interrupt should be enabled on the input pins */
	struct key_detect det;			/* used by the library to detect a key press, user must not modify this */
};

/**
 * @brief	function pointer data type for application callback
 * @note	the callback must be non blocking
 * @param key_id
 */
typedef void (*libmk_callback_handler_t)(int key_id);

/**
 * @brief		function to register an callback which will get called each time a key gets pressed
 * @param cb	callback function type libmk_callback_handler_t
 * @return		0
 */
int lib_mk_callback_register(libmk_callback_handler_t cb);

/**
 * @brief 	initialize the matrix keypad. This function will configure the pins as
 * 			input / output based on the configuration CONFIG_MATRIX_KEYPAD_ROW_OUTPUT and
 * 			CONFIG_MATRIX_KEYPAD_COL_OUTPUT.
 * @note	a valid 2D array must be provided else the behavior is undetermined
 * @param pkd	a 2D array representing the physical matrix keypad
 * 				CONFIG_MATRIX_KEYPAD_MAX_ROWS and CONFIG_MATRIX_KEYPAD_MAX_COLS should be set according
 * 				to the number of keys available in the physical keypad
 * @return	0 SUCCESS
 * 			-ve FAIL
 */
int lib_mk_init(struct libmk_key_data pkd[CONFIG_MATRIX_KEYPAD_MAX_ROWS][CONFIG_MATRIX_KEYPAD_MAX_COLS]);

#endif /* SRC_LIB_LIB_MATRIX_KEYPAD_LIB_MATRIX_KEYPAD_H_ */
