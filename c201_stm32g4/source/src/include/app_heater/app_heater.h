/*
 * app_heater.h
 *
 *  Created on: 01-Mar-2024
 *      Author: Shubham Keshari
 */

#ifndef SRC_INCLUDE_APP_HEATER_APP_HEATER_H_
#define SRC_INCLUDE_APP_HEATER_APP_HEATER_H_

#include <stdint.h>

typedef enum {
	APP_HEATER_MODULE_24,
	APP_HEATER_MODULE_12,
} APP_HEATER_MODULE;

typedef enum {
	HEATER_CONFIG_DEFAULT,
	HEATER_CONFIG_UPDATE,
} APP_HEATER_CONFIG;

struct heater_24w_var {
	float ntc_thermistor_c;
	float humidity_per;
	int check_interval;
};

struct heater_12w_var {
	float ntc_thermistor_temp_c;
	int chk_interval;
};

/**
 * @brief
 *      enables heater thread
 *
 * @param heater_wat[in]	heater module wattage selection
 * @param heater_set[in]	heater module set the default/update the variables
 * @param heater_24w_temp_c[in]	temperature at which the 24w heater needs to be modulated
 * @param heater_12w_temp_c[in]	temperature at which the 12w heater needs to be modulated
 * @param humid_24w_per	humidity[in] percentage at which the 24w heater needs to be modulated
 * @param interval_24w_ms[in]	checking interval for 24w heater
 * @param interval_12w_ms[in]	checking interval for 12w heater
 *
 * @brief If the range is not matched with the desired range of variables which comes
 * 		  from Kconfig file then those variables are set to there
 * 		  respective max or min values depending upon the conditions with there respective max or min range values
 *
 * 		  If the value 0 is passed as a parameter for heater temp and humidity then nothing happens
 * 		  error is thrown as humidity and temperature can not be zero
 *
 * @return 0 success
 * 			-ve for failure
 */
int app_heater_control_thread_en(APP_HEATER_MODULE heater_wat,
		APP_HEATER_CONFIG heater_set, float heater_24w_temp_c,
		float heater_12w_temp_c, float humid_24w_per, int interval_24w_ms,
		int interval_12w_ms);

/**
 * @param heater_wat[in]	heater module wattage selection
 * @brief
 *      disables heater thread
 *
 * @return
 * 		0 success
 * 		-ve for failure
 */
int app_heater_control_thread_dis(APP_HEATER_MODULE heater_wat);

/**
 * @brief
 *      event handler function is called from here keeps checking if the settings is changed or not
 *      event handler function is called from here keeps checking if the events like poweroff, reboot and suspend is called or not
 *
 * @return
 * 		0 success
 * 		-ve for failure
 */
int app_heater_init();

#endif /* SRC_INCLUDE_APP_HEATER_APP_HEATER_H_ */
