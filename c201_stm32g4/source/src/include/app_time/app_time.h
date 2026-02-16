/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_TIME_APP_TIME_H_
#define SRC_INCLUDE_APP_TIME_APP_TIME_H_

#include <time.h>

typedef enum {
	YEAR = 0, MONTH, DAY, HOUR, MINUTE, SECOND, DATE_TIME_ENUM_MAX
} DATE_TIME;


time_t app_time_value_get_secs();

int app_time_value_get(struct tm* tm);

int app_time_value_set(struct tm* tm);

int app_time_change_from_settings(const char *name, void *dest, size_t len);
int app_time_get_from_settings(const char *path, void *dest, size_t len);

void app_time_html_formatted_date_get(char *date);
void app_time_html_formatted_time_get(char *time);

int app_time_init();

#endif /* SRC_INCLUDE_APP_TIME_APP_TIME_H_ */
