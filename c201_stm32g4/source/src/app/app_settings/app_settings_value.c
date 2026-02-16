/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 10-Feb-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "app_time/app_time.h"
#include "app_net/app_net.h"
#include "app_settings/app_settings.h"
#include "app_settings/app_settings_data.h"
#include "app_settings/app_settings_value.h"
#include "app_settings/app_settings_paths.h"

/*Included this file for macro in WIFI ssid */
#include "app_settings/app_settings_def.h"
//#include "app_display/app_display.h"

#define DATETIME_MAKE_RUNTIME 	0

/* Common options */
const static struct app_settings_value low_med_high[3] = {
		{.key = "Low", .val.val1 = 0},
		{.key = "Medium", .val.val1 = 1},
		{.key = "High", .val.val1 = 2},
};

const static struct app_settings_value yes_no_val[2] = {
		{.key = "No", .val.val1 = 0},
		{.key = "Yes", .val.val1 = 1},
};

const static struct app_settings_value on_off_val[2] = {
		{.key = "Off", .val.val1 = 0},
		{.key = "On", .val.val1 = 1},
};


/********************************************************************
 * DEVICE SETTINGS
 *********************************************************************/
/* Date and Time */
#if DATETIME_MAKE_RUNTIME
static struct app_settings_value date_year[10];
static struct app_settings_value date_mon[12];
static struct app_settings_value date_day[31];
static struct app_settings_value date_hour[24];
static struct app_settings_value date_minute[60];
//static struct app_settings_value date_sec[60];
#else
const static struct app_settings_value date_year[10] = {
		{.key = "2023", .val.val1 = 2023},
		{.key = "2024", .val.val1 = 2024},
		{.key = "2025", .val.val1 = 2025},
		{.key = "2026", .val.val1 = 2026},
		{.key = "2027", .val.val1 = 2027},
		{.key = "2028", .val.val1 = 2028},
		{.key = "2029", .val.val1 = 2029},
		{.key = "2030", .val.val1 = 2030},
		{.key = "2031", .val.val1 = 2031},
		{.key = "2032", .val.val1 = 2032},
};

const static struct app_settings_value date_mon[12] = {
		{.key = "Jan", .val.val1 = 1},
		{.key = "Feb", .val.val1 = 2},
		{.key = "Mar", .val.val1 = 3},
		{.key = "Apr", .val.val1 = 4},
		{.key = "May", .val.val1 = 5},
		{.key = "Jun", .val.val1 = 6},
		{.key = "Jul", .val.val1 = 7},
		{.key = "Aug", .val.val1 = 8},
		{.key = "Sep", .val.val1 = 9},
		{.key = "Oct", .val.val1 = 10},
		{.key = "Nov", .val.val1 = 11},
		{.key = "Dec", .val.val1 = 12},
};

const static struct app_settings_value date_day[31] = {
		{.key = "1", .val.val1 = 1}, {.key = "2", .val.val1 = 2}, {.key = "3", .val.val1 = 3},
		{.key = "4", .val.val1 = 4}, {.key = "5", .val.val1 = 5}, {.key = "6", .val.val1 = 6},
		{.key = "7", .val.val1 = 7}, {.key = "8", .val.val1 = 8}, {.key = "9", .val.val1 = 9},
		{.key = "10", .val.val1 = 10}, {.key = "11", .val.val1 = 11}, {.key = "12", .val.val1 = 12},
		{.key = "13", .val.val1 = 13}, {.key = "14", .val.val1 = 14}, {.key = "15", .val.val1 = 15},
		{.key = "16", .val.val1 = 16}, {.key = "17", .val.val1 = 17}, {.key = "18", .val.val1 = 18},
		{.key = "19", .val.val1 = 19}, {.key = "20", .val.val1 = 20}, {.key = "21", .val.val1 = 21},
		{.key = "22", .val.val1 = 22}, {.key = "23", .val.val1 = 23}, {.key = "24", .val.val1 = 24},
		{.key = "25", .val.val1 = 25}, {.key = "26", .val.val1 = 26}, {.key = "27", .val.val1 = 27},
		{.key = "28", .val.val1 = 28}, {.key = "29", .val.val1 = 29}, {.key = "30", .val.val1 = 30},
		{.key = "31", .val.val1 = 31}
};

const static struct app_settings_value date_hour[24] = {
		{.key = "0", .val.val1 = 0}, {.key = "1", .val.val1 = 1}, {.key = "2", .val.val1 = 2},
		{.key = "3", .val.val1 = 3}, {.key = "4", .val.val1 = 4}, {.key = "5", .val.val1 = 5},
		{.key = "6", .val.val1 = 6}, {.key = "7", .val.val1 = 7}, {.key = "8", .val.val1 = 8},
		{.key = "9", .val.val1 = 9}, {.key = "10", .val.val1 = 10}, {.key = "11", .val.val1 = 11},
		{.key = "12", .val.val1 = 12}, {.key = "13", .val.val1 = 13}, {.key = "14", .val.val1 = 14},
		{.key = "15", .val.val1 = 15}, {.key = "16", .val.val1 = 16}, {.key = "17", .val.val1 = 17},
		{.key = "18", .val.val1 = 18}, {.key = "19", .val.val1 = 19}, {.key = "20", .val.val1 = 20},
		{.key = "21", .val.val1 = 21}, {.key = "22", .val.val1 = 22}, {.key = "23", .val.val1 = 23},
};

const static struct app_settings_value date_minute[60] = {
		{.key = "0", .val.val1 = 0}, {.key = "1", .val.val1 = 1}, {.key = "2", .val.val1 = 2},
		{.key = "3", .val.val1 = 3}, {.key = "4", .val.val1 = 4}, {.key = "5", .val.val1 = 5},
		{.key = "6", .val.val1 = 6}, {.key = "7", .val.val1 = 7}, {.key = "8", .val.val1 = 8},
		{.key = "9", .val.val1 = 9}, {.key = "10", .val.val1 = 10}, {.key = "11", .val.val1 = 11},
		{.key = "12", .val.val1 = 12}, {.key = "13", .val.val1 = 13}, {.key = "14", .val.val1 = 14},
		{.key = "15", .val.val1 = 15}, {.key = "16", .val.val1 = 16}, {.key = "17", .val.val1 = 17},
		{.key = "18", .val.val1 = 18}, {.key = "19", .val.val1 = 19}, {.key = "20", .val.val1 = 20},
		{.key = "21", .val.val1 = 21}, {.key = "22", .val.val1 = 22}, {.key = "23", .val.val1 = 23},
		{.key = "24", .val.val1 = 24}, {.key = "25", .val.val1 = 25}, {.key = "26", .val.val1 = 26},
		{.key = "27", .val.val1 = 27}, {.key = "28", .val.val1 = 28}, {.key = "29", .val.val1 = 29},
		{.key = "30", .val.val1 = 30}, {.key = "31", .val.val1 = 31}, {.key = "32", .val.val1 = 32},
		{.key = "33", .val.val1 = 33}, {.key = "34", .val.val1 = 34}, {.key = "35", .val.val1 = 35},
		{.key = "36", .val.val1 = 36}, {.key = "37", .val.val1 = 37}, {.key = "38", .val.val1 = 38},
		{.key = "39", .val.val1 = 39}, {.key = "40", .val.val1 = 40}, {.key = "41", .val.val1 = 41},
		{.key = "42", .val.val1 = 42}, {.key = "43", .val.val1 = 43}, {.key = "44", .val.val1 = 44},
		{.key = "45", .val.val1 = 45}, {.key = "46", .val.val1 = 46}, {.key = "47", .val.val1 = 47},
		{.key = "48", .val.val1 = 48}, {.key = "49", .val.val1 = 49}, {.key = "50", .val.val1 = 50},
		{.key = "51", .val.val1 = 51}, {.key = "52", .val.val1 = 52}, {.key = "53", .val.val1 = 53},
		{.key = "54", .val.val1 = 54}, {.key = "55", .val.val1 = 55}, {.key = "56", .val.val1 = 56},
		{.key = "57", .val.val1 = 57}, {.key = "58", .val.val1 = 58}, {.key = "59", .val.val1 = 59}
};
#endif
struct setting_value_options g_mdata_Date[3] = {
					{.num_options=10, .op_val=date_year},
					{.num_options=12, .op_val=date_mon},
					{.num_options=31, .op_val=date_day}
};

struct setting_value_options g_mdata_Time[2] = {
					{.num_options=24, .op_val=date_hour},
					{.num_options=60, .op_val=date_minute},
//					{.num_options=60, .op_val=date_sec}
};

/* Factory Reset */
struct setting_value_options g_mdata_Freset = {
		.num_options = 2, .op_val = yes_no_val
};

/* Pt access Ext */
struct setting_value_options g_mdata_ptAccExt = {
		.num_options = 2, .op_val = yes_no_val
};

/* Erase Data */
struct setting_value_options g_mdata_EraseData = {
		.num_options = 2, .op_val = yes_no_val
};

/* language */
const static struct app_settings_value lang[2] = {
		{.key = "EN", .val.val1 = 0},
		{.key = "FR", .val.val1 = 1},
};
struct setting_value_options g_mdata_lang = {
		.num_options = 2, .op_val = lang
};

/* Wi-Fi */
static struct app_settings_value wifi_ssid[10] = {
		{.key = "", .val.val1 = 0},
		{.key = "", .val.val1 = 1},
		{.key = "", .val.val1 = 2},
		{.key = "", .val.val1 = 3},
		{.key = "", .val.val1 = 4},
		{.key = "", .val.val1 = 5},
		{.key = "", .val.val1 = 6},
		{.key = "", .val.val1 = 7},
		{.key = "", .val.val1 = 8},
		{.key = "", .val.val1 = 9},
};

struct setting_value_options g_mdata_WiFi = {
		.num_options = 2, .op_val = on_off_val
};
struct setting_value_options g_mdata_SSID = {
		.num_options = 10, .op_val = wifi_ssid	// SSID options will be populated at run time
};
struct setting_value_options g_mdata_WiFiAP = {
		.num_options = 2, .op_val = on_off_val
};

/* Blower */
struct setting_value_options g_mdata_BlwState = {
		.num_options = 2, .op_val = on_off_val
};

/********************************************************************
 * COMFORT SETTINGS
 *********************************************************************/
/* place of use */
const static struct app_settings_value place_of_use[2] = {
		{.key = "Head", .val.val1 = 0},
		{.key = "Sidetable", .val.val1 = 1},
};
struct setting_value_options g_mdata_PlaceOfUse = {
		.num_options = 2, .op_val = place_of_use
};

/* humidity */
const static struct app_settings_value humidity[10] = {
		{.key = "Not connected", .val.val1 = -1},
		{.key = "0%", .val.val1 = 0},
		{.key = "20%", .val.val1 = 20},
		{.key = "40%", .val.val1 = 40},
		{.key = "50%", .val.val1 = 50},
		{.key = "60%", .val.val1 = 60},
		{.key = "70%", .val.val1 = 70},
		{.key = "80%", .val.val1 = 80},
		{.key = "90%", .val.val1 = 90},
		{.key = "100%", .val.val1 = 100},
};
struct setting_value_options g_mdata_Humidity = {
		.num_options = 10, .op_val = humidity
};

/* ramp time */
const static struct app_settings_value RampTime[5] = {
		{.key = "none", .val.val1 = 0},
		{.key = "5 min", .val.val1 = 5},
		{.key = "10 min", .val.val1 = 10},
		{.key = "15 min", .val.val1 = 15},
		{.key = "30 min", .val.val1 = 30},
};
struct setting_value_options g_mdata_RampTime = {
		.num_options = 5, .op_val = RampTime
};

/* settle time */
const static struct app_settings_value SettleTime[5] = {
		{.key = "none", .val.val1 = 0},
		{.key = "15 min", .val.val1 = 15},
		{.key = "30 min", .val.val1 = 30},
		{.key = "45 min", .val.val1 = 45},
		{.key = "60 min", .val.val1 = 60},
};
struct setting_value_options g_mdata_SettleTime = {
		.num_options = 5, .op_val = SettleTime
};

/* Enable autoOn */
struct setting_value_options g_mdata_AutoOn = {
		.num_options = 2, .op_val = yes_no_val
};

/* Enable autoOff */
struct setting_value_options g_mdata_AutoOff = {
		.num_options = 2, .op_val = yes_no_val
};

/*sensor location*/
const static struct app_settings_value sensor_location[11] = {
		{.key = "1", .val.val1 = 1},
		{.key = "2", .val.val1 = 2},
		{.key = "3", .val.val1 = 3},
		{.key = "4", .val.val1 = 4},
		{.key = "5", .val.val1 = 5},
		{.key = "6", .val.val1 = 6},
		{.key = "7", .val.val1 = 7},
		{.key = "8", .val.val1 = 8},
		{.key = "9", .val.val1 = 9},
		{.key = "10", .val.val1 = 10},
		{.key = "11", .val.val1 = 11},
};

struct setting_value_options g_mdata_SensorLocation = {
		.num_options = 11, .op_val = sensor_location
};
///////////////////////////////////////////////////////
/* Heater */
struct setting_value_options g_mdata_HeaterState = {
		.num_options = 2, .op_val = on_off_val
};

/* Leak Alert */
struct setting_value_options g_mdata_LeakAlert = {
		.num_options = 2, .op_val = yes_no_val
};


const static struct app_settings_value temp[11] = {
		{.key = "-1", .val.val1 = -1},
		{.key = "0", .val.val1 = 0},
		{.key = "25", .val.val1 = 25},
		{.key = "30", .val.val1 = 30},
		{.key = "35", .val.val1 = 35},
		{.key = "40", .val.val1 = 40},
		{.key = "45", .val.val1 = 45},
		{.key = "50", .val.val1 = 50},
		{.key = "55", .val.val1 = 55},
		{.key = "60", .val.val1 = 60},
		{.key = "100", .val.val1 = 100},

};
struct setting_value_options g_mdata_Temp = {
		.num_options = 11, .op_val = temp
};

/********************************************************************
 * CLINICAL / TECHNICAL SETTINGS
 *********************************************************************/
/* Mode */
const static struct app_settings_value mode[CLINICAL_MODE_MAX] = {
		{.key = "SnoreStop", .val.val1 = MODE_SNORESTOP},
		{.key = "PAPR", .val.val1 = MDOE_PAPR},
		{.key = "CPAP", .val.val1 = MODE_CPAP},
		{.key = "CPAP Boost", .val.val1 = MODE_CPAP_BOOST},
		{.key = "BiLevel at Rate", .val.val1 = MODE_BILEVEL_AT_RATE},
		{.key = "Pressure Support", .val.val1 = MODE_PRESSURE_SUPPORT},
		{.key = "BiLevel Backup Rate", .val.val1 = MODE_BILEVEL_BACKUP_RATE},
		{.key = "autoCPAP", .val.val1 = MODE_AUTO_CPAP},
		{.key = "autoBiLevel", .val.val1 = MODE_AUTO_BILEVEL},
};
struct setting_value_options g_mdata_Mode = {
		.num_options = CLINICAL_MODE_MAX, .op_val = mode
};

/* options for CPAP, minCPAP, maxCPAP, IPAP, fixed EPAP, minEPAP */
const static struct app_settings_value options_CPAP[41] = {
		{"0", {0,0}},				// 0
		{"0.5", {0,500000}},		// 1
		{"1.0", {1,0}},				// 2
		{"1.5", {1,500000}},		// 3
		{"2.0", {2,0}},				// 4
		{"2.5", {2,500000}},		// 5
		{"3.0", {3,0}},				// 6
		{"3.5", {3,500000}},		// 7
		{"4.0", {4,0}},				// 8
		{"4.5", {4,500000}},		// 9
		{"5.0", {5,0}},				// 10
		{"5.5", {5,500000}},		// 11
		{"6.0", {6,0}},				// 12
		{"6.5", {6,500000}},		// 13
		{"7.0", {7,0}},				// 14
		{"7.5", {7,500000}},		// 15
		{"8.0", {8,0}},				// 16
		{"8.5", {8,500000}},		// 17
		{"9.0", {9,0}},				// 18
		{"9.5", {9,500000}},		// 19
		{"10.0", {10,0}},			// 20
		{"10.5", {10,500000}},		// 21
		{"11.0", {11,0}},			// 22
		{"11.5", {11,500000}},		// 23
		{"12.0", {12,0}},			// 24
		{"12.5", {12,500000}},		// 25
		{"13.0", {13,0}},			// 26
		{"13.5", {13,500000}},		// 27
		{"14.0", {14,0}},			// 28
		{"14.5", {14,500000}},		// 29
		{"15.0", {15,0}},			// 30
		{"15.5", {15,500000}},		// 31
		{"16.0", {16,0}},			// 32
		{"16.5", {16,500000}},		// 33
		{"17.0", {17,0}},			// 34
		{"17.5", {17,500000}},		// 35
		{"18.0", {18,0}},			// 36
		{"18.5", {18,500000}},		// 37
		{"19.0", {19,0}},			// 38
		{"19.5", {19,500000}},		// 39
		{"20.0", {20,0}},			// 40
};

/* CPAP, minCPAP, maxCPAP, IPAP ... */
struct setting_value_options g_mdata_CPAP = {
		.num_options = 41, .op_val = options_CPAP
};

/********************************************************************
 * ADVANCED TECH SETTINGS
 *********************************************************************/
/* trigger */
struct setting_value_options g_mdata_Trigger = {
		.num_options = 3, .op_val = low_med_high
};

const static struct app_settings_value battery_status[2] = {
		{.key = "Connected", .val.val1 = 0},
		{.key = "Disconnected", .val.val1 = 1},
};

struct setting_value_options g_mdata_BatteryStatus = {
		.num_options = 2, .op_val = battery_status
};


/* cycle */
struct setting_value_options g_mdata_Cycle = {
		.num_options = 3, .op_val = low_med_high
};

/* tube length */
const static struct app_settings_value tube_length[4] = {
		{.key = "0 mtr", .val.val1 = 0},
		{.key = "1 mtr", .val.val1 = 1},
		{.key = "2 mtr", .val.val1 = 2},
		{.key = "3 mtr", .val.val1 = 3},
};
struct setting_value_options g_mdata_TubeLength = {
		.num_options = 4, .op_val = tube_length
};

/* max settle */
struct setting_value_options g_mdata_MaxSettle = {
		.num_options = 5, .op_val = SettleTime
};

/********************************************************************
 * DEVELOPER SETTINGS
 *********************************************************************/
/* Blower mode select */
const static struct app_settings_value blower_mode[2] = {
		{.key = "PID", .val.val1 = 0},
		{.key = "Test", .val.val1 = 1},
};
struct setting_value_options g_mdata_BlowerMode = {
		.num_options = 2, .op_val = blower_mode
};

/* USB select */
const static struct app_settings_value usb_select[3] = {
		{.key = "STM32", .val.val1 = 0},
		{.key = "ESP32", .val.val1 = 1},
		{.key = "Charger", .val.val1 = 2},
};
struct setting_value_options g_mdata_devUSB = {
		.num_options = 3, .op_val = usb_select
};

struct setting_value_options g_mdata_reloadSettings = {
		.num_options = 2, .op_val = yes_no_val
};



#if DATETIME_MAKE_RUNTIME
static const char *format_time(struct tm *tp, long nsec)
{
	static char buf[10] = {0x00};
	char *bp = buf;
	char *const bpe = bp + sizeof(buf);

//	bp += strftime(bp, bpe - bp, "%Y-%m-%d %H:%M:%S", tp);
//	bp += strftime(bp, bpe - bp, "%m-%d,%H:%M", tp);
//	bp += strftime(bp, bpe - bp, "%H:%M", tp);
//	bp += strftime(bp, bpe - bp, "%d%b,%k:%M", tp);
//	bp += strftime(bp, bpe - bp, "%k:%M", tp);
	bp += strftime(bp, bpe - bp, "%b", tp);
	if (nsec >= 0) {
		bp += snprintf(bp, bpe - bp, ".%09lu", nsec);
	}
//	bp += strftime(bp, bpe - bp, " %a %j", tp);
	return buf;
}

static void make_datetime_data(uint8_t choice, struct app_settings_value *dt)
{
	int min_val, max_val;
	int count=0;
	switch (choice) {
		case YEAR:
			min_val = 2023;
			max_val = 2040;
		break;

		case MONTH:
			min_val=1;
			max_val=12;
		break;

		case DAY:
			min_val=1;
			max_val=31;
		break;

		case HOUR:
			min_val=0;
			max_val=23;
		break;

		case MINUTE:
			min_val=0;
			max_val=59;
		break;

		case SECOND:
			min_val=0;
			max_val=59;
		break;

		default:
		break;
	}

	count = max_val-min_val;
//	printf("count = %d\n", count);
	for (int i=0; i<=count; i++) {
		if (choice == MONTH) {
			struct tm time;
			time.tm_mon = min_val-1;
			sprintf(dt[i].key, "%s", format_time(&time, -1));
		} else {
			sprintf(dt[i].key, "%d", min_val);
		}
		dt[i].val.val1 = min_val++;
	}
}
#endif

uint16_t app_settings_curr_mode_get()
{
	struct setting_value val;
	app_settings_load_single(SETTINGS_KEY_FULL_TS_MOD, &val, sizeof(struct setting_value));
	return val.val1;
}

uint16_t app_settings_wifi_stat_get()
{
	struct setting_value val;
	app_settings_load_single(SETTINGS_KEY_FULL_DS_NET_WI, &val, sizeof(struct setting_value));
	return val.val1;
}

void app_settings_value_init()
{
#if DATETIME_MAKE_RUNTIME
	/* date */
	make_datetime_data(YEAR, (struct app_settings_value*) g_mdata_Date[0].op_val);
	make_datetime_data(MONTH, (struct app_settings_value*) g_mdata_Date[1].op_val);
	make_datetime_data(DAY, (struct app_settings_value*) g_mdata_Date[2].op_val);
	/* time */
	make_datetime_data(HOUR, (struct app_settings_value*) g_mdata_Time[0].op_val);
	make_datetime_data(MINUTE, (struct app_settings_value*) g_mdata_Time[1].op_val);
//	make_datetime_data(SECOND, g_mdata_Time[2].op_val);
#endif
}


/********************************************************************
 * CLINICAL / TECHNICAL SETTINGS DATA
 *********************************************************************/
/* Param Values */
static const struct app_settings_param_value modeParam_CPAP[MODEPARAM_CPAP_MASK_COUNT] = {
		{MODEPARAM_CPAP_MASK1, "Level", 2, 6},
		{MODEPARAM_CPAP_MASK2, "Level", 6, 12},
		{MODEPARAM_CPAP_MASK3, "", 8, 40},
		{MODEPARAM_CPAP_MASK4, "", 8, 34},
		{MODEPARAM_CPAP_MASK5, "EPAP", 8, 36},
};

static const struct app_settings_param_value modeParam_minCPAP[MODEPARAM_MINCPAP_MASK_COUNT] = {
		{MODEPARAM_MINCPAP_MASK1, "", 8, 33},
};

static const struct app_settings_param_value modeParam_maxCPAP[MODEPARAM_MAXCPAP_MASK_COUNT] = {
		{MODEPARAM_MAXCPAP_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_maxIPAP[MODEPARAM_MAXIPAP_MASK_COUNT] = {
		{MODEPARAM_MAXIPAP_MASK1, "maxIPAP or1", 8, 40},
};

static const struct app_settings_param_value modeParam_IPAP[MODEPARAM_IPAP_MASK_COUNT] = {
		{MODEPARAM_IPAP_MASK1, "IPAP or1", 8, 40},
		{MODEPARAM_IPAP_MASK2, "IPAP or2", 8, 40},
		{MODEPARAM_IPAP_MASK3, "IPAP or3", 8, 40},
};

static const struct app_settings_param_value modeParam_fixedEPAPCPAP[MODEPARAM_FIXED_EPAP_CPAP_MASK_COUNT] = {
		{MODEPARAM_FIXED_EPAP_CPAP_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_minEPAPCPAP[MODEPARAM_MIN_EPAP_CPAP_MASK_COUNT] = {
		{MODEPARAM_MIN_EPAP_CPAP_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_PSSupport[MODEPARAM_PS_SUPPORT_MASK_COUNT] = {
		{MODEPARAM_PS_SUPPORT_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_RR[MODEPARAM_RR_MASK_COUNT] = {
		{MODEPARAM_RR_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_riseTime[MODEPARAM_RISETIME_MASK_COUNT] = {
		{MODEPARAM_RISETIME_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_fixedTI[MODEPARAM_FIXEDTI_MASK_COUNT] = {
		{MODEPARAM_FIXEDTI_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_maxTI[MODEPARAM_MAXTI_MASK_COUNT] = {
		{MODEPARAM_MAXTI_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_minTI[MODEPARAM_MINTI_MASK_COUNT] = {
		{MODEPARAM_MINTI_MASK1, "", 8, 40},
};

static const struct app_settings_param_value modeParam_EDT[MODEPARAM_EDT_MASK_COUNT] = {
		{MODEPARAM_EDT_MASK1, "", 8, 40},
};

/* Mode Params */
struct setting_mode_params g_modeParam_CPAP = {.num_params = MODEPARAM_CPAP_MASK_COUNT, .param_val = modeParam_CPAP};
struct setting_mode_params g_modeParam_minCPAP = {.num_params = MODEPARAM_MINCPAP_MASK_COUNT, .param_val = modeParam_minCPAP};
struct setting_mode_params g_modeParam_maxCPAP = {.num_params = MODEPARAM_MAXCPAP_MASK_COUNT, .param_val = modeParam_maxCPAP};
struct setting_mode_params g_modeParam_maxIPAP = {.num_params = MODEPARAM_MAXIPAP_MASK_COUNT, .param_val = modeParam_maxIPAP};
struct setting_mode_params g_modeParam_IPAP = {.num_params = MODEPARAM_IPAP_MASK_COUNT, .param_val = modeParam_IPAP};
struct setting_mode_params g_modeParam_fixedEPAPCPAP = {.num_params = MODEPARAM_FIXED_EPAP_CPAP_MASK_COUNT, .param_val = modeParam_fixedEPAPCPAP};
struct setting_mode_params g_modeParam_minEPAPCPAP = {.num_params = MODEPARAM_MIN_EPAP_CPAP_MASK_COUNT, .param_val = modeParam_minEPAPCPAP};
struct setting_mode_params g_modeParam_PSSupport = {.num_params = MODEPARAM_PS_SUPPORT_MASK_COUNT, .param_val = modeParam_PSSupport};
struct setting_mode_params g_modeParam_RR = {.num_params = MODEPARAM_RR_MASK_COUNT, .param_val = modeParam_RR};
struct setting_mode_params g_modeParam_riseTime = {.num_params = MODEPARAM_RISETIME_MASK_COUNT, .param_val = modeParam_riseTime};
struct setting_mode_params g_modeParam_fixedTI = {.num_params = MODEPARAM_FIXEDTI_MASK_COUNT, .param_val = modeParam_fixedTI};
struct setting_mode_params g_modeParam_maxTI = {.num_params = MODEPARAM_MAXTI_MASK_COUNT, .param_val = modeParam_maxTI};
struct setting_mode_params g_modeParam_minTI = {.num_params = MODEPARAM_MINTI_MASK_COUNT, .param_val = modeParam_minTI};
struct setting_mode_params g_modeParam_EDT = {.num_params = MODEPARAM_EDT_MASK_COUNT, .param_val = modeParam_EDT};

/********************************************************************
 * Menu display data
 *********************************************************************/
const struct app_settings_data g_sdata[SETTINGS_COUNT_MAX] =
{
		/********************************************************************
		 * The below lines are generated by codegen.
		 * Do not edit them! The sequence must be maintained
		 *********************************************************************/

		{SETTINGS_KEY_FULL_NAM, 1, "User", 1, 1, SETTING_DATATYPE_STRING, 20, NULL, NULL},        //0
                {SETTINGS_KEY_FULL_BST, 2, "Blower On/Off", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_BlwState, NULL},        //1
                {SETTINGS_KEY_FULL_DEV, 3, "Developer", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //2
                {SETTINGS_KEY_FULL_DEV_BMODE, 4, "Blower Mode", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_BlowerMode, NULL},        //3
                {SETTINGS_KEY_FULL_DEV_BRPM, 5, "Blower RPM", 1, 1, SETTING_DATATYPE_UINT32, sizeof(uint32_t), NULL, NULL},        //4
                {SETTINGS_KEY_FULL_DEV_BRAMP, 6, "Blower Ramp ms", 1, 1, SETTING_DATATYPE_UINT32, sizeof(uint32_t), NULL, NULL},        //5
                {SETTINGS_KEY_FULL_DEV_USB, 7, "USB select", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_devUSB, NULL},        //6
                {SETTINGS_KEY_FULL_TS, 8, "Tech Settings", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //7
                {SETTINGS_KEY_FULL_TS_MOD, 9, "Mode", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Mode, NULL},        //8
                {SETTINGS_KEY_FULL_TS_FIC, 10, "CPAP", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_CPAP},        //9
                {SETTINGS_KEY_FULL_TS_MNC, 11, "min CPAP", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_minCPAP},        //10
                {SETTINGS_KEY_FULL_TS_MXC, 12, "max CPAP", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_maxCPAP},        //11
                {SETTINGS_KEY_FULL_TS_MXI, 13, "maxIPAP", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_maxIPAP},        //12
                {SETTINGS_KEY_FULL_TS_IPA, 14, "ipap", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_IPAP},        //13
                {SETTINGS_KEY_FULL_TS_FXE, 15, "fixed epap, cpap", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_fixedEPAPCPAP},        //14
                {SETTINGS_KEY_FULL_TS_MNE, 16, "minEPAP, minCPAP", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_minEPAPCPAP},        //15
                {SETTINGS_KEY_FULL_TS_PRS, 17, "Pressure Support", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_PSSupport},        //16
                {SETTINGS_KEY_FULL_TS_RR, 18, "rr", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_RR},        //17
                {SETTINGS_KEY_FULL_TS_RIT, 19, "rise time", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_riseTime},        //18
                {SETTINGS_KEY_FULL_TS_FXT, 20, "ti (fixed)", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_fixedTI},        //19
                {SETTINGS_KEY_FULL_TS_MXT, 21, "ti max", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_maxTI},        //20
                {SETTINGS_KEY_FULL_TS_MNT, 22, "ti min", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_minTI},        //21
                {SETTINGS_KEY_FULL_TS_EDT, 23, "exhalation detection threshold", 2, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_CPAP, &g_modeParam_EDT},        //22
                {SETTINGS_KEY_FULL_DS, 24, "Device Settings", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //23
                {SETTINGS_KEY_FULL_DS_SRN, 25, "SN", 1, 0, SETTING_DATATYPE_STRING, 20, NULL, NULL},        //24
                {SETTINGS_KEY_FULL_DS_DAT, 26, "Date", 1, 1, SETTING_DATATYPE_DATE, 0, g_mdata_Date, NULL},        //25
                {SETTINGS_KEY_FULL_DS_DAT_YR, 27, "Year", 0, 1, SETTING_DATATYPE_DATE, sizeof(struct setting_value), NULL, NULL},        //26
                {SETTINGS_KEY_FULL_DS_DAT_MON, 28, "Month", 0, 1, SETTING_DATATYPE_DATE, sizeof(struct setting_value), NULL, NULL},        //27
                {SETTINGS_KEY_FULL_DS_DAT_DAY, 29, "Day", 0, 1, SETTING_DATATYPE_DATE, sizeof(struct setting_value), NULL, NULL},        //28
                {SETTINGS_KEY_FULL_DS_TIM, 30, "Time", 1, 1, SETTING_DATATYPE_TIME, 0, g_mdata_Time, NULL},        //29
                {SETTINGS_KEY_FULL_DS_TIM_HR, 31, "Hour", 0, 1, SETTING_DATATYPE_TIME, sizeof(struct setting_value), NULL, NULL},        //30
                {SETTINGS_KEY_FULL_DS_TIM_MIN, 32, "Min", 0, 1, SETTING_DATATYPE_TIME, sizeof(struct setting_value), NULL, NULL},        //31
                {SETTINGS_KEY_FULL_DS_FRS, 33, "Factory Reset", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Freset, NULL},        //32
                {SETTINGS_KEY_FULL_DS_PAE, 34, "pt access exh", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_ptAccExt, NULL},        //33
                {SETTINGS_KEY_FULL_DS_ERD, 35, "Erase Data", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_EraseData, NULL},        //34
                {SETTINGS_KEY_FULL_DS_LAN, 36, "language", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_lang, NULL},        //35
                {SETTINGS_KEY_FULL_DS_NET, 37, "Network", 1, 1, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //36
                {SETTINGS_KEY_FULL_DS_NET_WIAP, 38, "Hotspot", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_WiFiAP, NULL},        //37
                {SETTINGS_KEY_FULL_DS_NET_WPWD, 39, "Password", 5, 2, SETTING_DATATYPE_STRING, 64, NULL, NULL},        //38
                {SETTINGS_KEY_FULL_DS_NET_WSTACFG, 40, "Wi-Fi STA config", 0, 1, SETTING_DATATYPE_WIFI_STA_CFG, sizeof(struct wifi_sta_config), NULL, NULL},        //39
                {SETTINGS_KEY_FULL_DS_NET_WMAC, 41, "Wi-Fi MAC", 1, 0, SETTING_DATATYPE_STRING, 20, NULL, NULL},        //40
                {SETTINGS_KEY_FULL_DS_NET_WIP, 42, "IP addr", 4, 0, SETTING_DATATYPE_STRING, 20, NULL, NULL},        //41
                {SETTINGS_KEY_FULL_DS_NET_WSSID, 43, "Select SSID", 5, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SSID, NULL},        //42
                {SETTINGS_KEY_FULL_DS_NET_WSSIDCON, 44, "Current SSID", 4, 0, SETTING_DATATYPE_STRING, 32, NULL, NULL},        //43
                {SETTINGS_KEY_FULL_DS_NET_WI, 45, "Wi-Fi", 3, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_WiFi, NULL},        //44
                {SETTINGS_KEY_FULL_CS, 46, "Comfort Settings", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //45
                {SETTINGS_KEY_FULL_CS_POU, 47, "Place of use", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_PlaceOfUse, NULL},        //46
                {SETTINGS_KEY_FULL_CS_HUM, 48, "Humidity", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Humidity, NULL},        //47
                {SETTINGS_KEY_FULL_CS_RTM, 49, "Ramp Time", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_RampTime, NULL},        //48
                {SETTINGS_KEY_FULL_CS_STM, 50, "Settle Time", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SettleTime, NULL},        //49
                {SETTINGS_KEY_FULL_CS_AON, 51, "AutoOn", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_AutoOn, NULL},        //50
                {SETTINGS_KEY_FULL_CS_AOF, 52, "AutoOff", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_AutoOff, NULL},        //51
                {SETTINGS_KEY_FULL_CS_LAL, 53, "Leak Alert", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_LeakAlert, NULL},        //52
                {SETTINGS_KEY_FULL_CS_HET, 54, "Heater Settings", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //53
                {SETTINGS_KEY_FULL_CS_HET_W24, 55, "Heater 24W", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //54
                {SETTINGS_KEY_FULL_CS_HET_W24HST, 56, "Heater on/off", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_HeaterState, NULL},        //55
                {SETTINGS_KEY_FULL_CS_HET_W24HUM, 57, "Humidity set", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Humidity, NULL},        //56
                {SETTINGS_KEY_FULL_CS_HET_W12, 58, "Heater 12W", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //57
                {SETTINGS_KEY_FULL_CS_HET_W12HST, 59, "Heater on/off", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_HeaterState, NULL},        //58
                {SETTINGS_KEY_FULL_CS_HET_W12TEMP, 60, "Temperature set", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Temp, NULL},        //59
                {SETTINGS_KEY_FULL_CS_SP, 61, "Sensor Placement", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //60
                {SETTINGS_KEY_FULL_CS_SP_PRESS, 62, "Pressure", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //61
                {SETTINGS_KEY_FULL_CS_SP_PRESSINW, 63, "Inhale wall", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //62
                {SETTINGS_KEY_FULL_CS_SP_PRESSINPT, 64, "Inhale pitot-tube", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //63
                {SETTINGS_KEY_FULL_CS_SP_PRESSEXW, 65, "Exhale wall", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //64
                {SETTINGS_KEY_FULL_CS_SP_PRESSEXPT, 66, "Exhale pitot-tube", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //65
                {SETTINGS_KEY_FULL_CS_SP_PRESSAMB, 67, "Ambient", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //66
                {SETTINGS_KEY_FULL_CS_SP_HUMI, 68, "Humidity", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //67
                {SETTINGS_KEY_FULL_CS_SP_HUMIINCH, 69, "Inhale channel", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //68
                {SETTINGS_KEY_FULL_CS_SP_DIST, 70, "Distance", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //69
                {SETTINGS_KEY_FULL_CS_SP_DISTWCH, 71, "Water chamber", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_SensorLocation, NULL},        //70
                {SETTINGS_KEY_FULL_AS, 72, "Adv Tech Settings", 1, 0, SETTING_DATATYPE_NONE, 0, NULL, NULL},        //71
                {SETTINGS_KEY_FULL_AS_LAL, 73, "Leak Alert", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_LeakAlert, NULL},        //72
                {SETTINGS_KEY_FULL_AS_BS, 74, "Battery Status", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_BatteryStatus, NULL},        //73
                {SETTINGS_KEY_FULL_AS_TRG, 75, "Trigger", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Trigger, NULL},        //74
                {SETTINGS_KEY_FULL_AS_CYC, 76, "Cycle", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_Cycle, NULL},        //75
                {SETTINGS_KEY_FULL_AS_TLN, 77, "tube length", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_TubeLength, NULL},        //76
                {SETTINGS_KEY_FULL_AS_MXS, 78, "max Settle", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_MaxSettle, NULL},        //77
                {SETTINGS_KEY_FULL_LOD, 79, "Reload Settings", 1, 1, SETTING_DATATYPE_SETTING_VALUE, sizeof(struct setting_value), &g_mdata_reloadSettings, NULL},        //78

			/* end */
};

#if (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM)
struct settings_runtime_value g_disp_val[SETTINGS_COUNT_MAX] =
{
		/********************************************************************
		 * The below lines are generated by codegen.
		 * Do not edit them! The sequence must be maintained
		 *********************************************************************/

		{&g_sdata[0], NULL, NULL, 21},        //0
                {&g_sdata[1], NULL, NULL, 4},        //1
                {&g_sdata[2], NULL, NULL, 0},        //2
                {&g_sdata[3], NULL, NULL, 5},        //3
                {&g_sdata[4], NULL, NULL, 7},        //4
                {&g_sdata[5], NULL, NULL, 7},        //5
                {&g_sdata[6], NULL, NULL, 8},        //6
                {&g_sdata[7], NULL, NULL, 0},        //7
                {&g_sdata[8], NULL, NULL, 30},        //8
                {&g_sdata[9], NULL, NULL, 6},        //9
                {&g_sdata[10], NULL, NULL, 6},        //10
                {&g_sdata[11], NULL, NULL, 6},        //11
                {&g_sdata[12], NULL, NULL, 6},        //12
                {&g_sdata[13], NULL, NULL, 6},        //13
                {&g_sdata[14], NULL, NULL, 6},        //14
                {&g_sdata[15], NULL, NULL, 6},        //15
                {&g_sdata[16], NULL, NULL, 6},        //16
                {&g_sdata[17], NULL, NULL, 6},        //17
                {&g_sdata[18], NULL, NULL, 6},        //18
                {&g_sdata[19], NULL, NULL, 6},        //19
                {&g_sdata[20], NULL, NULL, 6},        //20
                {&g_sdata[21], NULL, NULL, 6},        //21
                {&g_sdata[22], NULL, NULL, 6},        //22
                {&g_sdata[23], NULL, NULL, 0},        //23
                {&g_sdata[24], NULL, NULL, 21},        //24
                {&g_sdata[25], NULL, NULL, 0},        //25
                {&g_sdata[26], NULL, NULL, 5},        //26
                {&g_sdata[27], NULL, NULL, 3},        //27
                {&g_sdata[28], NULL, NULL, 3},        //28
                {&g_sdata[29], NULL, NULL, 0},        //29
                {&g_sdata[30], NULL, NULL, 3},        //30
                {&g_sdata[31], NULL, NULL, 3},        //31
                {&g_sdata[32], NULL, NULL, 4},        //32
                {&g_sdata[33], NULL, NULL, 4},        //33
                {&g_sdata[34], NULL, NULL, 4},        //34
                {&g_sdata[35], NULL, NULL, 4},        //35
                {&g_sdata[36], NULL, NULL, 0},        //36
                {&g_sdata[37], NULL, NULL, 4},        //37
                {&g_sdata[38], NULL, NULL, 65},        //38
                {&g_sdata[39], NULL, NULL, 98},        //39
                {&g_sdata[40], NULL, NULL, 21},        //40
                {&g_sdata[41], NULL, NULL, 21},        //41
                {&g_sdata[42], NULL, NULL, 34},        //42
                {&g_sdata[43], NULL, NULL, 33},        //43
                {&g_sdata[44], NULL, NULL, 4},        //44
                {&g_sdata[45], NULL, NULL, 0},        //45
                {&g_sdata[46], NULL, NULL, 10},        //46
                {&g_sdata[47], NULL, NULL, 15},        //47
                {&g_sdata[48], NULL, NULL, 7},        //48
                {&g_sdata[49], NULL, NULL, 7},        //49
                {&g_sdata[50], NULL, NULL, 4},        //50
                {&g_sdata[51], NULL, NULL, 4},        //51
                {&g_sdata[52], NULL, NULL, 4},        //52
                {&g_sdata[53], NULL, NULL, 0},        //53
                {&g_sdata[54], NULL, NULL, 0},        //54
                {&g_sdata[55], NULL, NULL, 4},        //55
                {&g_sdata[56], NULL, NULL, 15},        //56
                {&g_sdata[57], NULL, NULL, 0},        //57
                {&g_sdata[58], NULL, NULL, 4},        //58
                {&g_sdata[59], NULL, NULL, 15},        //59
                {&g_sdata[60], NULL, NULL, 0},        //60
                {&g_sdata[61], NULL, NULL, 0},        //61
                {&g_sdata[62], NULL, NULL, 4},        //62
                {&g_sdata[63], NULL, NULL, 4},        //63
                {&g_sdata[64], NULL, NULL, 4},        //64
                {&g_sdata[65], NULL, NULL, 4},        //65
                {&g_sdata[66], NULL, NULL, 4},        //66
                {&g_sdata[67], NULL, NULL, 0},        //67
                {&g_sdata[68], NULL, NULL, 4},        //68
                {&g_sdata[69], NULL, NULL, 0},        //69
                {&g_sdata[70], NULL, NULL, 4},        //70
                {&g_sdata[71], NULL, NULL, 0},        //71
                {&g_sdata[72], NULL, NULL, 4},        //72
                {&g_sdata[73], NULL, NULL, 13},        //73
                {&g_sdata[74], NULL, NULL, 8},        //74
                {&g_sdata[75], NULL, NULL, 8},        //75
                {&g_sdata[76], NULL, NULL, 5},        //76
                {&g_sdata[77], NULL, NULL, 8},        //77
                {&g_sdata[78], NULL, NULL, 4},        //78

		/* end */
};
#endif /* (CONFIG_APP_SETTINGS_DISPVAL_LOAD_TO_RAM) */
