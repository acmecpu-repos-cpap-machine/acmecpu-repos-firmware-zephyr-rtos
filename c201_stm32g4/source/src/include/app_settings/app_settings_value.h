/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 10-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_VALUE_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_VALUE_H_

#include <zephyr/types.h>
#include <stdint.h>
#include <errno.h>

#include "app_settings_data.h"
#include "app_settings_def.h"

/**
 * @brief Representation of a setting value.
 *
 * The value is represented as having an integer and a fractional part,
 * and can be obtained using the formula val1 + val2 * 10^(-6). Negative
 * values also adhere to the above formula, but may need special attention.
 * Here are some examples of the value representation:
 *
 *      0.5: val1 =  0, val2 =  500000
 *     -0.5: val1 =  0, val2 = -500000
 *     -1.0: val1 = -1, val2 =  0
 *     -1.5: val1 = -1, val2 = -500000
 */
struct setting_value {
	/** Integer part of the value. */
	int32_t val1;
	/** Fractional part of the value (in one-millionth parts). */
	int32_t val2;
};

struct app_settings_value {
	char key[SETTING_VAL_KEY_LEN_MAX];
	struct setting_value val;
};

struct app_settings_param_value {
	uint16_t mode_bitmask;
	char override_name[SETTINGS_NAME_LEN_MAX];
	int16_t range_min_idx;
	int16_t range_max_idx;
};

/********************************************************************
 * CLINICAL / TECHNICAL SETTINGS
 *********************************************************************/
/****** MODES ******/
#define CLINICAL_MODE_MAX	9
typedef enum {
	MODE_SNORESTOP					= 0x0100,
	MDOE_PAPR						= 0x0080,
	MODE_CPAP						= 0x0040,
	MODE_CPAP_BOOST					= 0x0020,
	MODE_BILEVEL_AT_RATE			= 0x0010,
	MODE_PRESSURE_SUPPORT			= 0x0008,
	MODE_BILEVEL_BACKUP_RATE		= 0x0004,
	MODE_AUTO_CPAP					= 0x0002,
	MODE_AUTO_BILEVEL				= 0x0001,
} CLINICAL_MODES;

/****** PARAM - CPAP ******/
typedef enum {
	MODEPARAM_CPAP_MASK1 = 0x0100,
	MODEPARAM_CPAP_MASK2 = 0x0080,
	MODEPARAM_CPAP_MASK3 = 0x0040,
	MODEPARAM_CPAP_MASK4 = 0x0020,
	MODEPARAM_CPAP_MASK5 = 0x001C,

	MODEPARAM_CPAP_MASK_COUNT = 5
} MODEPARAM_CPAP_MASK;

/****** PARAM - minCPAP ******/
typedef enum {
	MODEPARAM_MINCPAP_MASK1 = 0x0002,

	MODEPARAM_MINCPAP_MASK_COUNT = 1
} MODEPARAM_MINCPAP;

/****** PARAM - maxCPAP ******/
typedef enum {
	MODEPARAM_MAXCPAP_MASK1 = 0x0002,

	MODEPARAM_MAXCPAP_MASK_COUNT = 1
} MODEPARAM_MAXCPAP;

/****** PARAM - maxIPAP ******/
typedef enum {
	MODEPARAM_MAXIPAP_MASK1 = 0x0002,

	MODEPARAM_MAXIPAP_MASK_COUNT = 1
} MODEPARAM_MAXIPAP;

/****** PARAM - IPAP ******/
typedef enum {
	MODEPARAM_IPAP_MASK1 = 0x0010,
	MODEPARAM_IPAP_MASK2 = 0x0008,
	MODEPARAM_IPAP_MASK3 = 0x0004,

	MODEPARAM_IPAP_MASK_COUNT = 3
} MODEPARAM_IPAP;

/****** PARAM - fixed EPAP CPAP ******/
typedef enum {
	MODEPARAM_FIXED_EPAP_CPAP_MASK1 = 0x001C,

	MODEPARAM_FIXED_EPAP_CPAP_MASK_COUNT = 1
} MODEPARAM_FIXED_EPAP_CPAP;

/****** PARAM - min EPAP CPAP ******/
typedef enum {
	MODEPARAM_MIN_EPAP_CPAP_MASK1 = 0x0002,

	MODEPARAM_MIN_EPAP_CPAP_MASK_COUNT = 1
} MODEPARAM_MIN_EPAP_CPAP;

/****** PARAM - pressure support ******/
typedef enum {
	MODEPARAM_PS_SUPPORT_MASK1 = 0x000C,

	MODEPARAM_PS_SUPPORT_MASK_COUNT = 1
} MODEPARAM_PS_SUPPORT;

/****** PARAM - RR ******/
typedef enum {
	MODEPARAM_RR_MASK1 = 0x0014,

	MODEPARAM_RR_MASK_COUNT = 1
} MODEPARAM_RR;

/****** PARAM - Rise Time ******/
typedef enum {
	MODEPARAM_RISETIME_MASK1 = 0x001C,

	MODEPARAM_RISETIME_MASK_COUNT = 1
} MODEPARAM_RISETIME;

/****** PARAM - Fixed TI ******/
typedef enum {
	MODEPARAM_FIXEDTI_MASK1 = 0x0010,

	MODEPARAM_FIXEDTI_MASK_COUNT = 1
} MODEPARAM_FIXEDTI;

/****** PARAM - Max TI ******/
typedef enum {
	MODEPARAM_MAXTI_MASK1 = 0x000E,

	MODEPARAM_MAXTI_MASK_COUNT = 1
} MODEPARAM_MAXTI;

/****** PARAM - Min TI ******/
typedef enum {
	MODEPARAM_MINTI_MASK1 = 0x000E,

	MODEPARAM_MINTI_MASK_COUNT = 1
} MODEPARAM_MINTI;

/****** PARAM - exhalation detection threshold ******/
typedef enum {
	MODEPARAM_EDT_MASK1 = 0x0002,

	MODEPARAM_EDT_MASK_COUNT = 1
} MODEPARAM_EDT;

#if 0
/**
 * @brief:
 * Function pointer prototype for settings having extra functionality than normal on/off/value change
 * e.g. The Hotspot setting after turning on, needs to show some messages to the user.
 * This function should get called when the value of a setting is changed
 * */
typedef int (*settings_extra_func)(
							const char *display_name,
							const char *setting_path,
							app_display_key_cb prev_screen_cb
							);
#endif

/********************************************************************
 * helper inline functions
 *********************************************************************/
/**
 * @brief Helper function for converting struct sensor_value to double.
 *
 * @param val A pointer to a sensor_value struct.
 * @return The converted value.
 */
static inline double setting_value_to_double(const struct setting_value *val)
{
	return (double)val->val1 + (double)val->val2 / 1000000;
}

/**
 * @brief Helper function for converting double to struct sensor_value.
 *
 * @param val A pointer to a sensor_value struct.
 * @param inp The converted value.
 * @return 0 if successful, negative errno code if failure.
 */
static inline int setting_value_from_double(struct setting_value *val, double inp)
{
	if (inp < INT32_MIN || inp > INT32_MAX) {
		return -ERANGE;
	}

	double val2 = (inp - (int32_t)inp) * 1000000.0;

	if (val2 < INT32_MIN || val2 > INT32_MAX) {
		return -ERANGE;
	}

	val->val1 = (int32_t)inp;
	val->val2 = (int32_t)val2;

	return 0;
}

/********************************************************************
 * function declarations
 *********************************************************************/
uint16_t app_settings_curr_mode_get();
uint16_t app_settings_wifi_stat_get();
void app_settings_value_init();

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_VALUE_H_ */
