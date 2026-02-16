/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 28-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_ERRORS_H_
#define SRC_INCLUDE_APP_ERRORS_H_

#define APP_ERR_MSG_SIZE_MAX	18
typedef enum {
	APP_ERR_NO_ERROR = 0,

	APP_ERR_BLOWER_OVERLOAD,
	APP_ERR_BLOWER_OVERHEAT,

	APP_ERR_MASK_LEAK,
	APP_ERR_NO_MASK,

	APP_ERR_MAX
} APP_ERRORS;

struct app_error_map {
	APP_ERRORS err_code;
	char err_msg[APP_ERR_MSG_SIZE_MAX];
};

const char* app_error_code_to_msg(APP_ERRORS err_code);
int app_error_handler_init();

#endif /* SRC_INCLUDE_APP_ERRORS_H_ */
