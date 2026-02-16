/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_BQ25611D_BQ25611D_H_
#define MODULES_BQ25611D_BQ25611D_H_

//#ifdef __cplusplus
//extern "C" {
//#endif
// #include <zephyr.h>
#include <stdint.h>
#include <zephyr/device.h>

#if CONFIG_BQ25611D_HAS_ANALOG
#define BQ25611D_ADC_REF_VOLTAGE				(3.3)

/* VBAT Divider Resistor Values
 * R1 = 27k, R2 = 75k */
#define BQ25611D_VBAT_ADC_VOLT_DIVIDER_R1		(27*1000)
#define BQ25611D_VBAT_ADC_VOLT_DIVIDER_R2		(75*1000)

/* VBUS Divider Resistor Values
 * R1 = 390k, R2 = 75k */
#define BQ25611D_VBUS_ADC_VOLT_DIVIDER_R1		(390*1000)
#define BQ25611D_VBUS_ADC_VOLT_DIVIDER_R2		(75*1000)
#endif	/*CONFIG_BQ25611D_HAS_ANALOG*/


/*
 * BQ25611D CONSTANTS
 * */
/* QON Timing */
#define BQ25611D_TSHIPMODE_MS_MIN				(1*1000)	/* Datasheet Section 8.6 Timing Requirements tSHIPMODE */
#define BQ25611D_TQON_RST_MS_MIN				(8*1000)	/* Datasheet Section 8.6 Timing Requirements tQON_RST */
#define BQ25611D_TBATFET_RST_MS_MIN				(250)		/* Datasheet Section 8.6 Timing Requirements tBATFET_RST */
#define BQ25611D_TBATFET_DLY_MS_MIN				(10*1000)	/* Datasheet Section 8.6 Timing Requirements tBATFET_DLY */

/* Charge current limit values
 * These values are set to the Current charge limit register, REG02
 * The values can be OR'ed together to set different charge limits
 * other than those defined below
 * */
#define BQ25611D_CHARGE_CURRENT_LIMIT_60MA		0x01	/* 00 000001 */
#define BQ25611D_CHARGE_CURRENT_LIMIT_120MA		0x02	/* 00 000010 */
#define BQ25611D_CHARGE_CURRENT_LIMIT_240MA		0x04	/* 00 000100 */
#define BQ25611D_CHARGE_CURRENT_LIMIT_480MA		0x08	/* 00 001000 */
#define BQ25611D_CHARGE_CURRENT_LIMIT_960MA		0x10	/* 00 010000 */
#define BQ25611D_CHARGE_CURRENT_LIMIT_1920MA	0x20	/* 00 100000 */

/* Input current limit values
 * These values are set to the Input Current limit register, REG00
 * The values can be OR'ed together to set different charge limits
 * other than those defined below
 * */
#define BQ25611D_INPUT_CURRENT_LIMIT_100MA		0x01	/* 00 000001 */
#define BQ25611D_INPUT_CURRENT_LIMIT_200MA		0x02	/* 00 000010 */
#define BQ25611D_INPUT_CURRENT_LIMIT_400MA		0x04	/* 00 000100 */
#define BQ25611D_INPUT_CURRENT_LIMIT_800MA		0x08	/* 00 001000 */
#define BQ25611D_INPUT_CURRENT_LIMIT_1600MA		0x10	/* 00 010000 */

/* BQ25611D events on INT pin. Datasheet secion 9.3.8.2 Interrupt to Host */
typedef enum {
	BQ25611D_EVENT_GOOD_SOURCE_DETECTED=0,		/* V(VBUS) above battery, V(VBUS) below V(ACOV), V(VBUS) above V(POORSRC) typically 3.8V*/
	BQ25611D_EVENT_ADAPTER_REMOVED,				/* when input adapter is removed */
	BQ25611D_EVENT_ADAPTER_SOURCE_IDENTIFIED,	/* during Input Source Type Detection, see datasheet*/
	BQ25611D_EVENT_CHARGE_COMPLETE,				/* */
	BQ25611D_EVENT_FAULT,						/* Any fault event in REG09*/
	BQ25611D_EVENT_VINDPM_IINDPM_DETECTED,		/* VINDPM / IINDPM event detected (REG0A[1:0] maskable) */
	BQ25611D_EVENT_TOP_OFF_TIMER				/* top off timer starts and expires */
} BQ25611D_EVENTS;

/*
 * BQ25611D STATUSES
 * */
/* status register 0 */
#define BQ25611D_STATUS_VBUS_1500MA							(2)
#define BQ25611D_STATUS_VBUS_2400MA							(3)
#define BQ25611D_STATUS_VBUS_1000_TO_2100MA					(6)
#define BQ25611D_STATUS_VBUS_500MA							(5)
#define BQ25611D_STATUS_NOT_CHARGING						(0)
#define BQ25611D_STATUS_PRE_CHARGING						(1)
#define BQ25611D_STATUS_FAST_CHARGING						(2)
#define BQ25611D_STATUS_CHARGE_TERMINATION					(3)
#define BQ25611D_STATUS_NOT_IN_THERMAL_REG					(0)
#define BQ25611D_STATUS_IN_THERMAL_REG						(1)
#define BQ25611D_STATUS_NOT_IN_SYSMIN_REG					(0)
#define BQ25611D_STATUS_IN_SYSMIN_REG						(1)

/* status register 1 */
#define BQ25611D_STATUS_WATCHDOG_FAULT_NORMAL				(0)
#define BQ25611D_STATUS_WATCHDOG_FAULT_TIM_EXP				(1)
#define BQ25611D_STATUS_BOOST_FAULT_NORMAL					(0)
#define BQ25611D_STATUS_BOOST_FAULT_DETECTED				(1)
#define BQ25611D_STATUS_CHARGE_FAULT_NORMAL					(0)
#define BQ25611D_STATUS_CHARGE_INPUT_FAULT					(1)
#define BQ25611D_STATUS_CHARGE_FAULT_THERMAL_SHUTDOWN		(2)
#define BQ25611D_STATUS_CHARGE_FAULT_SAFETIM_EXPIRY			(3)
#define BQ25611D_STATUS_BATT_FAULT_NORMAL					(0)
#define BQ25611D_STATUS_BATT_FAULT_OVERVOLTAGE				(1)
#define BQ25611D_STATUS_NTC_FAULT_NORMAL					(0)
#define BQ25611D_STATUS_NTC_FAULT_WARM						(2)
#define BQ25611D_STATUS_NTC_FAULT_COOL						(3)
#define BQ25611D_STATUS_NTC_FAULT_COLD						(5)
#define BQ25611D_STATUS_NTC_FAULT_HOT						(6)

/* status register 2 */
#define BQ25611D_STATUS_VBUS_NOT_GOOD						(0)
#define BQ25611D_STATUS_VBUS_GOOD							(1)

#define BQ25611D_STATUS_NOT_IN_VINDPM						(0)
#define BQ25611D_STATUS_IN_VINDPM							(1)
#define BQ25611D_STATUS_NOT_IN_IINDPM						(0)
#define BQ25611D_STATUS_IN_IINDPM							(1)
#define BQ25611D_STATUS_BATSNS_PIN_GOOD						(0)
#define BQ25611D_STATUS_BATSNS_PIN_OPEN						(1)
#define BQ25611D_STATUS_TOPOFF_NOT_COUNTING					(0)
#define BQ25611D_STATUS_TOPOFF_COUNTING						(1)
#define BQ25611D_STATUS_NOT_IN_ACOV							(0)
#define BQ25611D_STATUS_IN_ACOV								(1)
#define BQ25611D_STATUS_VINDPM_INT							(0)
#define BQ25611D_STATUS_VINDPM_NO_INT						(1)
#define BQ25611D_STATUS_IINDPM_INT							(0)
#define BQ25611D_STATUS_IINDPM_NO_INT						(1)


/* BQ25611D Status structure */
typedef struct {
	struct {
		uint8_t vsys_stat : 1;
		uint8_t therm_stat : 1;
		uint8_t reserved : 1;
		uint8_t chrg_stat : 2;
		uint8_t vbus_stat : 3;
	} chrg_status0;

	struct {
		uint8_t ntc_fault : 3;
		uint8_t bat_fault : 1;
		uint8_t chrg_fault : 2;
		uint8_t boost_fault : 1;
		uint8_t watchdog_fault : 1;
	} chrg_status1;

	struct {
		uint8_t iindpm_int_mask : 1;
		uint8_t vindpm_int_mask : 1;
		uint8_t acov_stat : 1;
		uint8_t topoff_active : 1;
		uint8_t batsns_stat : 1;
		uint8_t iindpm_stat : 1;
		uint8_t vindpm_stat : 1;
		uint8_t vbus_gd : 1;
	} chrg_status2;
} bq25611d_status_t;

typedef void (*bq25611d_intr_handler_t)(const struct device *dev, bq25611d_status_t *chrg_stat);

/* API type defines */
typedef int (*bq25611d_status_get_t)(const struct device *, bq25611d_status_t*);
typedef int (*bq25611d_enter_ship_mode_t)(const struct device *, bool, uint32_t*);
typedef int (*bq25611d_exit_ship_mode_t)(const struct device *);
typedef int (*bq25611d_full_system_reset_t)(const struct device *);
typedef int (*bq25611d_chrg_curr_lim_set_t)(const struct device *, uint8_t);
typedef int (*bq25611d_chrg_curr_setting_get_t)(const struct device *, uint32_t*);
typedef int (*bq25611d_in_curr_setting_set_t)(const struct device *, uint32_t);
typedef int (*bq25611d_in_curr_setting_get_t)(const struct device *, uint32_t*);
typedef int (*bq25611d_intr_handler_set_t)(const struct device *, bq25611d_intr_handler_t);
#if CONFIG_BQ25611D_HAS_ANALOG
typedef int (*bq25611d_batt_mvolts_get_t)(const struct device *, uint32_t*);
typedef int (*bq25611d_vbus_mvolts_get_t)(const struct device *, uint32_t*);
#endif

struct bq25611d_driver_api {
	bq25611d_status_get_t status_get;
	bq25611d_enter_ship_mode_t enter_ship_mode;
	bq25611d_exit_ship_mode_t exit_ship_mode;
	bq25611d_full_system_reset_t full_system_reset;
	bq25611d_chrg_curr_lim_set_t chrg_curr_lim_set;
	bq25611d_chrg_curr_setting_get_t chrg_curr_setting_get;
	bq25611d_in_curr_setting_set_t in_curr_setting_set;
	bq25611d_in_curr_setting_get_t in_curr_setting_get;
	bq25611d_intr_handler_set_t intr_handler_set;
#if CONFIG_BQ25611D_HAS_ANALOG
	bq25611d_batt_mvolts_get_t batt_mvolts_get;
	bq25611d_vbus_mvolts_get_t vbus_mvolts_get;
#endif
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_BQ25611D_BQ25611D_H_ */
