/*
 * Copyright (c) 2021 Acme CPU
 *
 * Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/reboot.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(c201_app);

#include "acpu_c201_application.h"
#if (CONFIG_APP_LOGGER)
	#include "app_logger.h"
#endif
#include "app_errors.h"
#if (CONFIG_APP_DATA_RECORDER)
	#include "app_data_recorder.h"
#endif
#if (CONFIG_BSP_MEMBRANE_SWITCH)
#include "bsp_membrane_switch/bsp_membrane_switch.h"
#endif
#if (CONFIG_APP_PUSH_SWITCH)
	#include "app_push_switch/app_push_switch.h"
#endif
#if (CONFIG_APP_MATRIX_KEYPAD)
	#include "app_matrix_keypad/app_matrix_keypad.h"
#endif
#include "app_blower/app_blower.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"
#if CONFIG_APP_DEVCMD
#include "app_devcmd/app_devcmd.h"
#endif
#if CONFIG_APP_UART_M2M
#include "app_uart_m2m_com/app_uart_m2m_com.h"
#endif
#if CONFIG_APP_BATTERY
#include "app_battery/app_battery.h"
#endif
#if CONFIG_APP_LED_NOTIFICATION
#include "app_led_notification.h"
#endif
#include "lib_events/lib_events.h"
#include "app_time/app_time.h"
#include "app_stepper/app_stepper.h"
#if (CONFIG_APP_DISPLAY)
#include "app_display/app_display.h"
#endif
#include "app_sensor/app_sensor.h"
#if CONFIG_APP_SHELLCMD
#include "app_shellcmd/app_shellcmd.h"
#endif
#include "app_storage/app_storage.h"
#if (CONFIG_APP_UCPD)
	#include "app_ucpd/app_ucpd.h"
#endif
#if (CONFIG_APP_UCPD_EXT_CONTR)
	#include "app_ucpd/app_ucpd_ext_contr.h"
#endif
#include "app_utils/app_utils.h"
#include "app_analog/app_analog.h"
#if (CONFIG_APP_NET)
	#include "app_net/app_net.h"
#endif

#if CONFIG_APP_HAS_HEATER
#include "app_heater/app_heater.h"
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
#include "app_fluid_level/app_fluid_level.h"
#endif

/* Declarations */
//typedef struct {
//	struct k_sem lock;
//} c201_app_data;
//c201_app_data m_c201_app_data;

/* Static variables */
//static ACPU_C201_APP_STATES m_c201_app_state = C201_APP_STATE_BOOTING;

/* Static functions */

/* Global functions */
//uint32_t acpu_c201_app_state_get() {
//	return m_c201_app_state;
//}
//
//void acpu_c201_app_state_set(uint32_t state) {
//	k_sem_take(&m_c201_app_data.lock, K_FOREVER);
//	m_c201_app_state = state;
//	k_sem_give(&m_c201_app_data.lock);
//}

static struct k_work app_worker;
static struct lib_events_callback m_evnt_cb_settings_changed;
static struct lib_events_callback m_evnt_cb_wifi_started;
static struct lib_events_callback m_evnt_cb_wifi_stopped;
static struct lib_events_callback m_evnt_cb_reboot;
static struct lib_events_callback m_evnt_cb_poweroff;
static int m_wifi_state = LIB_EVENT_NET_WIFI_STOPPED;
static int m_power_state = LIB_EVENT_POWER_ON;

static void app_workq_handler(struct k_work *work)
{
	int ret=0;
#if (CONFIG_APP_SETTINGS_DEVELOPER_MODE)
		/* */
		struct setting_value val;
		ret = app_settings_load_single(SETTINGS_KEY_FULL_DEV_USB, &val, sizeof(struct setting_value));
		if (ret == 0) {
			if (val.val1 == 0)	app_utils_usb_channel_select(USB_DATA_CHANNEL_STM32);
			if (val.val1 == 1)	app_utils_usb_channel_select(USB_DATA_CHANNEL_ESP32);
			if (val.val1 == 2)	app_utils_usb_channel_select(USB_DATA_CHANNEL_OTHER);
		}
#endif
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch (event) {
	case LIB_EVENT_SETTINGS_CHANGED:
	{
		k_work_submit(&app_worker);
		break;
	}
	case LIB_EVENT_NET_WIFI_STARTED:
	{
		m_wifi_state = LIB_EVENT_NET_WIFI_STARTED;
		break;
	}
	case LIB_EVENT_NET_WIFI_STOPPED:
	{
		m_wifi_state = LIB_EVENT_NET_WIFI_STOPPED;
		break;
	}
	case LIB_EVENT_REBOOT:
	{
		m_power_state = LIB_EVENT_REBOOT;
		break;
	}
	case LIB_EVENT_POWER_OFF:
	{
		m_power_state = LIB_EVENT_POWER_OFF;
		break;
	}
	default:
		break;
	}
}

void app_events_poll()
{
	/** this function is called periodically from the main thread */
	int ret=0;
	if (m_power_state == LIB_EVENT_REBOOT) {
		LOG_INF("******************** rebooting ...");
		/**
		 * things to do before reboot
		 * - blower off
		 * - wifi and softap off
		 * - ???
		 * */
#if CONFIG_APP_BLOWER
		ret = app_blower_settings_change_state(APP_BLOWER_STOP);
#endif
		ret = app_net_wifi_hotspot_start_stop(APP_NET_WIFI, 0, NULL, NULL);
		ret = app_net_wifi_hotspot_start_stop(APP_NET_HOTSPOT, 0, NULL, NULL);
		sys_reboot(SYS_REBOOT_COLD);
	} else if (m_power_state == LIB_EVENT_POWER_OFF) {
		LOG_INF("******************** shutting down ...");
	}
}

void app_common_events_register()
{
	lib_events_callback_add(&m_evnt_cb_settings_changed, app_event_handler, LIB_EVENT_SETTINGS_CHANGED);
	lib_events_callback_add(&m_evnt_cb_wifi_started, app_event_handler, LIB_EVENT_NET_WIFI_STARTED);
	lib_events_callback_add(&m_evnt_cb_wifi_stopped, app_event_handler, LIB_EVENT_NET_WIFI_STOPPED);
	lib_events_callback_add(&m_evnt_cb_reboot, app_event_handler, LIB_EVENT_REBOOT);
	lib_events_callback_add(&m_evnt_cb_poweroff, app_event_handler, LIB_EVENT_POWER_OFF);
}

int app_check_and_connect_to_network()
{
	int ret = 0;
	int count=0;

	/* start wifi */
	count=0;
	struct setting_value wi_val; wi_val.val1 = 1; wi_val.val2 = 0;
//	do {
//		ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WI, &wi_val, sizeof(struct setting_value), true);
//		k_sleep(K_MSEC(10));
//	} while ((ret != 0) && (++count < 10));
	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_DS_NET_WI, &wi_val, sizeof(struct setting_value), 10, true);

	/* wait until wifi has started */
	int max_wait = (40*1000); //(5*60*1000);	// 5 mins
	int ms_dly = 10;
	while (m_wifi_state != LIB_EVENT_NET_WIFI_STARTED) {
		k_sleep(K_MSEC(ms_dly));
		max_wait = max_wait - ms_dly;
		if (max_wait <= 0) {
			ret = -1;
			LOG_ERR("Wi-Fi start failed");
			goto hotspot;
		}
	}

	/* try connecting to a network */
	ret = app_net_connect();

hotspot:
	/* if not connected, start hotspot mode */
	if (ret != 0) {
		count=0;
		struct setting_value wiap_val; wiap_val.val1 = 1; wiap_val.val2 = 0;
		do {
			ret = app_settings_save_single(SETTINGS_KEY_FULL_DS_NET_WIAP, &wiap_val, sizeof(struct setting_value), true);
			k_sleep(K_MSEC(10));
		} while ((ret != 0) && (++count < 10));
		char ap_ip[20];
	//	ret = app_net_hotspot_start_stop(1, ap_ip);	// send start hotspot cmd
		ret = app_net_wifi_hotspot_start_stop(APP_NET_HOTSPOT, 1, ap_ip, NULL);
		if (ret == 0) {
			lib_events_report_event(LIB_EVENT_NET_HOTSPOT_STARTED);
			LOG_INF("hotspot start successful, IP = %s", ap_ip);
		} else {
			lib_events_report_event(LIB_EVENT_NET_HOTSPOT_STOPPED);
			LOG_ERR("hotspot start failed");
		}
	}

	return ret;
}

int acpu_c201_app_init() {
	int ret = 0;

	/* Prepare interrupt worker */
	k_work_init(&app_worker, app_workq_handler);

	/* Initialize the application data */
	m_power_state = LIB_EVENT_POWER_ON;
//	m_c201_app_state = C201_APP_STATE_INITIALIZING;
//	memset(&m_c201_app_data, 0x00, sizeof(c201_app_data));
//	k_sem_init(&m_c201_app_data.lock, 1, 1);

	/*
	 * Initialize the App Events module
	 * */
	ret |= lib_events_init();

	/*
	 * Initialize the USB C Power Delivery app
	 * */
#if (CONFIG_APP_UCPD)
	ret |= app_ucpd_init();
#endif
#if (CONFIG_APP_UCPD_EXT_CONTR)
	ret |= app_ucpd_ext_contr_init();
#endif

#if (CONFIG_APP_STORAGE)
	/*
	 * Mount the disk drives
	 * */
	ret |= app_storage_mount();
#endif

#if (CONFIG_APP_DISPLAY)
	/* Initialize the display application
	 *		- initialize pwm for display brightness
	 *		- start display data thread
	 *		- start display refresh thread
	 * */
	ret |= app_display_init();
#endif

	/* Initialize the system time application
	 * */
	ret |= app_time_init();

	/*
	 * Initialize the settings application
	 * 		- load settings from persistent memory
	 * 		- save default settings if settings are not available
	 * */
	ret |= app_settings_init();

	/* Initialize the error handler application
	 *		- start thread handling and logging errors
	 * */
	ret |= app_error_handler_init();

#if (CONFIG_APP_LOGGER)
	/* Initialize the application logger
	 *		- mount sd card, initialize the logger backend
	 *		- TODO: let the init happen, save the data depending on the settings
	 * */
	uint8_t start_logger = 0;
	if (app_settings_load_single(SETTINGS_KEY_FULL_LOG_STORE, &start_logger,
			sizeof(start_logger)) == 0) {
//		if (start_logger == SETTINGS_LOG_STORE) {
			ret |= app_logger_init();
//		}
	}
#endif

#if (CONFIG_APP_LED_NOTIFICATION)
	/*
	 * Initialize App LEDs
	 * */
	ret |= app_led_init();
#endif

#if (CONFIG_BSP_MEMBRANE_SWITCH)
	/* Initialize the membrane switch application
	 * 		- configure the pins
	 * 		- register interrupt callback
	 * 		- start button event dispatcher thread
	 * */
//	ret |= app_membrane_switch_init();
	ret |= bsp_membrane_switch_init();
//#endif

#elif (CONFIG_APP_PUSH_SWITCH)
	/* initialize push switches */
	ret |= app_push_switch_init();
#endif

#if (CONFIG_APP_MATRIX_KEYPAD)
	ret |= app_matrix_keypad_init();
#endif

#if CONFIG_APP_DEVCMD
	/* Initialize the application command interface
	 * */
	ret |= app_devcmd_init();
#endif

#if CONFIG_APP_UART_M2M
	/* Initialize uart comm interface between all m2m devices */
	ret = app_uart_m2m_com_init();
#endif

#if (CONFIG_APP_DATA_RECORDER)
	/* Initialize the data recorder application
	 *		- start thread for recording application data (currently not using thread)
	 *		- TODO: let the init happen, save the data depending on the settings
	 * */
//	uint8_t start_recorder = 0;
//	if (app_settings_load_single(SETTINGS_KEY_FULL_DATA_STORE, &start_recorder,
//			sizeof(start_recorder)) == 0) {
//		if (start_recorder == SETTINGS_DATA_STORE) {
			ret |= app_data_recorder_init();
//		}
//	}
#endif

	/* TODO: remove from here */
//	ret |= app_display_init();

	ret |= app_analog_init();

#if CONFIG_APP_BATTERY
	/* Initialize the battery management application
	 *		- convert battery voltage into level
	 *		- manage system on/off/reset
	 *		- manage battery charging
	 * */
	ret |= app_battery_init();
#endif

#if (CONFIG_APP_NET)
	/* Initialize the network interfaces
	 *		- wifi
	 *		- bluetooth
	 *		- ethernet
	 *		- broadband
	 * */
	ret |= app_net_init();
#endif

	/* Initialize the sensor application
	 *		- start thread for pressure sensors
	 *		- start thread for IMU
	 * */
	ret |= app_sensor_init();

#if (!CONFIG_BOARD_C208T)
	/* Initialize the stepper motor application
	 *		- start thread for stepper motor control
	 * */
	ret |= app_stepper_init();
#endif

#if CONFIG_APP_BLOWER
	/* Initialize the blower application
	 *		- start thread for blower pid control
	 * */
	ret |= app_blower_init();
#endif

	/* Initialize the heater application
	 *		- start thread for heater p control
	 * */

#if CONFIG_APP_HAS_HEATER
	ret |= app_heater_init();
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
	ret |= app_fluid_level_init();
#endif

#if CONFIG_APP_SHELLCMD
	/* Initialize the application shell command interface
	 * */
	ret |= app_shellcmd_init();
#endif

//	if (ret == 0) {
//		/* Initialization successfully done */
//		m_c201_app_state = C201_APP_STATE_INITIALIZED;
//	} else {
//		/* Initialization failed */
//		m_c201_app_state = C201_APP_STATE_INIT_FAILED;
//	}

	return ret;
}
