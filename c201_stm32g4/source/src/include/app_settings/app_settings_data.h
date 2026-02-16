/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 16-Mar-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DATA_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DATA_H_

#include <stdint.h>
#include <stdbool.h>
#include "app_settings_paths.h"
#include "app_settings_value.h"
#include "app_settings_def.h"

#define SETTINGS_COUNT_MAX	79	// Number of settings. This must be exact value

typedef enum {
	SETTING_DATATYPE_NONE,
	SETTING_DATATYPE_UINT8,
	SETTING_DATATYPE_UINT16,
	SETTING_DATATYPE_UINT32,
	SETTING_DATATYPE_INT,
	SETTING_DATATYPE_DOUBLE,
	SETTING_DATATYPE_CHAR,
	SETTING_DATATYPE_STRING,
	SETTING_DATATYPE_DATE,
	SETTING_DATATYPE_TIME,
	SETTING_DATATYPE_SETTING_VALUE,
	SETTING_DATATYPE_WIFI_STA_CFG,
} SETTING_DATATYPE;

typedef enum {
	SETTING_DISP_NO=0,				// do not display
	SETTING_DISP_YES,				// display
	SETTING_DISP_COND_MODE,			// conditional display depending on mode
	SETTING_DISP_MULTILEVEL,		// will have more than one levels / screens to set values
	SETTING_DISP_COND_WIFI,			// conditional display depending on wifi status
//	SETTING_DISP_COND_WIFI_MULTI,	// conditional display depending on wifi status, will have more than one levels / screens to set values
	SETTING_DISP_NO_MULTILEVEL,		// do not display in settings tree but have more than one levels / screens
} SETTING_DISPLAYABLE;

typedef enum {
	SETTING_EDIT_NO=0,				// not editable
	SETTING_EDIT_YES,				// editable
	SETTING_EDIT_PASSWORD,			// editable password field
} SETTING_EDITABLE;

struct setting_mode_params {
	uint8_t num_params;						// number of option a setting will have
	struct app_settings_param_value const *param_val;	// list of parameters
};

struct setting_value_options {
	uint8_t num_options;						// number of option a setting will have
	struct app_settings_value const *op_val;	// the list of options
};

struct app_settings_data {
	const char fullpath[SETTINGS_FULLPATH_LEN_MAX];	// path of a setting in the tree
	int display_order;								// order in which the seetings should be displayed
	char name[SETTINGS_NAME_LEN_MAX];				// name of the settings (can be used for displaying)
//	char *disp_val;									// the value of the settings in string to be displayed
	uint8_t displayable;							// whether a setting is displayable (see enum SETTINGS_DISPLAYABLE)
	uint8_t editable;								// whether a setting is editable
	uint8_t datatype;								// datatype of the value (see enum SETTING_DATATYPE)
	uint32_t size;									// size of the settings value
	struct setting_value_options *options;			// options a setting value can have, i.e. list of values
	struct setting_mode_params *params;				// if a setting's value is dependent on the current running mode (param), then it has an object else NULL
};

struct settings_runtime_value {
	struct app_settings_data const *settings_data;	// reference to app_settings_data object
	void *val;										// the value of the settings, memory will be allocated according to struct app_settings_data.size
	char *disp_val;									// the value of the settings in string to be displayed
	uint32_t len_max;								// maximum length of the disp_val string, memory will be allocated according to this
};

/********************************************************************
 * App settings data extern variable to be used by other modules
 *********************************************************************/
extern const struct app_settings_data g_sdata[SETTINGS_COUNT_MAX];

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
extern struct settings_runtime_value g_disp_val[SETTINGS_COUNT_MAX];
#endif
/********************************************************************/

/********************************************************************
 * Setting value extern variables to be used by other module
 *********************************************************************/
/* DEVICE SETTINGS */
extern struct setting_value_options g_mdata_Date[3];
extern struct setting_value_options g_mdata_Time[2];
extern struct setting_value_options g_mdata_Freset;
extern struct setting_value_options g_mdata_ptAccExt;
extern struct setting_value_options g_mdata_EraseData;
extern struct setting_value_options g_mdata_lang;
extern struct setting_value_options g_mdata_WiFi;
extern struct setting_value_options g_mdata_SSID;
extern struct setting_value_options g_mdata_WiFiAP;

/* BLOWER SETTINGS */
extern struct setting_value_options g_mdata_BlwState;

/* COMFORT SETTINGS */
extern struct setting_value_options g_mdata_PlaceOfUse;
extern struct setting_value_options g_mdata_Humidity;
extern struct setting_value_options g_mdata_HeaterState;
extern struct setting_value_options g_mdata_Temp;
extern struct setting_value_options g_mdata_RampTime;
extern struct setting_value_options g_mdata_SettleTime;
extern struct setting_value_options g_mdata_AutoOn;
extern struct setting_value_options g_mdata_AutoOff;
extern struct setting_value_options g_mdata_LeakAlert;
extern struct setting_value_options g_mdata_SensorLocation;

/* CLINICAL / TECHNICAL SETTINGS */
extern struct setting_value_options g_mdata_Mode;
extern struct setting_value_options g_mdata_CPAP;

/* ADVANCED TECH SETTINGS */
extern struct setting_value_options g_mdata_Trigger;
extern struct setting_value_options g_mdata_BatteryStatus;
extern struct setting_value_options g_mdata_Cycle;
extern struct setting_value_options g_mdata_TubeLength;
extern struct setting_value_options g_mdata_MaxSettle;

/* DEVELOPER SETTINGS */
extern struct setting_value_options g_mdata_BlowerMode;
extern struct setting_value_options g_mdata_devUSB;
extern struct setting_value_options g_mdata_reloadSettings;

/********************************************************************/


#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTINGS_DATA_H_ */
