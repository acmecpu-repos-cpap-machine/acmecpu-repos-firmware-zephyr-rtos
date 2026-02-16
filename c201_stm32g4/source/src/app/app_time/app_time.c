/*
 * Copyright (c) 2021 Acme CPU
 */


#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_time);

#include "app_time/app_time.h"
#include "app_time/bsp_time.h"
#include "lib_events/lib_events.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"
#include "app_settings/app_settings_paths.h"

#define MINIMUM_SECONDS		CONFIG_TIME_MIN_VALUE_SECONDS
#define SYNC_PERIOD_SEC		(4)	/* in seconds */

struct app_time {
	struct k_sem lock;

	/* Timer structure used for maintaining a software clock */
	struct k_timer sync_timer;

	/* Work queue thread for synchronizing the software clock */
	struct k_work worker;

	/* the software clock time to be maintained */
	time_t time;

	/* timer status */
	uint32_t tstat;

	/* events the app_time wants to get notified */
	struct lib_events_callback suspend_event;
	struct lib_events_callback resume_event;
	struct lib_events_callback poweroff_event;
};

static struct app_time m_time;

static void sync_worker(struct k_work *work) {
	struct app_time *atime = CONTAINER_OF(work, struct app_time, worker);

	LOG_DBG("sync_worker called");

	time_t tmp;
	if (bsp_time_value_get_time(&tmp)) {
		LOG_ERR("unable to get time value!");
		return;
	}

	k_sem_take(&atime->lock, K_FOREVER);
	atime->time = tmp;
	k_sem_give(&atime->lock);
}

static void sync_timer(struct k_timer *tmr) {
	struct app_time *atime = CONTAINER_OF(tmr, struct app_time, sync_timer);
	LOG_DBG("sync_timer fired");

	k_sem_take(&atime->lock, K_FOREVER);
	++atime->time;
	k_sem_give(&atime->lock);

	if ((++m_time.tstat % SYNC_PERIOD_SEC) == 0) {
		m_time.tstat = 0;
		/* synchronize the software clock with the hardware clock every SYNC_PERIOD_SEC */
		k_work_submit(&atime->worker);
	}
}

static void sync_timer_stop_handler(struct k_timer *tmr) {
//	struct app_time *atime = CONTAINER_OF(tmr, struct app_time, sync_timer);

	LOG_INF("sync_timer stopped");
}

static void suspend_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch(event) {
	case LIB_EVENT_SUSPEND:
		k_timer_stop(&m_time.sync_timer);
		break;
	case LIB_EVENT_RESUME:
		bsp_time_value_get_time(&m_time.time);
		k_timer_start(&m_time.sync_timer, K_SECONDS(1), K_SECONDS(1));
		break;
	case LIB_EVENT_POWER_OFF:
		k_timer_stop(&m_time.sync_timer);
		bsp_time_poweroff_rtc();
		break;
	default:
		break;
	}
}

time_t app_time_value_get_secs() {
	return m_time.time;
}

int app_time_value_get(struct tm* tm) {
	if (tm == NULL) {
		return -EINVAL;
	}

	struct tm* ptm;
	time_t tmp = m_time.time;

	ptm = gmtime(&tmp);
	if (ptm) {
		memcpy(tm, ptm, sizeof(struct tm));
	} else {
		return -1;
	}

	return 0;
}

int app_time_value_set(struct tm* tm) {
	int ret=0;

	ret = bsp_time_value_set(tm);
	if (!ret) {
		k_sem_take(&m_time.lock, K_FOREVER);
		ret = bsp_time_value_get_time(&m_time.time);
		if (!ret) {
			k_timer_start(&m_time.sync_timer, K_SECONDS(1), K_SECONDS(1));
		} else {
			LOG_ERR("unable to get time value!");
		}
		k_sem_give(&m_time.lock);
	}

	return ret;
}

static uint8_t val_from_path(const char *path) {
	uint8_t part = DATE_TIME_ENUM_MAX;
	if (strstr(path, SETTINGS_KEY_YR)) part = YEAR;
	else if (strstr(path, SETTINGS_KEY_MON)) part = MONTH;
	else if (strstr(path, SETTINGS_KEY_DAY)) part = DAY;
	else if (strstr(path, SETTINGS_KEY_HR)) part = HOUR;
	else if (strstr(path, SETTINGS_KEY_MIN)) part = MINUTE;
//	else if (strstr(path, SETTINGS_KEY_SEC)) part = SECOND;
	return part;
}

int app_time_get_from_settings(const char *path, void *dest, size_t len)
{
	struct setting_value *dt_val = (struct setting_value *)dest;
	struct tm dt;
	int ret=0;

	uint8_t part = val_from_path(path);
	if (part == DATE_TIME_ENUM_MAX)	return -1;

	ret = app_time_value_get(&dt);
	switch (part) {
	case YEAR:
		dt_val->val1 = dt.tm_year + 1900;
		break;
	case MONTH:
		dt_val->val1 = dt.tm_mon + 1;
		break;
	case DAY:
		dt_val->val1 = dt.tm_mday;
		break;
	case HOUR:
		dt_val->val1 = dt.tm_hour;
		break;
	case MINUTE:
		dt_val->val1 = dt.tm_min;
		break;
//	case SECOND:
//		dt_val->val1 = dt.tm_sec;
//		break;
	default:
		break;
	}
	return ret;
}

int app_time_change_from_settings(const char *path, void *dest, size_t len)
{
	struct setting_value *dt_val = (struct setting_value *)dest;
	struct tm dt;
	int ret=0;

	uint8_t part = val_from_path(path);
	if (part == DATE_TIME_ENUM_MAX)	return -1;

	ret = app_time_value_get(&dt);
	switch (part) {
	case YEAR:
		dt.tm_year = dt_val->val1 - 1900;
		break;
	case MONTH:
		dt.tm_mon = dt_val->val1 - 1;
		break;
	case DAY:
		dt.tm_mday = dt_val->val1;
		break;
	case HOUR:
		dt.tm_hour = dt_val->val1;
		break;
	case MINUTE:
		dt.tm_min = dt_val->val1;
		break;
//	case SECOND:
//		dt.tm_sec = dt_val->val1;
//		break;
	default:
		break;
	}
	ret = app_time_value_set(&dt);
	return ret;
}

void app_time_html_formatted_date_get(char *date)
{
	int idx=0;
	struct setting_value dt_val;

	int ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_YR, &dt_val, sizeof(struct setting_value));
	idx += sprintf(date+idx, "%d-", dt_val.val1);	// year

	ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_MON, &dt_val, sizeof(struct setting_value));
	if (dt_val.val1 < 10)
		idx += sprintf(date+idx, "0%d-", dt_val.val1);	// mon
	else
		idx += sprintf(date+idx, "%d-", dt_val.val1);	// mon

	ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_DAT_DAY, &dt_val, sizeof(struct setting_value));
	if (dt_val.val1 < 10)
		idx += sprintf(date+idx, "0%d", dt_val.val1);	// day
	else
		idx += sprintf(date+idx, "%d", dt_val.val1);	// day
}

void app_time_html_formatted_time_get(char *time)
{
	int idx=0;
	struct setting_value tm_val;
	int ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_TIM_HR, &tm_val, sizeof(struct setting_value));
	if (tm_val.val1 < 10)
		idx += sprintf(time+idx, "0%d:", tm_val.val1);	// mon
	else
		idx += sprintf(time+idx, "%d:", tm_val.val1);	// mon

	ret = app_settings_load_single(SETTINGS_KEY_FULL_DS_TIM_MIN, &tm_val, sizeof(struct setting_value));
	if (tm_val.val1 < 10)
		idx += sprintf(time+idx, "0%d", tm_val.val1);	// day
	else
		idx += sprintf(time+idx, "%d", tm_val.val1);	// day
}

int app_time_init() {
	int ret=0;

	/* initialize the rtc */
	ret = bsp_time_init();
	if (ret) {
		LOG_ERR("bsp_time_init failed");
		return -1;
	}

	/* initialize the software time */
	ret = bsp_time_value_get_time(&m_time.time);
	if (ret) {
		LOG_ERR("unable to get initial time value!");
		return -1;
	}

	m_time.tstat = 0;

	/* add application event callbacks to get notified */
	ret = lib_events_callback_add(&m_time.suspend_event, suspend_event_handler, LIB_EVENT_SUSPEND);
	ret = lib_events_callback_add(&m_time.resume_event, suspend_event_handler, LIB_EVENT_RESUME);
	ret = lib_events_callback_add(&m_time.poweroff_event, suspend_event_handler, LIB_EVENT_POWER_OFF);

	/* initialize the lock */
	k_sem_init(&m_time.lock, 1, 1);

	/* Prepare interrupt worker */
	k_work_init(&m_time.worker, sync_worker);

	/* Start periodic timer that expires once every second,
	 * this is used to maintain the software clock
	 * */
	k_timer_init(&m_time.sync_timer, sync_timer, sync_timer_stop_handler);
	k_timer_start(&m_time.sync_timer, K_SECONDS(1), K_SECONDS(1));

	/* Check if the current time is ok or not */
	if (m_time.time < MINIMUM_SECONDS) {
		/* report time not set event, so other modules can set the time
		 * we will continue with the current time till a new one gets set */
		lib_events_report_event(LIB_EVENT_TIME_NOT_SET);
	}

	return ret;
}
