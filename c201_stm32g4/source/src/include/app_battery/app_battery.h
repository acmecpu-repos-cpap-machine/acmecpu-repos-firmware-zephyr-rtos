/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_BATTERY_APP_BATTERY_H_
#define SRC_INCLUDE_APP_BATTERY_APP_BATTERY_H_

#include <stdint.h>
#include <time.h>

/**
 * Get the battery level in percentage
 * If the system has no battery, then battery level will be 0
 *
 * @param batt_level[out]	battery level in percentage
 * @return
 * 	0		battery is present and successfully read the battery level
 * 	-ENXIO	battery not present
 * 	other 	battery is present and could not read the battery level
 */
int app_battery_level_get(uint8_t *batt_level);

/**
 * This function checks whether a battery is connected by measuring the voltage
 * If a battery is connected, then it enables charging else disables charging
 * @return
 * 0 SUCCESS
 * -ve failure
 */
int app_battery_check_enable_charging();

/**
 * This function checks if VBUS is available and tries to initialize
 * an external UCPD controller like stusb4500 if available
 * @return
 * 0 SUCCESS
 * -ve failure
 */
int app_battery_check_enable_ucpd();

int app_battery_runtime_get(struct tm *time);

/**
 * This function checks if battery is being charged or not
 *
 * @return 	0 SUCCESS
 * 			-ve Fail
 */
int app_battery_charging_check_and_act();

/**
 * This function checks if the USB is attached or not. It does by checking the
 * VBUS voltage.
 * @return	true 	USB is attached
 * 			false	USB is not attached
 */
bool app_battery_usb_attached_check();

bool app_battery_chargestat_get();	// returns true if charging, false not charging

int app_battery_init();

#endif /* SRC_INCLUDE_APP_BATTERY_APP_BATTERY_H_ */
