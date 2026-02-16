/*
 * Copyright (c) 2021 Acme CPU
 *
 * Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#if CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif

#include "bsp_init.h"
#include "acpu_c201_application.h"
#include "lib_events/lib_events.h"

#include "bsp_buzzer/bsp_buzzer.h"
#if (CONFIG_BSP_MEMBRANE_SWITCH)
	#include "bsp_membrane_switch/bsp_membrane_switch.h"
#endif
#if (CONFIG_APP_PUSH_SWITCH)
	#include "app_push_switch/app_push_switch.h"
#endif
#include "app_blower/app_blower.h"

#if (CONFIG_APP_UCPD)
	#include "app_ucpd/app_ucpd.h"
#endif
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"

#include "app_utils/app_utils.h"
#include "app_battery/app_battery.h"
#include "app_dfu/app_dfu.h"

///* TODO remove this. It is temporarily used */
//#include "bsp_usb_mux/bsp_usb_mux.h"

#define APP_ENABLED	1

static void main_print_message() {
	printk("*** Starting C20x application *** \n\n");
	printk("*** VERSION: 0.0.0.3 *** \n\n");
}

static void keypad_poll() {
#if (CONFIG_BSP_MEMBRANE_SWITCH && CONFIG_BOARD_C204_CORE)
	const struct device *dev;
	if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_ENTER, BSP_MEMBR_SWITCH_PIN_ENTER) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_ENTER);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_ENTER, BSP_MEMBR_SWITCH_MASK_ENTER);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_BACK, BSP_MEMBR_SWITCH_PIN_BACK) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_BACK);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_BACK, BSP_MEMBR_SWITCH_MASK_BACK);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_LEFT, BSP_MEMBR_SWITCH_PIN_LEFT) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_LEFT);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_LEFT, BSP_MEMBR_SWITCH_MASK_LEFT);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_RIGHT, BSP_MEMBR_SWITCH_PIN_RIGHT) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_RIGHT);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_RIGHT, BSP_MEMBR_SWITCH_MASK_RIGHT);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_DOWN, BSP_MEMBR_SWITCH_PIN_DOWN) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_DOWN);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_DOWN, BSP_MEMBR_SWITCH_MASK_DOWN);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_UP, BSP_MEMBR_SWITCH_PIN_UP) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_UP);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_UP, BSP_MEMBR_SWITCH_MASK_UP);
	}
	else if (bsp_membrane_switch_state_get(BSP_MEMBR_SWITCH_LABEL_HOME, BSP_MEMBR_SWITCH_PIN_HOME) == SWITCH_ASSERTED)
	{
		dev = device_get_binding(BSP_MEMBR_SWITCH_LABEL_HOME);
		bsp_membrane_switch_poll_handler(dev, BSP_MEMBR_SWITCH_LABEL_HOME, BSP_MEMBR_SWITCH_MASK_HOME);
	}
#elif (CONFIG_APP_PUSH_SWITCH && CONFIG_BOARD_C204_CORE)

	if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_MASK_ENTER, APP_PUSH_SWITCH_PIN_ENTER);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_PIN_BACK) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_MASK_BACK, APP_PUSH_SWITCH_PIN_BACK);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_PIN_DOWN) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_MASK_DOWN, APP_PUSH_SWITCH_PIN_DOWN);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_MASK_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_PIN_LEFT) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_MASK_LEFT, APP_PUSH_SWITCH_PIN_LEFT);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_PIN_UP) == SWITCH_ASSERTED)
	{
		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_MASK_UP, APP_PUSH_SWITCH_PIN_UP);
	}
	else if (lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_HOME, APP_PUSH_SWITCH_PIN_HOME) == SWITCH_ASSERTED)
	{
//		lib_push_switch_poll_handler(APP_PUSH_SWITCH_DEVICE_HOME, APP_PUSH_SWITCH_MASK_HOME, APP_PUSH_SWITCH_PIN_HOME);
	}
#elif (CONFIG_APP_PUSH_SWITCH && (CONFIG_BOARD_STM32G473_ACME_CPU_C201 || CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED))
#endif	/*#if (CONFIG_BSP_MEMBRANE_SWITCH)*/

}

static void reed_poll() {

}

int main(void) {
	main_print_message();

#if APP_ENABLED
	int ret = 0;

	/* Initialize the board support packages */
	ret = bsp_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize the bsp!");
		/* If the bsp init fails, we have nothing to do! */
		return -1;
	}

	/* Initialize the application modules */
	ret = acpu_c201_app_init();
	ret = 0;	// todo must remove
	if (ret != 0) {
		LOG_ERR("Failed to initialize the acpu_c20x_app!");
		/* TODO handle error */
		for(int i=0; i<3; i++) {
			bsp_buzzer_on();
			k_sleep(K_MSEC(50));
			bsp_buzzer_off();
			k_sleep(K_MSEC(50));
		}
		return -1;
	}

//	/* TODO remove this. It is temporarily used */
//	bsp_usb_mux_select_usb1();
////	bsp_usb_mux_select_mhl();

	struct setting_value val;
#if (CONFIG_APP_SETTINGS_DEVELOPER_MODE)
	ret = app_settings_load_single(SETTINGS_KEY_FULL_DEV_USB, &val, sizeof(struct setting_value));
	if (ret == 0) {
//		if (val.val1 == 0)	app_utils_usb_channel_select(USB_DATA_CHANNEL_ESP32);
//		if (val.val1 == 1)	app_utils_usb_channel_select(USB_DATA_CHANNEL_STM32);
		if (val.val1 == 0)	app_utils_usb_channel_select(USB_DATA_CHANNEL_STM32);
		if (val.val1 == 1)	app_utils_usb_channel_select(USB_DATA_CHANNEL_ESP32);
		if (val.val1 == 2)	app_utils_usb_channel_select(USB_DATA_CHANNEL_OTHER);
	}
#else
	/* select STM32 USB as default USB data port */
	val.val1=1; val.val2=0;
	ret = app_utils_usb_channel_select(USB_DATA_CHANNEL_ESP32);
	while (ret != 0) {
		ret = app_settings_save_single(SETTINGS_KEY_FULL_DEV_USB, &val, sizeof(struct setting_value), true);
		k_sleep(K_MSEC(100));
	}
#endif

#if CONFIG_USB_DEVICE_STACK
	if (usb_enable(NULL)) {
		return -1;
	}
#endif

    app_common_events_register();

	/* Check and enable battery charging */
	ret = app_battery_check_enable_charging();

	/* Check if USB is attached and report attached event so UCPD is negotiated */
	if (app_battery_usb_attached_check()) {
		lib_events_report_event(LIB_EVENT_CHARGER_ATTACHED);
	}

	/* notify booting completed */
	bsp_buzzer_on();
	k_sleep(K_MSEC(50));
	bsp_buzzer_off();
	lib_events_report_event(LIB_EVENT_SYSTEM_BOOTING_COMPLETE);

	/* Connect to network */
	ret = app_check_and_connect_to_network();

#if (CONFIG_APP_DFU)
	/* Validate image */
	if (ret == 0) {
		/* todo: do more tests */

		/* mark this image as permanent */
		app_dfu_upgrade_permanent();
	}
#endif

	while (1) {
#if (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH)
		/* poll for keypad switch presses */
		keypad_poll();
#endif
		/* poll for reed switch changes */
		reed_poll();

#if (CONFIG_APP_UCPD)
		/* check for ucpd source changes */
		if (app_ucpd_check_ps()) {
			// TODO get new ucpd source caps
		}
#endif

		/* poll for various application events */
		app_events_poll();

		k_sleep(K_MSEC(10));
	}
#endif
	return 0;
}
