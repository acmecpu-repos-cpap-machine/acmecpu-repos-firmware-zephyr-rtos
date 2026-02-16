/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_TIME_BSP_TIME_H_
#define SRC_INCLUDE_APP_TIME_BSP_TIME_H_

#include <time.h>

int bsp_time_poweroff_rtc();

int bsp_time_value_get_time(time_t *time);

int bsp_time_value_get_tm(struct tm* tm);

int bsp_time_value_set(struct tm* tm);

int bsp_time_init();

#endif /* SRC_INCLUDE_APP_TIME_BSP_TIME_H_ */
