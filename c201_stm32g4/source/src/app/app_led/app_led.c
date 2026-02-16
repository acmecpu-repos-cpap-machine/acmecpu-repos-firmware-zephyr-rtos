/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_led);

#include "acpu_c201_modules.h"
#include "app_thread_configs.h"
#include "app_led_notification.h"
#include "bsp_led/bsp_led_ntf.h"
#include "lib_events/lib_events.h"

#define APP_LED_THREAD	0

/* static variables */
#if (APP_LED_THREAD)
K_THREAD_STACK_DEFINE(m_led_thread_stack, APP_THREAD_STACK_SIZE_LED_NTF);
static struct k_thread m_led_thread_data;
static k_tid_t m_led_tid;
#endif	/* #if (APP_LED_THREAD) */

static struct k_mutex m_lock;

static struct lib_events_callback m_cb_suspend;
static struct lib_events_callback m_cb_resume;
static struct lib_events_callback m_evnt_cb_wifi_connected;
static struct lib_events_callback m_evnt_cb_wifi_disconnected;
static struct lib_events_callback m_evnt_cb_hotspot_started;
static struct lib_events_callback m_evnt_cb_hotspot_stopped;
static struct lib_events_callback m_evnt_cb_charger_attached;
static struct lib_events_callback m_evnt_cb_charger_removed;
static struct lib_events_callback m_evnt_cb_charge_complete;

//LIB_EVENT_TYPE m_event;

/* static functions */
#if (APP_LED_THREAD)
static void led_thread(void *p1, void *p2, void *p3) {
//static void led_event_worker(struct k_work *work) {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);
	led_config.led_state = 1;
	bsp_led_show_notification(APP_LED_SYS_ON, &led_config);

	led_config.ntf_type = (BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = 0;
	int toggle = 1;

	while (1) {
		if (m_event != LIB_EVENT_SUSPEND) {
			led_config.ntf_type = (BSP_LED_NTF_NORMAL | BSP_LED_NTF_BRIGHTNESS);
			led_config.led_state = 1;
			led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
			bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
			break;
		}

		bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
		if (toggle) {
			if (++led_config.brightness >= APP_LED_NTF_DEFAULT_BRIGHTNESS) {
				toggle = 0;
			}
		} else {
			if (--led_config.brightness <= 0) {
				toggle = 1;

//				led_config.ntf_type = (BSP_LED_NTF_NORMAL);
//				led_config.led_state = 0;
//				bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
				k_sleep(K_MSEC(400));
//				led_config.led_state = 1;
//				bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
//
//				led_config.ntf_type = (BSP_LED_NTF_BRIGHTNESS);
			}
		}
		k_sleep(K_MSEC(25));
	}
}
#endif	/* #if (APP_LED_THREAD) */

/* global functions */
static int app_led_show_charging() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_BLINK | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.blink_delay_on_ms = APP_LED_NTF_BLINK_DELAY_ON;
	led_config.blink_delay_off_ms = APP_LED_NTF_BLINK_DELAY_OFF;

	int ret = bsp_led_show_notification(APP_LED_POWER, &led_config);
	return ret;
}

static int app_led_show_not_charging() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.led_state = 0;

	int ret = bsp_led_show_notification(APP_LED_POWER, &led_config);
	return ret;
}

static int app_led_show_charging_done() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.led_state = 1;

	int ret = bsp_led_show_notification(APP_LED_POWER, &led_config);
	return ret;
}

int app_led_show_fault() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_BLINK | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.blink_delay_on_ms = APP_LED_NTF_BLINK_DELAY_ON;
	led_config.blink_delay_off_ms = APP_LED_NTF_BLINK_DELAY_OFF;

	int ret = bsp_led_show_notification(APP_LED_SYS_FAULT, &led_config);
	return ret;
}

static int app_led_show_ble_advertising() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_BLINK | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.blink_delay_on_ms = APP_LED_NTF_BLINK_DELAY_ON;
	led_config.blink_delay_off_ms = APP_LED_NTF_BLINK_DELAY_OFF;

	int ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
	return ret;
}

static int app_led_show_ble_connected() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL | BSP_LED_NTF_BRIGHTNESS);
	led_config.brightness = APP_LED_NTF_DEFAULT_BRIGHTNESS;
	led_config.led_state = 1;

	int ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);

	k_sleep(K_MSEC(1000));

	led_config.led_state = 1;
	ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);

	return ret;
}

static int app_led_show_wifi_connected() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);

	led_config.led_state = 0;
	int ret = bsp_led_show_notification(APP_LED_SYS_FAULT, &led_config);

	led_config.led_state = 1;
	ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);
	return ret;
}

static int app_led_show_wifi_disconnected() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);
	led_config.led_state = 0;

	int ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);

	return ret;
}

static int app_led_show_hotspot_started() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);
	led_config.led_state = 1;
	int ret = bsp_led_show_notification(APP_LED_SYS_FAULT, &led_config);
	return ret;
}

static int app_led_show_hotspot_stopped() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);
	led_config.led_state = 0;
	int ret = bsp_led_show_notification(APP_LED_SYS_FAULT, &led_config);
	return ret;
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_SUSPEND:
		LOG_INF("LIB_EVENT_SUSPEND");
//		k_mutex_lock(&m_lock, K_FOREVER);
//		m_event = LIB_EVENT_SUSPEND;
//		k_mutex_unlock(&m_lock);
#if (APP_LED_THREAD)
		m_led_tid = k_thread_create(&m_led_thread_data, m_led_thread_stack,
						K_THREAD_STACK_SIZEOF(m_led_thread_stack), led_thread,
						NULL, NULL, NULL, APP_THREAD_PRIO_LED_NTF, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
		k_thread_name_set(m_led_tid, APP_THREAD_NAME_LED_NTF);
#endif
#endif
		break;
	case LIB_EVENT_RESUME:
		LOG_INF("LIB_EVENT_RESUME");
//		k_mutex_lock(&m_lock, K_FOREVER);
//		m_event = LIB_EVENT_RESUME;
//		k_mutex_unlock(&m_lock);
//		k_thread_join(&m_led_thread_data, K_MSEC(50));
		break;
	case LIB_EVENT_CHARGER_ATTACHED:
		app_led_show_charging();
		break;
	case LIB_EVENT_CHARGER_REMOVED:
		app_led_show_not_charging();
		break;
	case LIB_EVENT_CHARGE_COMPLETE:
		app_led_show_charging_done();
		break;
	case LIB_EVENT_NET_WIFI_CONNECTED:
		app_led_show_wifi_connected();
		break;
	case LIB_EVENT_NET_WIFI_DISCONNECTED:
		app_led_show_wifi_disconnected();
		break;
	case LIB_EVENT_NET_HOTSPOT_STARTED:
		app_led_show_hotspot_started();
		break;
	case LIB_EVENT_NET_HOTSPOT_STOPPED:
		app_led_show_hotspot_stopped();
		break;
	default:
		LOG_INF("%d", event);
		break;
	}
}

int app_led_init() {
	struct bsp_led_config led_config;
	led_config.ntf_type = (BSP_LED_NTF_NORMAL);
	led_config.led_state = 0;

	int ret = bsp_led_show_notification(APP_LED_POWER, &led_config);
	ret = bsp_led_show_notification(APP_LED_SYS_FAULT, &led_config);
	ret = bsp_led_show_notification(APP_LED_RESERVED, &led_config);
	ret = bsp_led_show_notification(APP_LED_SYS_ON, &led_config);

	k_mutex_init(&m_lock);

	/* register application event callbacks */
	ret = lib_events_callback_add(&m_cb_suspend, app_event_handler, LIB_EVENT_SUSPEND);
	ret = lib_events_callback_add(&m_cb_resume, app_event_handler, LIB_EVENT_RESUME);
	ret = lib_events_callback_add(&m_evnt_cb_wifi_connected, app_event_handler, LIB_EVENT_NET_WIFI_CONNECTED);
	ret = lib_events_callback_add(&m_evnt_cb_wifi_disconnected, app_event_handler, LIB_EVENT_NET_WIFI_DISCONNECTED);
	ret = lib_events_callback_add(&m_evnt_cb_hotspot_started, app_event_handler, LIB_EVENT_NET_HOTSPOT_STARTED);
	ret = lib_events_callback_add(&m_evnt_cb_hotspot_stopped, app_event_handler, LIB_EVENT_NET_HOTSPOT_STOPPED);
	ret = lib_events_callback_add(&m_evnt_cb_charger_attached, app_event_handler, LIB_EVENT_CHARGER_ATTACHED);
	ret = lib_events_callback_add(&m_evnt_cb_charger_removed, app_event_handler, LIB_EVENT_CHARGER_REMOVED);
	ret = lib_events_callback_add(&m_evnt_cb_charge_complete, app_event_handler, LIB_EVENT_CHARGE_COMPLETE);

	return ret;
}
