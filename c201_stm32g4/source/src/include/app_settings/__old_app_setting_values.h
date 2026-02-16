/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_SETTINGS_APP_SETTING_VALUES_H_
#define SRC_INCLUDE_APP_SETTINGS_APP_SETTING_VALUES_H_

#include <zephyr.h>

#define SETTINGS_PKG_NAME_LEN_MAX		15
#define SETTINGS_FULLPATH_LEN_MAX		64
#define SETTINGS_STR_VAL_LEN_MAX		32

/********************************************
 * 				Values for root				*
 ********************************************/
#define SETTINGS_KEY_ROOT					"root"

/********************************************************************
 * 			Key and Value for settings load							*
 ********************************************************************/
#define SETTINGS_KEY_LOAD					"load"
#define SETTINGS_KEY_FULL_LOAD				SETTINGS_KEY_ROOT"/load"
/* load_set */
#define SETTINGS_KEY_LOAD_SET				"load/load_set"
#define SETTINGS_KEY_FULL_LOAD_SET			SETTINGS_KEY_ROOT"/load/load_set"
#define SETTINGS_LOAD_TRUE					(1)	/* value to represent that settings are available */

/********************************************************************
 * 			Values for settings_logging members						*
 ********************************************************************/
#define SETTINGS_KEY_LOG					"log"
#define SETTINGS_KEY_FULL_LOG				SETTINGS_KEY_ROOT"/log"

/* log_store */
#define SETTINGS_KEY_LOG_STORE				"log_store"
#define SETTINGS_KEY_FULL_LOG_STORE			SETTINGS_KEY_ROOT"/log/log_store"
#define SETTINGS_LOG_STORE_NONE				(0)	/* do not store any system log */
#define SETTINGS_LOG_STORE					(1)	/* store system logs in a storage device selected by KConfig */

#if (SETTINGS_NEEDED)
/* log_tx_dur */
#define SETTINGS_KEY_LOG_TX_DUR				"log_tx_dur"
#define SETTINGS_KEY_FULL_LOG_TX_DUR		SETTINGS_KEY_ROOT"/log/log_tx_dur"
#define SETTINGS_LOG_TX_DUR_NONE			(0)		/* do not transmit the system log */
#define SETTINGS_LOG_TX_DUR_HR				(24)	/* transmit the system log every 24 hour or any other value */

/* log_tx_med */
#define SETTINGS_KEY_LOG_TX_MED				"log_tx_med"
#define SETTINGS_KEY_FULL_LOG_TX_MED		SETTINGS_KEY_ROOT"/log/log_tx_med"
#define SETTINGS_LOG_TX_MED_NONE			(1)	/* do not transmit the system log */
#define SETTINGS_LOG_TX_MED_WIFI			(2)	/* transmit the system log via wifi only */
#define SETTINGS_LOG_TX_MED_WIFI_4G			(3)	/* transmit the system log via wifi and 4G */
#endif

/* data_store */
#define SETTINGS_KEY_DATA_STORE				"data_store"
#define SETTINGS_KEY_FULL_DATA_STORE		SETTINGS_KEY_ROOT"/log/data_store"
#define SETTINGS_DATA_STORE_NONE			(0)	/* do not store any application data */
#define SETTINGS_DATA_STORE					(1)	/* store application data in a storage device selected by KConfig */

#if (SETTINGS_NEEDED)
/* data_tx_dur */
#define SETTINGS_KEY_DATA_TX_DUR			"data_tx_dur"
#define SETTINGS_KEY_FULL_DATA_TX_DUR		SETTINGS_KEY_ROOT"/log/data_tx_dur"
#define SETTINGS_DATA_TX_DUR_NONE			(0)		/* do not transmit the application data */
#define SETTINGS_DATA_TX_DUR_HR				(24)	/* transmit the application data every 24 hour or any other value */

/* data_tx_med */
#define SETTINGS_KEY_DATA_TX_MED			"data_tx_med"
#define SETTINGS_KEY_FULL_DATA_TX_MED		SETTINGS_KEY_ROOT"/log/data_tx_med"
#define SETTINGS_DATA_TX_MED_NONE			(1)	/* do not transmit the application data */
#define SETTINGS_DATA_TX_MED_WIFI			(2)	/* transmit the application data via wifi only */
#define SETTINGS_DATA_TX_MED_WIFI_4G		(3)	/* transmit the application data via wifi and 4G */
#define SETTINGS_DATA_TX_MED_BT				(4)	/* transmit the application data via Bluetooth only */
#endif

/********************************************************************
 * 			Values for settings_power_and_charging members			*
 ********************************************************************/
#define SETTINGS_KEY_POWER_CHRG				"pwr_chrg"
#define SETTINGS_KEY_FULL_POWER_CHRG		SETTINGS_KEY_ROOT"/pwr_chrg"

/* powered_on */
#define SETTINGS_KEY_POWERED_ON				"powered_on"
#define SETTINGS_KEY_FULL_POWERED_ON		SETTINGS_KEY_ROOT"/pwr_chrg/powered_on"
#define SETTINGS_POWERED_ON_RESUME			(1)	/* when device is powered on, resume the last state */
#define SETTINGS_POWERED_ON_STAY_IDLE		(2) /* when device is powered on, do not resume any states, stay idle until and user event occurs */
/* powered_off */
#define SETTINGS_KEY_POWERED_OFF			"powered_off"
#define SETTINGS_KEY_FULL_POWERED_OFF		SETTINGS_KEY_ROOT"/pwr_chrg/powered_off"
#define SETTINGS_POWERED_OFF_SHUTDOWN		(1) /* when user tries to power off the device, shutdown the device immediately */
#define SETTINGS_POWERED_OFF_SHOW_OPTIONS	(2)	/* when user tries to power off the device, show options like shutdown, reboot ... */
/* chrg_dev_on */
#define SETTINGS_KEY_CHRG_DEV_ON			"chrg_dev_on"
#define SETTINGS_KEY_FULL_CHRG_DEV_ON		SETTINGS_KEY_ROOT"/pwr_chrg/chrg_dev_on"
#define SETTINGS_CHRG_DEV_ON_SCREEN_ON		(1)	/* if charger is attached when the device is on, turn on the screen */
#define SETTINGS_CHRG_DEV_ON_LED_ON			(2) /* if charger is attached when the device is on, turn on the charging led */
#define SETTINGS_CHRG_DEV_ON_SCREEN_LED_ON	(3) /* if charger is attached when the device is on, turn on the screen and charging led */
#define SETTINGS_CHRG_DEV_ON_NOOP			(4) /* if charger is attached when the device is on, no operation */
/* chrg_dev_off */
#define SETTINGS_KEY_CHRG_DEV_OFF			"chrg_dev_off"
#define SETTINGS_KEY_FULL_CHRG_DEV_OFF		SETTINGS_KEY_ROOT"/pwr_chrg/chrg_dev_off"
#define SETTINGS_CHRG_DEV_OFF_DEV_ON		(1)	/* if charger is attached when the device is off, turn the device on */
#define SETTINGS_CHRG_DEV_OFF_DEV_SCREEN_ON	(2) /* if charger is attached when the device is on, turn the device on and screen on */
#define SETTINGS_CHRG_DEV_OFF_DEV_LED_ON	(3) /* if charger is attached when the device is on, turn the device on and led on */
#define SETTINGS_CHRG_DEV_OFF_DEV_SCREEN_LED_ON	(4) /* if charger is attached when the device is on, turn the device on and screen & led on */
#define SETTINGS_CHRG_DEV_OFF_NOOP			(5) /* if charger is attached when the device is off, no operation */

#if (SETTINGS_NEEDED)
/********************************************************************
 * 				Values for settings_prescription members			*
 ********************************************************************/
#define SETTINGS_KEY_PRESC					"presc"
#define SETTINGS_KEY_FULL_PRESC				SETTINGS_KEY_ROOT"/presc"

/* presc_change */
#define SETTINGS_KEY_PRESC_CHANGE			"presc/presc_change"
#define SETTINGS_KEY_FULL_PRESC_CHANGE		SETTINGS_KEY_ROOT"/presc/presc_change"
#define SETTINGS_PRESC_CHANGE_UPDATE_NO_CONF (1)	/* on a prescription change request, update without any user confirmation */
#define SETTINGS_PRESC_CHANGE_UPDATE_CONF 	(2)	/* on a prescription change request, update with any user confirmation */
#define SETTINGS_PRESC_CHANGE_UPDATE_NO_CONF_24HRS	(3)	/* on a prescription change request, update without any user confirmation for next 24 hours */
#define SETTINGS_PRESC_CHANGE_UPDATE_NO_CONF_1WK	(4)	/* on a prescription change request, update without any user confirmation for next 1 week */
#endif

/********************************************************************
 * 				Values for settings_blower members					*
 ********************************************************************/
#define SETTINGS_KEY_BLOWER					"blower"
#define SETTINGS_KEY_FULL_BLOWER			SETTINGS_KEY_ROOT"/blower"

#if (SETTINGS_NEEDED)
/* voltage_mv */
#define SETTINGS_KEY_BLOWER_VOLTAGE			"blower/volt_mv"
#define SETTINGS_KEY_FULL_BLOWER_VOLTAGE	SETTINGS_KEY_ROOT"/blower/volt_mv"
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
#define SETTINGS_BLOWER_VOLTAGE_MV			(11000)	/* desired blower voltage in mili-volts, to be updated every time the it changes */
#else
#define SETTINGS_BLOWER_VOLTAGE_MV			(12000)	/* desired blower voltage in mili-volts, to be updated every time the it changes */
#endif
/* speed_rpm */
#define SETTINGS_KEY_BLOWER_SPEED			"blower/speed_rpm"
#define SETTINGS_KEY_FULL_BLOWER_SPEED		SETTINGS_KEY_ROOT"/blower/speed_rpm"
#define SETTINGS_BLOWER_SPEED_RPM			(10000)	/* desired blower RPM, to be updated every time the it changes */

/* duty_percent */
#define SETTINGS_KEY_BLOWER_DUTY			"blower/duty"
#define SETTINGS_KEY_FULL_BLOWER_DUTY		SETTINGS_KEY_ROOT"/blower/duty"
#define SETTINGS_BLOWER_DUTY				(100)	/* desired duty cycle in percentage, to be updated every time the it changes */
#endif

/* pressure */
#define SETTINGS_KEY_BLOWER_PRESS_PA			"blower/ppa"
#define SETTINGS_KEY_FULL_BLOWER_PRESS_PA		SETTINGS_KEY_ROOT"/blower/ppa"
#define SETTINGS_BLOWER_PRESS_PA				(200)	/* blower pressure in Pa */

/* state */
#define SETTINGS_KEY_BLOWER_STATE			"blower/state"
#define SETTINGS_KEY_FULL_BLOWER_STATE		SETTINGS_KEY_ROOT"/blower/state"
#define SETTINGS_BLOWER_STATE				(0)		/* whether the blower is on=1 or off=0, to be updated every time the it changes */

#if (SETTINGS_NEEDED)
/********************************************************************
 * 				Values for settings_stepper members					*
 ********************************************************************/
#define SETTINGS_KEY_STEPPER					"stepper"
#define SETTINGS_KEY_FULL_STEPPER				SETTINGS_KEY_ROOT"/stepper"

/* reset_pos */
#define SETTINGS_KEY_STEPPER_RESET_POS			"stepper/reset_pos"
#define SETTINGS_KEY_FULL_STEPPER_RESET_POS		SETTINGS_KEY_ROOT"/stepper/reset_pos"
#define SETTINGS_RESET_POS						(CONFIG_STEPPER_RESET_POSITION)	/* the reset position (in degrees) of the stepper motor after system power up */

/* step_speed_hz */
#define SETTINGS_KEY_STEPPER_STEP_SPEED_HZ		"stepper/step_speed_hz"
#define SETTINGS_KEY_FULL_STEPPER_STEP_SPEED_HZ		SETTINGS_KEY_ROOT"/stepper/step_speed_hz"
#define SETTINGS_STEP_SPEED_HZ					(CONFIG_STEPPER_STEP_SPEED_HZ)	/* step speed in hertz */

/* reset_rot_cnt */
#define SETTINGS_KEY_STEPPER_RESET_ROT_CNT		"stepper/reset_rot_cnt"
#define SETTINGS_KEY_FULL_STEPPER_RESET_ROT_CNT		SETTINGS_KEY_ROOT"/stepper/reset_rot_cnt"
#define SETTINGS_RESET_ROT_CNT					(CONFIG_STEPPER_NUM_ROTATION_TO_RESET)	/* The number of complete rotation the stepper should make in order to reach the reset position */

/* reset_dir */
#define SETTINGS_KEY_STEPPER_RESET_DIR			"stepper/reset_dir"
#define SETTINGS_KEY_FULL_STEPPER_RESET_DIR		SETTINGS_KEY_ROOT"/stepper/reset_dir"
#define SETTINGS_RESET_DIR						(CONFIG_STEPPER_RESET_DIRECTION)	/* The direction at which the stepper should rotate to reach reset position */

/* step_mode */
#define SETTINGS_KEY_STEPPER_STEP_MODE			"stepper/step_mode"
#define SETTINGS_KEY_FULL_STEPPER_STEP_MODE		SETTINGS_KEY_ROOT"/stepper/step_mode"
#define SETTINGS_STEP_MODE						(CONFIG_STEPPER_STEP_MODE)	/* Step mode at which the driver should be configured */

/* step_angle */
#define SETTINGS_KEY_STEPPER_STEP_ANGLE			"stepper/step_angle"
#define SETTINGS_KEY_FULL_STEPPER_STEP_ANGLE	SETTINGS_KEY_ROOT"/stepper/step_angle"
#define SETTINGS_STEP_ANGLE						(CONFIG_FULL_STEP_ANGLE)	/* One full step angle in degrees of the motor used in the application */
#endif

/********************************************************************
 * 				Key and Value for personalization settings			*
 ********************************************************************/
#define SETTINGS_KEY_PERSONALIZATION			"pers"
#define SETTINGS_KEY_FULL_PERSONALIZATION		SETTINGS_KEY_ROOT"/"SETTINGS_KEY_PERSONALIZATION

/* screen brightness */
#define SETTINGS_KEY_PERS_BRIGHT				SETTINGS_KEY_PERSONALIZATION"/brgt"
#define SETTINGS_KEY_FULL_PERS_BRIGHT			SETTINGS_KEY_ROOT"/"SETTINGS_KEY_PERS_BRIGHT
#if CONFIG_APP_DISPLAY
#define SETTINGS_PERS_BRIGHT_DEF				(CONFIG_APP_DISPLAY_BRIGHTNESS_LEVEL)	/* default screen brightness */
#else
#define SETTINGS_PERS_BRIGHT_DEF				(100)	/* default screen brightness */
#endif

/* screen on time */
#define SETTINGS_KEY_PERS_SCREENON_TM			SETTINGS_KEY_PERSONALIZATION"/scn"
#define SETTINGS_KEY_FULL_PERS_SCREENON_TM		SETTINGS_KEY_ROOT"/"SETTINGS_KEY_PERS_SCREENON_TM
#if CONFIG_APP_DISPLAY
#define SETTINGS_PERS_SCREENON_TM_DEF			(CONFIG_APP_DISPLAY_SLEEP_TM)	/* default time to screen off when inactive */
#else
#define SETTINGS_PERS_SCREENON_TM_DEF			(100)	/* default time to screen off when inactive */
#endif

/* keypad / tap sound */
#define SETTINGS_KEY_PERS_KPSOUND				SETTINGS_KEY_PERSONALIZATION"/kps"
#define SETTINGS_KEY_FULL_PERS_KPSOUND			SETTINGS_KEY_ROOT"/"SETTINGS_KEY_PERS_KPSOUND
#define SETTINGS_PERS_SCREENON_KPSOUND_DEF		(1)	/* keypad sound default ON */

/********************************************************************
 * 				Key and Value for date time settings			*
 ********************************************************************/
#define SETTINGS_KEY_DATETIME					"dt"
#define SETTINGS_KEY_FULL_DATETIME				SETTINGS_KEY_ROOT"/"SETTINGS_KEY_DATETIME

/* date */
#define SETTINGS_KEY_DT_DATE				SETTINGS_KEY_DATETIME"/date"
#define SETTINGS_KEY_FULL_DT_DATE			SETTINGS_KEY_ROOT"/"SETTINGS_KEY_DT_DATE

/* time - dummy, used for display purpose only */
#define SETTINGS_KEY_DT_TIME				SETTINGS_KEY_DATETIME"/time"
#define SETTINGS_KEY_FULL_DT_TIME			SETTINGS_KEY_ROOT"/"SETTINGS_KEY_DT_TIME

#define SETTINGS_DT_UNIXTIME_DEF				(1675334363) // unix time when this program was written

#endif /* SRC_INCLUDE_APP_SETTINGS_APP_SETTING_VALUES_H_ */
