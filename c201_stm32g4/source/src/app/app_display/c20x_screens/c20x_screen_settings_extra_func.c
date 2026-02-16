/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 28-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(c20x_screens);

#include "app_display/app_display.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"
//#include "app_settings/app_settings_display_data.h"

#include "c20x_screen_config.h"
#include "c20x_screen_settings.h"
#include "c20x_screen_alert_msg.h"

#include "app_net/app_net.h"

static const char * mbox_btn_pn[] = {"Prev", "Next", ""};
static const char * mbox_btn_next[] = {"Next", ""};
static const char * mbox_btn_ok[] = {"Ok", ""};

/******************************************************************************
 * WIFI HOTSPOT
 ******************************************************************************/
struct hotspot_screen {
	const char *display_name;
	app_display_key_cb prev_screen_cb;
	uint8_t screen_idx;
	struct k_work hotspot_work;
	char ip[20];
//	struct app_uart_m2m_callback m2m_cb_hp;
};

typedef enum {
	HOTSPOT_SCREEN_1 = 0,
	HOTSPOT_SCREEN_2 = 1,
	HOTSPOT_SCREEN_ERR = 2,
	HOTSPOT_SCREEN_FINISH = 3,
} HOTSPOT_SCREEN;

static void settings_hotspot_deinit(struct hotspot_screen *hps);

static void settings_hotspot_extra_cb(const char* active_btn_txt, void *data)
{
	struct hotspot_screen *hps = (struct hotspot_screen *)data;


	if (hps->screen_idx == HOTSPOT_SCREEN_ERR) {
		hps->screen_idx = HOTSPOT_SCREEN_FINISH;
	} else {
		if (active_btn_txt == NULL)	return;

		if (strcmp("Next", active_btn_txt) == 0) {
			hps->screen_idx++;
		} else if (strcmp("Prev", active_btn_txt) == 0) {
			hps->screen_idx--;
		} else if (strcmp("Ok", active_btn_txt) == 0) {
			hps->screen_idx = HOTSPOT_SCREEN_FINISH;
		}
	}

	k_work_submit(&hps->hotspot_work);
}

static void hotspot_work_handler(struct k_work *work)
{
	struct hotspot_screen *hps = CONTAINER_OF(work, struct hotspot_screen, hotspot_work);

	if (hps->screen_idx == HOTSPOT_SCREEN_1) {
		c20x_screen_alert_msg_show(NULL, 1, mbox_btn_next, hps->display_name,
				"Connect to the hotspot from your phone / computer",
				false, 0, C20X_SCREEN_SETTINGS, settings_hotspot_extra_cb, hps);
	} else if (hps->screen_idx == HOTSPOT_SCREEN_2) {
		char msg[100] = "Open a browser and enter ";
		strcat(msg, hps->ip);
		strcat(msg, "/home");
		c20x_screen_alert_msg_show(NULL, 1, mbox_btn_ok, hps->display_name, msg,
				false, 0, C20X_SCREEN_SETTINGS, settings_hotspot_extra_cb, hps);
	} else if (hps->screen_idx == HOTSPOT_SCREEN_ERR) {
		c20x_screen_alert_msg_show(NULL, 1, NULL, hps->display_name,
				"Cannot do hotspot operation",
				false, 3, C20X_SCREEN_SETTINGS, settings_hotspot_extra_cb, hps);
	} else if (hps->screen_idx == HOTSPOT_SCREEN_FINISH) {
		// done, let the called deallocate itself and the load the previous screen
		k_sleep(K_MSEC(1));

		app_display_key_cb prev_screen_cb = hps->prev_screen_cb;

		/* deinit and deallocate memory */
		settings_hotspot_deinit(hps);

		/* call the previous screen button handler to load it */
		if (prev_screen_cb != NULL)
			prev_screen_cb(LV_KEY_ESC);
	}
}

static void settings_hotspot_deinit(struct hotspot_screen *hps)
{
	free(hps);
}

int settings_hotspot_extra_func(
							const char *display_name,
							const char *setting_path,
							app_display_key_cb prev_screen_cb
							)
{
	int ret = 0;
	uint8_t start_stop=0;

	/* check if the hotspot setting is on or off */
	struct setting_value val;
	app_settings_load_single(setting_path, &val, sizeof(struct setting_value));
	if (val.val1 == 0)		start_stop = 0;
	else if (val.val1 == 1)	start_stop = 1;

	struct hotspot_screen *hps = (struct hotspot_screen*) calloc(1, sizeof(struct hotspot_screen));
	if(hps == NULL) {
		LOG_ERR("settings_hotspot_extra_func calloc failed");
		return -ENOMEM;
	}
	k_work_init(&hps->hotspot_work, hotspot_work_handler);
	hps->prev_screen_cb = prev_screen_cb;

	/* send command to network processor to start hotspot */
//	ret = app_net_hotspot_start_stop(start_stop, hps->ip);	// send start hotspot cmd
	ret = app_net_wifi_hotspot_start_stop(APP_NET_HOTSPOT, start_stop, hps->ip, NULL);
	if (ret == 0) {
		LOG_INF("hotspot start / stop successful");
		if (start_stop == 1) {	// successfully started the hotspot
			hps->display_name = display_name;
			hps->screen_idx = HOTSPOT_SCREEN_1;
			/* submit work item to display hotspot screens */
			ret = k_work_submit(&hps->hotspot_work);
			if (ret < 0)
				return -1;
		} else if (start_stop == 0) {	// successfully stopped the hotspot
			if (prev_screen_cb != NULL)
				prev_screen_cb(LV_KEY_ESC);
			return 0;
		}
	} else {	// could not start / stop the hotspot
		LOG_ERR("could not start / stop the hotspot");
		hps->display_name = "Error";
		hps->screen_idx = HOTSPOT_SCREEN_ERR;

		/* revert the setting because the operation failed */
		if (start_stop == 1) val.val1 = 0;
		else if (start_stop == 0) val.val1 = 1;
		val.val2 = 0;
		app_settings_save_single(setting_path, &val, sizeof (struct setting_value), true);

		/* submit work item to display hotspot screens */
		ret = k_work_submit(&hps->hotspot_work);
	}

	return ret;
}


/******************************************************************************
 * Extra functions common
 ******************************************************************************/
void c20x_screen_settings_extra_func_init()
{

}



