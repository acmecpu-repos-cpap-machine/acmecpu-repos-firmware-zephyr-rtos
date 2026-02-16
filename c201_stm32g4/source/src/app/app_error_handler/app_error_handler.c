/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 28-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <stddef.h>
#include "app_errors.h"

const struct app_error_map m_app_errors[APP_ERR_MAX] =
{
		{APP_ERR_NO_ERROR, "no error"},
		{APP_ERR_BLOWER_OVERLOAD, "overload"},
		{APP_ERR_BLOWER_OVERHEAT, "overheat"},
		{APP_ERR_MASK_LEAK, "leak"},
		{APP_ERR_NO_MASK, "no mask"},
};

int app_error_handler_init() {
	int ret = 0;

	return ret;
}

const char* app_error_code_to_msg(APP_ERRORS err_code)
{
	if (err_code >= APP_ERR_MAX)
		return NULL;
	return m_app_errors[err_code].err_msg;
}
