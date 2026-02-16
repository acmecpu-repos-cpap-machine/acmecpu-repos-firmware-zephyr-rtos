/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <time.h>
#include <stdint.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_battery);

#include "app_battery/app_battery.h"
#include "app_battery/bsp_battery.h"
#if CONFIG_APP_USBC_PD
#include "app_battery/bsp_ucpd.h"
#endif
#include "app_battery_level.h"
#if (CONFIG_BSP_MEMBRANE_SWITCH)
	#include "bsp_membrane_switch/bsp_membrane_switch.h"
#endif
#include "bsp_buzzer/bsp_buzzer.h"
#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
#include "app_led_notification.h"
#endif
#include "lib_events/lib_events.h"
#if (CONFIG_APP_DISPLAY)
	#include "c20x_screen_statusbar.h"
#endif
#include "app_analog/app_analog.h"
#include "app_utils/app_utils.h"
#if (CONFIG_APP_UCPD_EXT_CONTR)
	#include "app_ucpd/app_ucpd_ext_contr.h"
#endif
#include "app_settings/app_settings_paths.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"

#define APP_BATTERY_MIN_VALUE_MVOLTS	CONFIG_APP_BATTERY_MIN_VALUE_MVOLTS
#define APP_BATTERY_MAX_VALUE_MVOLTS	CONFIG_APP_BATTERY_MAX_VALUE_MVOLTS
#define READING_NORMALIZE_COUNT			CONFIG_APP_BATTERY_VALUE_NORMALIZE_COUNT
#define APP_CHARGING_CURR_PERCENT		CONFIG_APP_CHARGING_CURR_PERCENT_OF_AVAILABLE_CURRENT

typedef enum {
	DEVICE_PWR_STATE_RUNNING,
	DEVICE_PWR_STATE_SUSPENDED,
	DEVICE_PWR_STATE_UNKNOWN
} DEVICE_POWER_STATES;

/* static variables */
static DEVICE_POWER_STATES m_pwr_state = DEVICE_PWR_STATE_UNKNOWN;
#if (CONFIG_BSP_MEMBRANE_SWITCH)
static struct bsp_membr_callback power_cb_data;
static struct k_work m_long_press_worker;
#endif /*#if (CONFIG_BSP_MEMBRANE_SWITCH)*/
static struct lib_events_callback m_cb_suspend;
static struct lib_events_callback m_cb_resume;
static struct lib_events_callback m_cb_poweroff;
static struct lib_events_callback m_cb_ucpd_nego_done;
static bool m_power_down=false;
static bool m_charge_stat=false;	// true charging, false not charging
static int m_is_batt_connected = BSP_BATT_CONNECTED;

/*Global variable for battery state*/
int battery_removed = 1;

#if (CONFIG_BSP_MEMBRANE_SWITCH)
/* static functions, threads and callback handler */
static void long_press_detection_worker(struct k_work *work) {

	int64_t start_time = k_uptime_get();
	int64_t duration = 0;
	uint32_t ms_to_off=0;
	int ret=0;
	do {
		int64_t temp = start_time;
		duration = k_uptime_delta(&temp);
		if (duration > BSP_MEMBR_SWITCH_EDGE_LONG_PRESS_DURATION_MIN) {
			bsp_buzzer_play_switch_pressed();

			LOG_INF("SWITCH_PRESSED_NORMAL_EDGE for %s switch detected successfully",
								(BSP_MEMBR_SWITCH_LABEL_POWER));

			/* Report POWER_OFF event to the application events module */
			lib_events_report_event(LIB_EVENT_POWER_OFF);

			/* wait for the application events module to confirm power down */
			while (!m_power_down) {
				k_sleep(K_MSEC(100));
			}

			k_sleep(K_MSEC(500));

			/* schedule system off */
			ret = bsp_battery_schedule_system_off(&ms_to_off);
			if (!ret) {
				LOG_WRN("System off scheduled at %s", ("<timestamp>"));

				/* countdown to system off */
				LOG_WRN("System will turn off in approximately:");
#if 0
				while (1) {
					if ((--ms_to_off % 1000) == 0) {
						LOG_WRN("%d ms", ms_to_off);
					}
					if (ms_to_off == 0)	{
						bsp_buzzer_on();
						break;
					}
					k_sleep(K_MSEC(1));
				}
#endif
			} else {
				LOG_ERR("could not schedule system off");
			}

			break;
		}
		k_sleep(K_MSEC(10));
	} while (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_POWER, BSP_MEMBR_SWITCH_PIN_POWER) == SWITCH_ASSERTED);

	/* check if it was not a long press but a regular power button press */
	if (duration < BSP_MEMBR_SWITCH_EDGE_LONG_PRESS_DURATION_MIN) {
		/* power switch pressed, report to app_event module */
		switch (m_pwr_state) {
		case DEVICE_PWR_STATE_RUNNING:
			LOG_INF("DEVICE_PWR_STATE_RUNNING reporting LIB_EVENT_SUSPEND");
			lib_events_report_event(LIB_EVENT_SUSPEND);
			break;
		case DEVICE_PWR_STATE_SUSPENDED:
			LOG_INF("DEVICE_PWR_STATE_SUSPENDED reporting LIB_EVENT_RESUME");
			lib_events_report_event(LIB_EVENT_RESUME);
			break;
		case DEVICE_PWR_STATE_UNKNOWN:
			LOG_INF("DEVICE_PWR_STATE_UNKNOWN");
			break;
		}
	}
}

static void power_switch_cb_handler(struct bsp_membr_callback *cb,
		const char *switch_label, uint32_t pin, BSP_MEMBR_SWITCH_PRESSED_TYPE press_type) {

	/* start a worker thread to check for power switch long press */
	k_work_submit(&m_long_press_worker);
}
#endif	/*#if (CONFIG_BSP_MEMBRANE_SWITCH)*/

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_SUSPEND:
		LOG_INF("LIB_EVENT_SUSPEND");
		m_pwr_state = DEVICE_PWR_STATE_SUSPENDED;
		break;
	case LIB_EVENT_RESUME:
		LOG_INF("LIB_EVENT_RESUME");
		m_pwr_state = DEVICE_PWR_STATE_RUNNING;
		break;
	case LIB_EVENT_POWER_OFF:
		LOG_INF("LIB_EVENT_POWER_OFF");
		m_power_down = true;
		break;
	case LIB_EVENT_UCPD_SNK_NEGO_DONE:
	{
		LOG_INF("LIB_EVENT_UCPD_SNK_NEGO_DONE");
		uint32_t mvolts = 5000, max_curr_ma = 1000, oper_curr_ma = 1000;
		int ret = 0;
#if (CONFIG_APP_UCPD_EXT_CONTR)
		ret = app_ucpd_ext_contr_nego_power_get(&mvolts, &max_curr_ma, &oper_curr_ma);
#endif
		if (ret == 0) {
			int32_t max_chrg_ma = (max_curr_ma * APP_CHARGING_CURR_PERCENT) / 100;	// set charge current to a %age of max current
			LOG_INF("setting charging current to: %d", max_chrg_ma);
			ret = bsp_battery_ichrg_set(max_chrg_ma);
			if (ret) {
				LOG_ERR("could not set charging current, %d", ret);
			}
		}
	}
		break;
	default:
		LOG_INF("%d", event);
		break;
	}
}

void charge_status_cb_handler(BSP_BATTERY_CHARGER_EVENTS event)
{
//	int ret = 0;
	switch (event) {
	case BSP_BATT_CHRG_EVENT_ADAPTER_ATTACHED:
	{
		LOG_INF("BSP_BATT_CHRG_EVENT_ADAPTER_ATTACHED");
		lib_events_report_event(LIB_EVENT_CHARGER_ATTACHED);

		m_charge_stat = true;	// charger attached (charging)
#if (CONFIG_APP_DISPLAY)
		c20x_screen_statusbar_reload();		// update display
#endif
	}
		break;
	case BSP_BATT_CHRG_EVENT_ADAPTER_REMOVED:
	{
		LOG_INF("BSP_BATT_CHRG_EVENT_ADAPTER_REMOVED");
		lib_events_report_event(LIB_EVENT_CHARGER_REMOVED);
		m_charge_stat = false;	// charger removed (not charging)
#if (CONFIG_APP_DISPLAY)
		c20x_screen_statusbar_reload();		// update display
#endif
	}
		break;
	case BSP_BATT_CHRG_EVENT_CHARGE_COMPLETE:
	{
		LOG_INF("BSP_BATT_CHRG_EVENT_CHARGE_COMPLETE");
		bsp_battery_capacity_value_set(CONFIG_APP_BATTERY_MAX_CAPACITY_MAH);
		lib_events_report_event(LIB_EVENT_CHARGE_COMPLETE);
	}
		break;
	case BSP_BATT_CHRG_EVENT_FAULT:
	{
		LOG_INF("BSP_BATT_CHRG_EVENT_FAULT");

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
		app_led_show_fault();
#endif
	}
		break;
	default:
		break;
	}
}

static int check_battery_connected(int *is_battery_connected)
{
	/* check if battery is attached and enable charging */
	int32_t vbat_uv = 0;
	int ret = bsp_battery_vbat_get(&vbat_uv);
	if (ret)	return ret;

	vbat_uv *= 1000;
	LOG_DBG("vbat_uv = %d", vbat_uv);
	if (vbat_uv >= BSP_BATTERY_VBAT_MIN_UV) {
		ret = bsp_battery_charging_control(BSP_BATT_CHARGING_ENABLE);
		*is_battery_connected = BSP_BATT_CONNECTED;
	}
	else {
		ret = bsp_battery_charging_control(BSP_BATT_CHARGING_DISABLE);
		*is_battery_connected = BSP_BATT_DISCONNECTED;
	}
	return ret;
}

int app_battery_check_enable_charging()
{
	return check_battery_connected(&m_is_batt_connected);
}

int app_battery_level_get(uint8_t *batt_level)
{
	int ret = 0;
	uint8_t level = 0;
	int32_t batt_mvolts = 0;
	float batt_capacity=0;
	if (m_is_batt_connected == BSP_BATT_DISCONNECTED) {
		level = 0;
		LOG_ERR("battery not connected!!!");
		return -ENXIO;
	}
#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	//int32_t batt_mvolts = 0;
	int32_t batt_mvolts_sum = 0;

	app_analog_measure_en(APP_ANALOG_VBAT);

	/* read the battery mili volts and average it */
	for (uint32_t i = 0; i < READING_NORMALIZE_COUNT; i++) {
//		ret = bsp_battery_mvolts_get(&batt_mvolts);
		ret = app_analog_vbat_mv_get(&batt_mvolts);
		if (ret != 0) {
			LOG_ERR("bsp_battery_mvolts_get failed");
			return ret;
		}
		batt_mvolts_sum += batt_mvolts;
	}
	batt_mvolts = (batt_mvolts_sum / READING_NORMALIZE_COUNT);

	level = asigmoidal(batt_mvolts, APP_BATTERY_MIN_VALUE_MVOLTS,
	APP_BATTERY_MAX_VALUE_MVOLTS);
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
//	float batt_capacity=0;
//	ret = bsp_battery_available_capacity_get(&batt_capacity);
//	level = (uint8_t)((batt_capacity/CONFIG_APP_BATTERY_MAX_CAPACITY_MAH) * 100);
//	LOG_INF("Battery Capacity = %.2f mAh", (double)batt_capacity);
//#endif
	//int32_t batt_mvolts = 0;

/*This condition runs when the battery has been removed
* If the battery has been removed then it checks the battery voltage and calculates the battery capacity
* Else if battery is connected it skips the battery voltage checking*/
	if (battery_removed == 1) {
		int32_t batt_mvolts_sum = 0;
		app_analog_measure_en(APP_ANALOG_VBAT);

		/* read the battery mili volts and average it */
		for (uint32_t i = 0; i < 100; i++) {
			ret = app_analog_vbat_mv_get(&batt_mvolts);
			if (ret != 0) {
				LOG_ERR("bsp_battery_mvolts_get failed");
				return ret;
			}
			batt_mvolts_sum += batt_mvolts;
		}
		batt_mvolts = (batt_mvolts_sum / 100);
		level = asigmoidal(batt_mvolts, CONFIG_APP_BATTERY_MIN_VALUE_MVOLTS, CONFIG_APP_BATTERY_MAX_VALUE_MVOLTS);
//#endif
		if (batt_mvolts < 12800) {
			LOG_INF("Last charge state fetching ....!!");
			return ret;
		}
		float battery_capacity_cal = 0;

		/*TODO The number 1500mAH battery capacity should be fetched dynamically from the BAT_SPEC adc pin*/
		battery_capacity_cal = ((float)level / 100) * 1500;
		LOG_INF("Battery voltage = %d mV", batt_mvolts);
		LOG_INF("Last state of charge: %0.2f mAH", (double)battery_capacity_cal);
		bsp_battery_capacity_value_set(battery_capacity_cal);

		/*TODO add a logic to check this condition for C208 board*/
		battery_removed = 0;
	}
#endif
//	if (level > 100)	level = 100;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	ret = bsp_battery_available_capacity_get(&batt_capacity);
	level = (uint8_t)((batt_capacity/CONFIG_APP_BATTERY_MAX_CAPACITY_MAH) * 100);
	LOG_INF("Battery Capacity = %.2f mAh", (double)batt_capacity);
#endif
	if (level > 100)	level = 100;
	LOG_INF("Battery Level = %d", level);
	*batt_level = level;
	return ret;
}

int app_battery_runtime_get(struct tm *time)
{
	/* get available battery capacity */
	float batt_avail_capacity=0;	// maH
	int ret = bsp_battery_available_capacity_get(&batt_avail_capacity);
	if (ret < 0) {
		LOG_DBG("could not get battery capacity");
		return ret;
	}

	/* TODO: get current consumption from charger */
	int32_t ibat_ma = 0;
	ret = bsp_battery_ibat_get(&ibat_ma);
	LOG_DBG("ibat_ma = %d", ibat_ma);
	float curr_consum = 200.0f;	// ma

	/* calculate remaining hours*/
	float rem_hrs = batt_avail_capacity / curr_consum;

	/* convert hours to time struct */
	time->tm_hour = (int)rem_hrs;

	int min = (int)(rem_hrs*100);
	min = min % 100;
	time->tm_min = (min*60)/100;

	return 0;
}

int app_battery_charging_check_and_act()
{
	int ret = 0;
	uint8_t charging_status=0;
	uint32_t ichg=0;
	ret = bsp_battery_charging_status_get(&charging_status, &ichg);
	LOG_INF("charging status = %d", charging_status);
	if ((!ret) && (charging_status == BSP_BATT_CHARGING)) {
		lib_events_report_event(LIB_EVENT_CHARGER_ATTACHED);
		m_charge_stat = true;	// charger attached (charging)
#if (CONFIG_APP_DISPLAY)
		c20x_screen_statusbar_reload();		// update display
#endif
	}
	return ret;
}

bool app_battery_usb_attached_check()
{
	int32_t vbus_mv = 0;
	int ret = bsp_battery_vbus_get(&vbus_mv);
	if (ret) {
		LOG_ERR("could not read vbus voltage, %d", ret);
		return false;
	}
	if ((vbus_mv * 1000) >= BSP_BATTERY_VBUS_MIN_UV) {
		return true;
	} else {
		return false;
	}
}

bool app_battery_chargestat_get()
{
	return m_charge_stat;	// true charging, false not charging
}

int app_battery_init()
{
	int ret = 0;

	/* initialize the bsp battery */
	ret = bsp_battery_init();
	if (ret != 0) {
		LOG_ERR("bsp_battery_init failed");
		return ret;
	}
#if (CONFIG_BSP_MEMBRANE_SWITCH)
	/* register callback for power off and system reset event */
	ret = bsp_membrane_callback_add(&power_cb_data, power_switch_cb_handler, BSP_MEMBR_SWITCH_LABEL_POWER, BSP_MEMBR_SWITCH_PIN_POWER);
	if (ret != 0) {
		LOG_ERR("bsp_membrane_callback_add failed");
		return ret;
	}

	/* Prepare worker thread */
	k_work_init(&m_long_press_worker, long_press_detection_worker);
#endif /*#if (CONFIG_BSP_MEMBRANE_SWITCH)*/

	/* get the status of battery charger and show notification
	 * we need to do this here because we might miss an interrupt
	 * from the charger during system on, e.g. if the system is
	 * powered on after attaching the charger, we will not get the
	 * charger attached interrupt, so we need to check it here
	 *  */
	ret = app_battery_charging_check_and_act();

	/* register callback for battery charging events from bsp battery layer */
	ret = bsp_battery_register_cb(charge_status_cb_handler);

	/* initialize device power state */
	m_pwr_state = DEVICE_PWR_STATE_RUNNING;

	/*maps the ambient pressure sensor id on location*/
	struct setting_value battery_attach;
	app_settings_load_single(SETTINGS_KEY_FULL_AS_BS, &battery_attach, sizeof(struct setting_value));
	if (battery_attach.val1 == 0)
		LOG_INF("BATTERY FOUND");
	else
		LOG_INF("BATTERY NOT-FOUND !!!");
	battery_removed = battery_attach.val1;

	/* register application event callbacks */
	ret = lib_events_callback_add(&m_cb_suspend, app_event_handler, LIB_EVENT_SUSPEND);
	ret = lib_events_callback_add(&m_cb_resume, app_event_handler, LIB_EVENT_RESUME);
	ret = lib_events_callback_add(&m_cb_poweroff, app_event_handler, LIB_EVENT_POWER_OFF);
	ret = lib_events_callback_add(&m_cb_ucpd_nego_done, app_event_handler, LIB_EVENT_UCPD_SNK_NEGO_DONE);

#if (CONFIG_APP_USBC_PD)
	/* Initialize the UCPD BSP */
	ret = bsp_ucpd_init();
#endif

	return ret;
}
