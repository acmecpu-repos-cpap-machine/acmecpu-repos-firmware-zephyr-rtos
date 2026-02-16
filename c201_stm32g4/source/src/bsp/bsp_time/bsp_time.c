/*
 * Copyright (c) 2021 Acme CPU
 */


#include <time.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/timeutil.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_time);

#include "acpu_c201_modules.h"
#if CONFIG_MICROCRYSTAL_RV3028
#include "microcrystal_rv3028.h"
#endif

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201 || CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
	#define RTC_NODE_ID DT_INST(0, microcrystal_rv3028)
#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	#if defined(CONFIG_RTC)
		#define RTC_NODE_ID DT_INST(0, st_stm32_rtc)
	#endif
#endif

int bsp_time_poweroff_rtc() {
//	const struct device *rtc = device_get_binding(ACPU_C201_MOD_NAME_RTC);
	const struct device *rtc = DEVICE_DT_GET(RTC_NODE_ID);
	if (!rtc) {
		return -1;
	}
#if CONFIG_MICROCRYSTAL_RV3028
	rv3028_activate_vbackup(rtc);
#endif
	return 0;
}

int bsp_time_value_get_time(time_t *time) {
	int ret=0;
	time_t ticks=0;

	if (time == NULL) {
		return -EINVAL;
	}

//	const struct device *rtc = device_get_binding(ACPU_C201_MOD_NAME_RTC);
	const struct device *rtc = DEVICE_DT_GET(RTC_NODE_ID);
	if (!rtc) {
		return -1;
	}
#if (CONFIG_RTC)
	struct rtc_time datetime_get;
	ret = rtc_get_time(rtc, &datetime_get);
	if (!ret) {
		ticks = timeutil_timegm((struct tm*) (&datetime_get));
	}
#else
	ret = counter_get_value(rtc, (uint32_t*)&ticks);
#endif
	if (!ret) {
		*time = ticks;
	}
	return ret;
}

int bsp_time_value_get_tm(struct tm* tm) {
	int ret=0;
	time_t ticks=0;
	struct tm* ptm;

	if (tm == NULL) {
		return -EINVAL;
	}

	ret = bsp_time_value_get_time(&ticks);
	if (!ret) {
		ptm = gmtime(&ticks);
		if (ptm) {
			memcpy(tm, ptm, sizeof(struct tm));
		} else {
			ret = -1;
		}
	}

	return ret;
}

int bsp_time_value_set(struct tm* tm) {
	int ret=0;

	if (tm == NULL) {
		return -EINVAL;
	}

//	const struct device *rtc = device_get_binding(ACPU_C201_MOD_NAME_RTC);
	const struct device *rtc = DEVICE_DT_GET(RTC_NODE_ID);
	if (!rtc) {
		return -1;
	}

#if CONFIG_MICROCRYSTAL_RV3028
	ret = rv3028_set_time(rtc, tm);
#elif (CONFIG_RTC)
	struct rtc_time datetime_set;
	time_t timer_set = timeutil_timegm(tm);
	gmtime_r(&timer_set, (struct tm *)(&datetime_set));
	ret = rtc_set_time(rtc, &datetime_set);
#else
	tm = NULL;
	return -ENOTSUP;
#endif

	return ret;
}

int bsp_time_init()
{
	int ret = 0;
//#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
//	const struct device *rtc = DEVICE_DT_GET(RTC_NODE_ID);
//	if (rtc == NULL) {
//		LOG_ERR("device %s not found", "RTC");
//		return -ENODEV;
//	}
//	if (!device_is_ready(rtc)) {
//		LOG_ERR("device not ready.\n");
//		return -1;
//	}
//
//	ret = counter_start(rtc);
//#endif
	return ret;
}
