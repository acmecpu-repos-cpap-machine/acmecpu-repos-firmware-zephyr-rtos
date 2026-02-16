/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_BATTERY_BSP_BATTERY_H_
#define SRC_INCLUDE_APP_BATTERY_BSP_BATTERY_H_

#include <stdint.h>
#if (CONFIG_BQ25611D)
#include "bq25611d.h"
#endif

#define BSP_BATTERY_VBAT_MIN_UV		(3000000)	/* minimum battery voltage in uV */
#define BSP_BATTERY_VBUS_MIN_UV		(3600000)	/* minimum battery voltage in uV */

#define BSP_BATTERY_CHARGE_CURR_LIMIT_50		50			/* charge current limit = 50 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_100		100			/* charge current limit = 100 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_250		250			/* charge current limit = 250 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_500		500			/* charge current limit = 500 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_1000		1000		/* charge current limit = 1000 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_1500		1500		/* charge current limit = 1500 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_2000		2000		/* charge current limit = 2000 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_3000		3000		/* charge current limit = 3000 MA */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_5000		5000		/* charge current limit = 5000 MA */

/**
 * Battery charger current limit settings for driver
 * */
#if (CONFIG_BQ25611D)
#if (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_50)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_60MA)				/* assigning the nearest value of 60MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_100)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_120MA)				/* assigning the nearest value of 120MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_250)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_240MA)				/* assigning the nearest value of 240MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_500)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_480MA)				/* assigning the nearest value of 480MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_1000)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_960MA | BQ25611D_CHARGE_CURRENT_LIMIT_60MA)		/* assigning the nearest value of 1020MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_2000)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_1920MA | BQ25611D_CHARGE_CURRENT_LIMIT_120MA)	/* assigning the nearest value of 2040MA which is supported by the driver */

#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_3000)
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_1920MA | BQ25611D_CHARGE_CURRENT_LIMIT_960MA | BQ25611D_CHARGE_CURRENT_LIMIT_120MA)

//#elif (CONFIG_APP_BATTERY_CHARGE_CURRENT_LIMIT == BSP_BATTERY_CHARGE_CURR_LIMIT_5000)
#else
/* assigning the maximum value supported by the driver */
#define BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS	(BQ25611D_CHARGE_CURRENT_LIMIT_1920MA | BQ25611D_CHARGE_CURRENT_LIMIT_960MA | BQ25611D_CHARGE_CURRENT_LIMIT_120MA)
#endif
#endif /* CONFIG_BQ25611D */

typedef enum {
	BSP_BATT_UNKNOWN = 0,
	BSP_BATT_CHARGING,
	BSP_BATT_DISCHARGING,
	BSP_BATT_NOT_CHARGING,
	BSP_BATT_FULL,
} BSP_BATTERY_CHARGING_STATUS;

typedef enum {
	BSP_BATT_CHRG_EVENT_NONE = 0,
	BSP_BATT_CHRG_EVENT_ADAPTER_ATTACHED,
	BSP_BATT_CHRG_EVENT_ADAPTER_REMOVED,
	BSP_BATT_CHRG_EVENT_CHARGE_COMPLETE,
	BSP_BATT_CHRG_EVENT_FAULT,
} BSP_BATTERY_CHARGER_EVENTS;

typedef enum {
	BSP_BATT_CHARGING_DISABLE = 0,
	BSP_BATT_CHARGING_ENABLE
} BSP_BATTERY_CHARGING_CONTROL;

typedef enum {
	BSP_BATT_DISCONNECTED = 0,
	BSP_BATT_CONNECTED
} BSP_BATTERY_CONNECTION_STATE;

typedef enum {
	BSP_BATT_IBAT_DIS_SENSE_DISABLE = 0,
	BSP_BATT_IBAT_DIS_SENSE_ENABLE
} BSP_BATTERY_IBAT_DISCHARGE_SENSING_CONTROL;


typedef void (*bsp_battery_cb_handler_t)(BSP_BATTERY_CHARGER_EVENTS event);

int bsp_battery_register_cb(bsp_battery_cb_handler_t handler);

int bsp_battery_charging_status_get(uint8_t *charging_status, uint32_t *charge_curr_ma);

int bsp_battery_schedule_system_off(uint32_t *ms_to_off);

int bsp_battery_mvolts_get(uint32_t *batt_mvolts);

int bsp_battery_available_capacity_get(float *batt_capacity);
int bsp_battery_capacity_value_set(float batt_capacity_mah);

int bsp_battery_charging_control(BSP_BATTERY_CHARGING_CONTROL en_dis);
int bsp_battery_vbus_get(int32_t *mv);
int bsp_battery_ibus_get(int32_t *ma);
int bsp_battery_vbat_get(int32_t *mv);
int bsp_battery_ibat_discharge_sensing_control(BSP_BATTERY_IBAT_DISCHARGE_SENSING_CONTROL en_dis);
int bsp_battery_ibat_get(int32_t *ma);
int bsp_battery_vchrg_get(int32_t *mv);
int bsp_battery_ichrg_get(int32_t *ma);
int bsp_battery_ichrg_set(int32_t ma);

int bsp_battery_ucpd_init();

int bsp_battery_init();

#endif /* SRC_INCLUDE_APP_BATTERY_BSP_BATTERY_H_ */
