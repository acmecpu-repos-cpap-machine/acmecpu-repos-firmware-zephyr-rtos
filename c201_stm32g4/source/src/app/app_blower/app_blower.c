/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 11-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_blower);

#include <zephyr/fs/fs.h>

#include "app_thread_configs.h"
#include "app_data_recorder.h"
#include "app_blower/app_blower.h"
#include "bsp_blower/bsp_blower.h"
#include "lib_events/lib_events.h"
#include "app_sensor/app_sensor.h"

#include "app_settings/app_settings.h"
#include "app_settings/app_settings_value.h"

#if (CONFIG_APP_DISPLAY)
	#include "c20x_screen_dashboard.h"
#endif
#include "lib_pid/PID.h"

#if CONFIG_LIB_FILE_OPER
#include "lib_file_oper/lib_file_oper.h"
#endif

/* constants */
#if (CONFIG_BLOWER_MOTOR_A101)
	#define BLOWER_MVOLTS_MIN 	CONFIG_BLOWER_MOTOR_A101_MVOLTS_MIN
	#define BLOWER_MVOLTS_MAX 	CONFIG_BLOWER_MOTOR_A101_MVOLTS_MAX
#elif (CONFIG_BLOWER_MOTOR_A102)
	#define BLOWER_MVOLTS_MIN 	CONFIG_BLOWER_MOTOR_A102_MVOLTS_MIN
	#define BLOWER_MVOLTS_MAX 	CONFIG_BLOWER_MOTOR_A102_MVOLTS_MAX
#elif (CONFIG_BLOWER_MOTOR_A2)
	#define BLOWER_MVOLTS_MIN 	CONFIG_BLOWER_MOTOR_A2_MVOLTS_MIN
	#define BLOWER_MVOLTS_MAX 	CONFIG_BLOWER_MOTOR_A2_MVOLTS_MAX
#endif	/* (CONFIG_BLOWER_MOTOR_A101) */

#if (CONFIG_BLOWER_MOTOR_A101)
	#define SPEED_MIN	CONFIG_BLOWER_MOTOR_A101_SPEED_RPM_MIN
	#define SPEED_MAX	CONFIG_BLOWER_MOTOR_A101_SPEED_RPM_MAX
#elif (CONFIG_BLOWER_MOTOR_A102)
	#define SPEED_MIN	CONFIG_BLOWER_MOTOR_A102_SPEED_RPM_MIN
	#define SPEED_MAX	CONFIG_BLOWER_MOTOR_A102_SPEED_RPM_MAX
#endif	/* (CONFIG_BLOWER_MOTOR_A101) */

#define BLOWER_PARAMS_UPDATE(mv, hz, rpm, flt, kpa, cp, fl) \
	m_blower_params.acq_volt_mv = mv; \
	m_blower_params.acq_speed_hz = hz; \
	m_blower_params.acq_speed_rpm = rpm; \
	m_blower_params.faults = flt; \
	m_blower_params.acq_pressure_kpa = kpa; \
	m_blower_params.controlled_press_kpa = cp; \
	m_blower_params.acq_air_flow = fl;

/* static and global variables */
static struct k_sem m_blw_settings_lock;
static struct k_sem m_blw_params_lock;
static bool m_blower_settings_changed = false;
static struct app_blower_params m_blower_params;

static struct lib_events_callback m_cb_poweroff;
static struct lib_events_callback m_cb_settings_changed;

/*static global variable for printing the data*/
int g_press_csv = 0;

/* pid control static variables */
static bool m_pid_blower_start = false;
static float m_pid_sv_Pkpa = 0.2f;
static uint32_t m_pid_act_change_const = 250;
static uint8_t m_pid_blower_state = BLOWER_NOT_RUNNING;

/* log file static variables */
static int m_sensor_log_file_handle = -1;

/* thread static variables */
K_THREAD_STACK_DEFINE(m_pid_stack, APP_THREAD_STACK_SIZE_PID);
static struct k_thread m_pid_data;
static k_tid_t m_pid_tid;

/* other static variables */
static uint8_t m_blower_oper_mode = APP_BLOWER_OPER_MODE_PID;
static uint32_t m_blower_speed_ramp_ms = 1000;
static uint32_t m_blower_test_mode_rpm = 10000;
static APP_BLOWER_SETTINGS_STATES m_blw_setting_state = APP_BLOWER_SETTINGS_READY;

/*static global variable to store the sensor id*/
int32_t g_ambi_press_id = 0;
int32_t g_inhale_wall_press_id = 0;
int32_t g_inhale_pitot_tube_press_id = 0;
int32_t g_exhale_wall_press_id = 0;
int32_t g_exhale_pitot_tube_press_id = 0;

/* function declarations */
static void blower_settings_read(uint8_t *state, float *Ppa);


/****************************************************
 * STATIC FUNCTIONS
 *****************************************************/
static int voltage_range_check(uint32_t voltage_mv)
{
	if (voltage_mv < BLOWER_MVOLTS_MIN) {
		LOG_ERR("voltage %d too low", voltage_mv);
		return -EINVAL;
	}

	if (voltage_mv > BLOWER_MVOLTS_MAX) {
		LOG_ERR("voltage %d too high", voltage_mv);
		return -EINVAL;
	}
	return 0;
}

static int app_blower_state_get(uint8_t *p_blower_state)
{
	uint8_t blower_last_state = BLOWER_NOT_RUNNING;
	/* get the last blower state from memory */
	struct setting_value val;
	if (app_settings_load_single(SETTINGS_KEY_FULL_BST, &val, sizeof(struct setting_value)) != 0) {
		blower_last_state = BLOWER_NOT_RUNNING; /* keep it off if no last state found */
	}
	blower_last_state = (uint8_t) val.val1;
	*p_blower_state = blower_last_state;
	return 0;
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_POWER_OFF: {
		LOG_INF("LIB_EVENT_POWER_OFF");
		uint8_t state;
		app_blower_state_get(&state);
		if (state == BLOWER_RUNNING) {
			app_blower_settings_change_state(APP_BLOWER_STOP);
			do {
				app_blower_state_get(&state);
			} while (state == BLOWER_RUNNING);
		}
		break;
	}
	case LIB_EVENT_SETTINGS_CHANGED:
	{
		char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
		app_settings_changed_latest_get(changed_setting);

		if ((strcmp(changed_setting, SETTINGS_KEY_FULL_BST) == 0) ||
				(strcmp(changed_setting, SETTINGS_KEY_FULL_TS_FIC) == 0)) {
			uint8_t state;
			float pid_sv_Ppa;
			blower_settings_read(&state, &pid_sv_Ppa);

#if (CONFIG_APP_DISPLAY)
			/* update display */
			c20x_screen_dashboard_pressure_label_set(
					(float) (pid_sv_Ppa / (float)PRESS_PA_TO_CMH2O_DIV));
#endif
			k_sem_take(&m_blw_settings_lock, K_FOREVER);
			m_blower_settings_changed = true;
			k_sem_give(&m_blw_settings_lock);
			/* resume the pid thread if it was suspended */
			k_thread_resume(m_pid_tid);
		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DEV_BMODE) == 0) {
			struct setting_value val;
			if (app_settings_load_single(SETTINGS_KEY_FULL_DEV_BMODE, &val, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_DEV_BMODE);
			}
			app_blower_oper_mode_set(val.val1);
			k_sem_take(&m_blw_settings_lock, K_FOREVER);
			m_blower_settings_changed = true;
			k_sem_give(&m_blw_settings_lock);
			/* resume the pid thread if it was suspended */
			k_thread_resume(m_pid_tid);
		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DEV_BRPM) == 0) {

		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_DEV_BRAMP) == 0) {

		} /*inhale wall pressure sensor id*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_SP_PRESSINW) == 0) {
			struct setting_value inhale_wall_sensor_id;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSINW, &inhale_wall_sensor_id, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_CS_SP_PRESSINW);
			}
			g_inhale_wall_press_id = inhale_wall_sensor_id.val1;

		} /*inhale pitot-tube pressure sensor id*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_SP_PRESSINPT) == 0) {
			struct setting_value inhale_pitot_tube_sensor_id;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSINPT, &inhale_pitot_tube_sensor_id, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_CS_SP_PRESSINPT);
			}
			g_inhale_pitot_tube_press_id = inhale_pitot_tube_sensor_id.val1;

		} /*exhale wall pressure sensor id*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_SP_PRESSEXW) == 0) {
			struct setting_value exhale_wall_sensor_id;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSEXW, &exhale_wall_sensor_id, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_CS_SP_PRESSEXW);
			}
			g_exhale_wall_press_id = exhale_wall_sensor_id.val1;

		} /*exhale pitot-tube pressure sensor id*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_SP_PRESSEXPT) == 0) {
			struct setting_value exhale_pitot_tube_sensor_id;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSEXPT, &exhale_pitot_tube_sensor_id, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_CS_SP_PRESSEXPT);
			}
			g_exhale_pitot_tube_press_id = exhale_pitot_tube_sensor_id.val1;

		} /*ambient pressure sensor id*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_SP_PRESSAMB) == 0) {
			struct setting_value ambient_sensor_id;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSAMB, &ambient_sensor_id, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_CS_SP_PRESSAMB);
			}
			g_ambi_press_id = ambient_sensor_id.val1;
		}
		break;
	}
	default:
		LOG_INF("%d", event);
		break;
	}
}

//#define RAMP_DURATION		(3000)
static int blower_start()
{
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A101)
	if (app_blower_oper_mode_get() == APP_BLOWER_OPER_MODE_TEST)
		ret = bsp_blower_spdRamp_set(-m_blower_test_mode_rpm, m_blower_speed_ramp_ms);
	else
		ret = bsp_blower_spdRamp_set(-10000, m_blower_speed_ramp_ms);
#elif (CONFIG_BLOWER_MOTOR_A102)
	if (app_blower_oper_mode_get() == APP_BLOWER_OPER_MODE_TEST)
		ret = bsp_blower_spdRamp_set(m_blower_test_mode_rpm, m_blower_speed_ramp_ms);
	else
		ret = bsp_blower_spdRamp_set(10000, m_blower_speed_ramp_ms);
#elif (CONFIG_BLOWER_MOTOR_A2)
	float vset = (float)((float)BLOWER_MVOLTS_MIN/1000);
	ret = bsp_blower_oper_voltage_set(vset);
#endif
	if (ret == 0) {
		if (bsp_blower_on() < 0) {
			LOG_ERR("bsp_blower_on failed");
			bsp_blower_fault_ack();
			return -1;
		}
	} else {
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		LOG_ERR("bsp_blower_spdRamp_set failed");
#elif (CONFIG_BLOWER_MOTOR_A2)
		LOG_ERR("bsp_blower_oper_voltage_set failed");
#endif
		bsp_blower_fault_ack();
		return -1;
	}
	return 0;
}

#define SETTINGS_BLOWER_PRESS_PA 	(200.0)
static void blower_settings_read(uint8_t *state, float *Ppa)
{
	/* blower state */
	struct setting_value val;
	if (app_settings_load_single(SETTINGS_KEY_FULL_BST, &val, sizeof(struct setting_value)) != 0) {
		LOG_ERR("app_settings_load_single for %s failed", SETTINGS_KEY_FULL_BST);
		*state = BLOWER_NOT_RUNNING; /* keep it off if no last state found */
	}
	*state = (uint8_t) val.val1;

	/* pressure */
	uint16_t curr_mode = app_settings_curr_mode_get();
	switch (curr_mode) {
	case MODE_SNORESTOP:
	case MDOE_PAPR:
	case MODE_CPAP:
	{
		if (app_settings_load_single(SETTINGS_KEY_FULL_TS_FIC, &val, sizeof(struct setting_value)) != 0) {
			LOG_ERR("app_settings_load_single for %s failed",
					SETTINGS_KEY_FULL_TS_FIC);
			*Ppa = SETTINGS_BLOWER_PRESS_PA; /* default value Pa*/
		}
		float pa_cmh2o = setting_value_to_double(&val);
		*Ppa = (float)(pa_cmh2o * (float)PRESS_CMH2O_TO_PA_MUL);	// cmh2o to pascal
	}
		break;
	case MODE_CPAP_BOOST:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	case MODE_BILEVEL_AT_RATE:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	case MODE_PRESSURE_SUPPORT:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	case MODE_BILEVEL_BACKUP_RATE:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	case MODE_AUTO_CPAP:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	case MODE_AUTO_BILEVEL:
		*Ppa = SETTINGS_BLOWER_PRESS_PA; /* mode not supported, assign a default value Pa*/
		break;
	default:
		break;
	}
}

int m_ramp_div_const = 13;
//float m_sample_time_s = 0.02;

/*Sample time in ms*/
float sample_time = 20;

static uint32_t compute_ramp_ms(int32_t spd_diff)
{
#if (CONFIG_BOARD_C204_CORE)
	const int ramp_div = 20;
#else
	const int ramp_div = m_ramp_div_const;
#endif
	uint32_t ramp_ms = (spd_diff / ramp_div);
	if (ramp_ms <= 0)	ramp_ms = 1;
	return ramp_ms;
}

#if CONFIG_LIB_PID_CONTROL
	/* Controller parameters */
	#define PID_KP  				10.0f
	#define PID_KI  				0.2f
	#define PID_KD  				0.1f

	#define PID_TAU 				0.02f

	#define PID_LIM_MIN 			-3
	#define PID_LIM_MAX 			3

	#define PID_LIM_MIN_INT 		-1.23
	#define PID_LIM_MAX_INT  		1.23

	#define SAMPLE_TIME_S 			0.02f
#endif	/* CONFIG_LIB_PID_CONTROL */
#define RAMP_MS_MIN		(200)
#define PID_ACTUATOR_CONSTANT	(400)
enum {
	PID_ERR_NONE=0,
	PID_ERR_BLOWER_DRIVER_FAULT,
	PID_ERR_BLOWER_START_FAIL,
	PID_ERR_BLOWER_STOP_FAIL,
	PID_ERR_BLOWER_VOLTS_MEASURE_FAIL,
	PID_ERR_INCORRECT_MODE,

	PID_ERR_MAX
} PID_LOOP_ERR;

PIDController pid = { PID_KP, PID_KI, PID_KD,
                          PID_TAU,
                          PID_LIM_MIN, PID_LIM_MAX,
						  PID_LIM_MIN_INT, PID_LIM_MAX_INT,
                          SAMPLE_TIME_S };

/*Sets the KP parameter for PID loop*/
int app_blower_kp_set (float kp)
{
	pid.Kp = kp;
	return 0;
}

/*Sets the KI parameter for PID loop*/
int app_blower_ki_set (float ki)
{
	pid.Ki = ki;
	return 0;
}

/*Sets the KD parameter for PID loop*/
int app_blower_kd_set (float kd)
{
	pid.Kd = kd;
	return 0;
}

/*Sets the sample time parameter for PID loop*/
int app_blower_sample_time_set (float sample_time_ms)
{
	pid.T = sample_time_ms / 1000;
	sample_time = sample_time_ms;
	//LOG_INF("Set sample time - %0.2f",m_sample_time_s );
	return 0;
}

/*Sets the ramp_div constant parameter for PID loop*/
int app_blower_ramp_div_set (int ramp_div)
{
	m_ramp_div_const =  ramp_div;
	return 0;
}

/* queue for print pressure data */
static struct k_fifo m_press_fifo;
struct k_fifo* app_blower_press_fifo_get()
{
	return &m_press_fifo;
}

static void blower_pid_control_thread(void *p1, void *p2, void *p3)
{
	int ret = 0;
	int blw_state_err = PID_ERR_NONE;
    /* Initialize PID controller */
    /*
	PIDController pid = { PID_KP, PID_KI, PID_KD,
                          PID_TAU,
                          PID_LIM_MIN, PID_LIM_MAX,
						  PID_LIM_MIN_INT, PID_LIM_MAX_INT,
                          SAMPLE_TIME_S };
    */
    float pid_sv_Ppa = SETTINGS_BLOWER_PRESS_PA;	// in pascal

    /* read settings */
	blower_settings_read(&m_pid_blower_state, &pid_sv_Ppa);
	m_pid_sv_Pkpa = (float)(pid_sv_Ppa / 1000.0f); 		// Pa to kPa

	/* set actuator change constant */
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	m_pid_act_change_const = PID_ACTUATOR_CONSTANT;
#elif (CONFIG_BLOWER_MOTOR_A2)
	m_pid_act_change_const = 100;
#endif

	float amb_press_kpa, tube_press_kpa=0, pid_press_kpa;
	int32_t blower_mvolts_acquired;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	int32_t curr_spd_rpm, new_spd_rpm = 10000, change = 0, spd_diff=0;
#elif (CONFIG_BLOWER_MOTOR_A2)
	int32_t curr_mvolts=0, new_mvolts = 8500, change = 0, mvolts_diff=0;
#endif
	uint32_t ramp_ms = RAMP_MS_MIN;
	uint16_t fault = MC_NO_FAULTS;

	int64_t start, delta=0, /*sample_time,*/ delay,ex_loop;
	//sample_time = m_sample_time_s * 1000;

	/* measure pressure and adjust speed / voltage */
	//app_sensor_pressure_kpa_get(1, &amb_press_kpa);

	while (1) {
		//int64_t ex_delta1 = k_uptime_get();
    	ex_loop = k_uptime_get();

    	/* wait for user to issue a start command */
    	if (!m_pid_blower_start) {
    		if (blw_state_err == PID_ERR_NONE) {
				/* turn off blower */
				if (bsp_blower_off() < 0) {
					LOG_ERR("%s, %d: bsp_blower_off failed", __func__, __LINE__);
					bsp_blower_fault_ack();
				}
			} else if (blw_state_err == PID_ERR_INCORRECT_MODE) {
				app_blower_settings_change_state(APP_BLOWER_STOP);
    			blw_state_err = PID_ERR_NONE;
			} else if (blw_state_err != PID_ERR_BLOWER_START_FAIL) {
				// blower start failed, try again
				LOG_INF("blower start failed, try again ...");
    			app_blower_settings_change_state(APP_BLOWER_START);
				m_blw_setting_state = APP_BLOWER_SETTINGS_BUSY;
    			blw_state_err = PID_ERR_NONE;
    		} else {
    			// do nothing
    		}

    		BLOWER_PARAMS_UPDATE((blower_mvolts_acquired), 0, 0, 0, tube_press_kpa, 0, 0)
    		LOG_INF("Suspending %s thread", (k_thread_name_get(m_pid_tid)));
    		k_thread_suspend(m_pid_tid);
    		LOG_INF("Resumed %s thread", (k_thread_name_get(m_pid_tid)));
//    		m_blw_setting_state = APP_BLOWER_SETTINGS_READY;

    		/* if any settings are changed, the thread will be resumed to get here */
    		if (m_blower_settings_changed) {
    			k_sem_take(&m_blw_settings_lock, K_FOREVER);
    			m_blower_settings_changed = false;
    			k_sem_give(&m_blw_settings_lock);

				/* read settings */
    			blower_settings_read(&m_pid_blower_state, &pid_sv_Ppa);

    			if (m_pid_blower_state == BLOWER_RUNNING)	m_pid_blower_start = true;
    			else    									m_pid_blower_start = false;
    			m_pid_sv_Pkpa = (float)(pid_sv_Ppa / 1000.0f); 		// Pa to kPa
    			LOG_INF("state = %s, %0.1f", m_pid_blower_state ? "BLOWER START" : "BLOWER STOP", (double)m_pid_sv_Pkpa);
    		}

			if (m_pid_blower_start) {
				/* this thread is resumed only if start command has been issued */
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
				/* check the blower voltage */
				bsp_blower_oper_voltage_get(&blower_mvolts_acquired);
				if (voltage_range_check(blower_mvolts_acquired) < 0) {
					m_pid_blower_start = false;
					blw_state_err = PID_ERR_BLOWER_VOLTS_MEASURE_FAIL;
					continue;
				}
#elif (CONFIG_BLOWER_MOTOR_A2)
#endif
				/* so start the blower */
				if (blower_start() < 0) {
					m_pid_blower_start = false;
					blw_state_err = PID_ERR_BLOWER_START_FAIL;
					continue;
				}
				m_blw_setting_state = APP_BLOWER_SETTINGS_READY;

				/* the blowers will take some time to start, so put a delay in execution here
				 * this will prevent the PID to overshoot during start */
				/* TODO: check if this delay can be removed */
				k_sleep(K_MSEC(2000));

				fault = MC_NO_FAULTS;
				/*Log to check the set values of KP, KI, KD*/
				/* measure pressure and adjust speed / voltage */
				app_sensor_pressure_kpa_get(g_ambi_press_id, &amb_press_kpa);
				LOG_INF("id: %d Ambient = %0.2fcmH2O", g_ambi_press_id, (double)amb_press_kpa * PRESS_KPA_TO_CMH2O_MUL);
				LOG_INF("LOG: KP-%0.2f, KI-%0.2f, KD-%0.2f ----",(double)pid.Kp,(double)pid.Ki,(double)pid.Kd);
				/* reset pid variables */
				PIDController_Init(&pid);
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
				new_spd_rpm = 10000;
				change = 0;
				spd_diff = 0;
#elif (CONFIG_BLOWER_MOTOR_A2)
				new_mvolts = BLOWER_MVOLTS_MIN;
				change = 0;
				mvolts_diff=0;
#endif
			} else {
				/* blower not started, only other settings were changed */
				continue;
			}
    	}

    	 /**************************
    	 * 	start PID loop
    	 * *************************/

    	/* check if settings were changed */
    	if (m_blower_settings_changed) {
			k_sem_take(&m_blw_settings_lock, K_FOREVER);
			m_blower_settings_changed = false;
			k_sem_give(&m_blw_settings_lock);

			/* read settings */
			blower_settings_read(&m_pid_blower_state, &pid_sv_Ppa);

			if (m_pid_blower_state == BLOWER_RUNNING)	m_pid_blower_start = true;
			else    									m_pid_blower_start = false;
			m_pid_sv_Pkpa = (float)(pid_sv_Ppa / 1000.0f); 		// Pa to kPa

			LOG_INF("state = %s, %0.1f", m_pid_blower_state ? "BLOWER START" : "BLOWER STOP", (double)m_pid_sv_Pkpa);
		}

    	/* check mode */
		if (m_blower_oper_mode != APP_BLOWER_OPER_MODE_PID) {
//			LOG_WRN("Oper mode is not PID_MODE!");
			k_sleep(K_MSEC(100));
			continue;
		}

    	/* measure the blower voltage */
		bsp_blower_oper_voltage_get(&blower_mvolts_acquired);

		/* measure pressure and adjust speed / voltage */
		//app_sensor_pressure_kpa_get(1, &amb_press_kpa);
		/*Checking of the global variable to print the pressure data from the queue*/
		//int64_t avg_start = k_uptime_get();
		//int64_t ex_press_fetch = k_uptime_get();
		float press_data[4] = {0};

		app_sensor_pressure_kpa_get(g_exhale_wall_press_id, &press_data[0]);
		app_sensor_pressure_kpa_get(g_exhale_pitot_tube_press_id, &press_data[1]);
		app_sensor_pressure_kpa_get(g_inhale_wall_press_id, &press_data[2]);
		app_sensor_pressure_kpa_get(g_inhale_pitot_tube_press_id, &press_data[3]);
		//LOG_DBG("press_loop_time = %lld", k_uptime_delta(&avg_start));
		//LOG_INF("id: %d, Exhale-wall = %0.2fcmH2O",g_exhale_wall_press_id, (double)press_data[0] * PRESS_KPA_TO_CMH2O_MUL);
		//LOG_INF("id: %d, Exhale-pito = %0.2fcmH2O",g_exhale_pitot_tube_press_id, (double)press_data[1] * PRESS_KPA_TO_CMH2O_MUL);
		LOG_INF("id: %d, Inhale-wall = %0.2fcmH2O",g_inhale_wall_press_id, (double)press_data[2] * PRESS_KPA_TO_CMH2O_MUL);
		//LOG_INF("id: %d, Inhale-pito = %0.2fcmH2O",g_inhale_pitot_tube_press_id, (double)press_data[3] * PRESS_KPA_TO_CMH2O_MUL);
		LOG_INF("id: %d Ambient = %0.2fcmH2O",g_ambi_press_id, (double)amb_press_kpa * PRESS_KPA_TO_CMH2O_MUL);
		pid_press_kpa = press_data[2] - amb_press_kpa;
		//pid_press_kpa = tube_press_kpa - amb_press_kpa;

		/* Compute new control signal */
		PIDController_Update(&pid, m_pid_sv_Pkpa, pid_press_kpa);
		//LOG_INF("Press fetch time = %lldms",k_uptime_delta(&ex_press_fetch));
		if (g_press_csv == 1) {
			/* queue pressure data for print purpose */
			struct press_data *pd = (struct press_data*) calloc(1,
					sizeof(struct press_data));
			if (pd == NULL) {
				LOG_ERR("%s calloc failed", __func__);
				k_sleep(K_MSEC(10));
				continue;
			}
			memcpy(&pd->data, press_data, sizeof(press_data));
			/* queue put */
			k_fifo_put(&m_press_fifo, pd);
		}
		//LOG_INF("Set sample time - %0.2f",m_sample_time_s );
		change = m_pid_act_change_const * pid.out;
		//LOG_INF("Delta 1 - %lldms",k_uptime_delta(&ex_delta1));
		start = k_uptime_get();

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		new_spd_rpm = new_spd_rpm + change;
		if (new_spd_rpm > SPEED_MAX)	new_spd_rpm = SPEED_MAX;
		else if (new_spd_rpm < SPEED_MIN)	new_spd_rpm = SPEED_MIN;

#elif (CONFIG_BLOWER_MOTOR_A2)
		new_mvolts = new_mvolts + change;
		if (new_mvolts > BLOWER_MVOLTS_MAX)	new_mvolts = BLOWER_MVOLTS_MAX;
		else if (new_mvolts < BLOWER_MVOLTS_MIN)	new_mvolts = BLOWER_MVOLTS_MIN;
#endif

		/* get blower speed and faults */
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
//		ret = bsp_blower_speed_get(NULL, NULL, &curr_spd_rpm);
		ret = bsp_blower_runstat_get(&fault, &curr_spd_rpm);

		/* check and report faults */
		if (fault != MC_NO_FAULTS) {
			lib_events_report_event(LIB_EVENT_BLOWER_FAULT);
			LOG_ERR("%s, %d: blower fault = 0x%x", __func__, __LINE__, fault);
			m_pid_blower_start = false;
			blw_state_err = PID_ERR_BLOWER_DRIVER_FAULT;
			bsp_blower_fault_ack();
			continue;
		}

		/* calculate ramp duration */
		curr_spd_rpm = abs(curr_spd_rpm);
		spd_diff = new_spd_rpm - curr_spd_rpm;
		spd_diff = abs(spd_diff);
		ramp_ms = compute_ramp_ms(spd_diff);

#elif (CONFIG_BLOWER_MOTOR_A2)
		bsp_blower_oper_voltage_get(&curr_mvolts);
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */

#if (CONFIG_BLOWER_MOTOR_A101)
		ret = bsp_blower_spdRamp_set(-new_spd_rpm, ramp_ms);
		if (ret < 0){
			// TODO: handle error
		}
#elif (CONFIG_BLOWER_MOTOR_A102)
//		ret = bsp_blower_speed_set(new_spd_rpm);
		ret = bsp_blower_spdRamp_set(new_spd_rpm, ramp_ms);
		if (ret < 0){
			// TODO: handle error
		}
#elif (CONFIG_BLOWER_MOTOR_A2)
		float new_v = (float)(((float)new_mvolts) / 1000);
		ret = bsp_blower_oper_voltage_set(new_v);
		if (ret < 0){
			// TODO: handle error
		}
#endif

		/* send data to storage queue */

		/* read motor stats and faults and take action */

		/* check user and system events and take action */

		/* compute sleep delay */
		delta = k_uptime_delta(&start);
		//int64_t ex_delta_3 = k_uptime_get();
		delay = sample_time - delta;
		if (delay <= 0) delay = 1;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		LOG_DBG("Curr_sp = %dRPM, New_sp = %dRPM",curr_spd_rpm,new_spd_rpm);
#endif

		/* sleep */
		k_sleep(K_MSEC(delay /*+ ramp_ms*/));

		/* update blower params */
		k_sem_take(&m_blw_params_lock, K_FOREVER);
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		BLOWER_PARAMS_UPDATE((blower_mvolts_acquired), 0, curr_spd_rpm, fault, tube_press_kpa, pid_press_kpa, 0)
#elif (CONFIG_BLOWER_MOTOR_A2)
		BLOWER_PARAMS_UPDATE((blower_mvolts_acquired), 0, 0, 0, tube_press_kpa, pid_press_kpa, 0);
#endif
		k_sem_give(&m_blw_params_lock);

		/* debug logs */
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		LOG_DBG("Delta 2 - %lldms, Sample time - %0.2fms, Ramp div_constant - %d, Sleep - %lld", delta, (double)sample_time, m_ramp_div_const, delay);
		if (ramp_ms > RAMP_MS_MIN) {
			LOG_DBG("%.4f, %.4f, %d, %d, %d, %d",
					(double)m_pid_sv_Pkpa, (double)pid_press_kpa, change, curr_spd_rpm, new_spd_rpm, ramp_ms);
		}
		LOG_DBG("%.4f, %.4f, %.4f, %.4f, %d, %d, %d, %d, %d",
				(double)m_pid_sv_Pkpa, (double)pid_press_kpa, (double)pid.out, (double)pid.prevError, change, curr_spd_rpm, new_spd_rpm, spd_diff, ramp_ms);
#elif (CONFIG_BLOWER_MOTOR_A2)
		LOG_DBG("%.4f, %.4f, %.4f, %.4f, %d, %d, %d",
				(double)m_pid_sv_Pkpa, (double)pid_press_kpa, (double)pid.out, (double)pid.prevError, change, curr_mvolts, new_mvolts/*, mvolts_diff, ramp_ms*/);
#endif
		//ex_loop_log = k_uptime_delta(&ex_loop);
		//LOG_INF("Delta 3 = %lldms", k_uptime_delta(&ex_delta_3));
		//LOG_INF("Loop_time = %lldms", k_uptime_delta(&ex_loop));
		//LOG_INF("New_sp = %d, Curr_sp = %d", new_spd_rpm, curr_spd_rpm);
    }
}



/****************************************************
 * GLOBAL FUNCTIONS
 *****************************************************/
int app_blower_voltage_mv_change(uint32_t voltage_mv)
{
	int ret = 0;
	/* voltage can be changed in TEST mode only */
	if (m_blower_oper_mode != APP_BLOWER_OPER_MODE_TEST)
		return -ENOTSUP;
	if (voltage_range_check(voltage_mv) < 0)
		return -EINVAL;

	/* change the voltage */
	if (bsp_blower_oper_voltage_set(voltage_mv/1000) < 0)
		return -1;

	return ret;
}

int app_blower_speed_rpm_change(uint32_t speed_rpm)
{
	int ret = 0;
	/* speed can be changed in TEST mode only */
	if (m_blower_oper_mode != APP_BLOWER_OPER_MODE_TEST)
		return -ENOTSUP;

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	if ((speed_rpm < SPEED_MIN) || (speed_rpm > SPEED_MAX))
		return -EINVAL;
	uint32_t ramp_ms = m_blower_speed_ramp_ms;
	m_blower_test_mode_rpm = speed_rpm;
	#if (CONFIG_BLOWER_MOTOR_A101)
		ret = bsp_blower_spdRamp_set(-speed_rpm, ramp_ms);
	#elif (CONFIG_BLOWER_MOTOR_A102)
		ret = bsp_blower_spdRamp_set(speed_rpm, ramp_ms);
	#elif (CONFIG_BLOWER_MOTOR_A2)
		LOG_ERR("Not supported by blower");
		return -ENOTSUP;
	#endif
	if (ret < 0){
		LOG_ERR("bsp_blower_spdRamp_set failed, %d", ret);
	}
#endif
	return ret;
}

int app_blower_duty_percent_change(uint8_t duty_percent)
{
	int ret = 0;
	/* duty can be changed in TEST mode only */
	if (m_blower_oper_mode != APP_BLOWER_OPER_MODE_TEST)
		return -ENOTSUP;
	if (duty_percent > 100)
		duty_percent = 100;
#if (CONFIG_BLOWER_MOTOR_A2)
	ret = bsp_blower_duty_cycle_set(duty_percent);
#else
	LOG_ERR("Not supported by blower");
	return -ENOTSUP;
#endif
	return ret;
}

int app_blower_settings_change_state(uint8_t state)
{
	int ret = 0;

	if ((state != APP_BLOWER_START) && (state != APP_BLOWER_STOP)) {
		LOG_ERR("Trying to set invalid state %d", state);
		return -EINVAL;
	}
	/* for TEST mode start / stop blower here */
//	if (m_blower_oper_mode == APP_BLOWER_OPER_MODE_TEST) {
//		if (state == APP_BLOWER_START) {
//			if ((ret = blower_start()) < 0) {
//				LOG_ERR("blower_start failed");
//			}
//		} else if (state == APP_BLOWER_STOP) {
//    		if ((ret = bsp_blower_off()) < 0) {
//				LOG_ERR("bsp_blower_off failed");
//				bsp_blower_fault_ack();
//    		}
//		}
//	} else {
		if (m_blw_setting_state == APP_BLOWER_SETTINGS_BUSY)
			return -EBUSY;

		struct setting_value val;
		val.val1 = state;
		val.val2 = 0;
		ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_BST, &val,
				sizeof(struct setting_value), 10, true);
		if (ret != 0) {
			LOG_ERR("app_settings_save_single_with_retry failed for %s",
					SETTINGS_KEY_FULL_BST);
		}
//	}

	return ret;
}

int app_blower_runtime_params_get(struct app_blower_params *pblw_params)
{
	int ret = 0;
	if (pblw_params == NULL) {
		return -EINVAL;
	}

	/* for PID mode, the runtime params are updated by the PID thread
	 * for TEST mode, we update voltage, faults and rpm here */
	if (m_blower_oper_mode == APP_BLOWER_OPER_MODE_TEST) {
		/* get voltage */
		uint32_t blw_volts;
		bsp_blower_oper_voltage_get(&blw_volts);
		m_blower_params.acq_volt_mv = (blw_volts);
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		/* get speed */
		uint16_t fault = MC_NO_FAULTS;
		int32_t curr_spd_rpm=0;
		ret = bsp_blower_runstat_get(&fault, &curr_spd_rpm);
		m_blower_params.faults = fault;
		m_blower_params.acq_speed_rpm = abs(curr_spd_rpm);
#endif
	} else {
	}
	memcpy(pblw_params, &m_blower_params, sizeof(m_blower_params));
	return ret;
}

uint8_t app_blower_run_state_get()
{
	if (m_blower_oper_mode == APP_BLOWER_OPER_MODE_TEST) {
		// TODO
	}
	return m_pid_blower_state;
}

int app_blower_speed_rpm_get(int32_t *speed_rpm)
{
	struct app_blower_params params;
	int ret = app_blower_runtime_params_get(&params);
	if (ret == 0) {
		*speed_rpm = params.acq_speed_rpm;
	} else {
		*speed_rpm = 0;
	}
	return ret;
}

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
int app_blower_fault_ack()
{
	uint8_t state;
	app_blower_state_get(&state);
	int ret=0;
	if (state == BLOWER_NOT_RUNNING)
		ret = bsp_blower_fault_ack();
	else {
		LOG_ERR("cannot process when blower is running");
		ret = -1;
	}
	return ret;
}
int app_blower_power_get(uint16_t *power_avg_w, uint16_t *power_inst_w)
{
	int ret = 0;
	return ret;
}
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */

float app_blower_set_press_cmh2o_get()
{
	float p_cmh2o = (float)m_pid_sv_Pkpa * (float)10.1972;	// 1 kilopascal = 10.1972 cm h2o
	return p_cmh2o;
}

void app_blower_pid_sv_change(const float set_val)	// change pressure value in cmH2O
{
	/* update runtime variable */
//	m_pid_sv_Pkpa = set_val;

	/* save setting */
	struct setting_value val;
	setting_value_from_double(&val, set_val);
//	app_settings_save_single(SETTINGS_KEY_FULL_TS_FIC, &val, sizeof(struct setting_value), true);
	int ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_TS_FIC, &val, sizeof(struct setting_value), 10, true);
	if (ret != 0) {
		LOG_ERR("app_settings_save_single_with_retry failed for %s", SETTINGS_KEY_FULL_TS_FIC);
	}
}

void app_blower_pid_actconst_change(const uint32_t actconst)
{
	m_pid_act_change_const = actconst;
}

uint32_t app_blower_pid_actconst_get()
{
	return m_pid_act_change_const;
}

int app_blower_oper_mode_set(uint8_t oper_mode)
{
	if (oper_mode >= APP_BLOWER_OPER_MODE_MAX)	return -EINVAL;
	m_blower_oper_mode = oper_mode;
	return 0;
}

uint8_t app_blower_oper_mode_get()
{
	return m_blower_oper_mode;
}

int app_blower_ramp_ms_set(uint32_t ramp_ms)
{
	if (m_blower_oper_mode == APP_BLOWER_OPER_MODE_TEST) {
		m_blower_speed_ramp_ms = ramp_ms;
		return 0;
	} else {
		return -1;
	}
}

uint32_t app_blower_ramp_ms_get()
{
	return m_blower_speed_ramp_ms;
}

int app_blower_init()
{
	int ret = 0;

	ret = bsp_blower_init();
	if (ret != 0) {
		LOG_ERR("Failed to enable bsl_blower");
		return ret;
	}

	memset(&m_blower_params, 0x00, sizeof(m_blower_params));
	k_sem_init(&m_blw_settings_lock, 1, 1);
	k_sem_init(&m_blw_params_lock, 1, 1);

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	/* TODO setting the bldc_rst pin causes problems with writing to flash.
	 * Getting PGSERR don't know why */
	/* reset the blower driver */
	ret = bsp_blower_reset();
	if (ret < 0) {
		LOG_ERR("Could not reset blower driver!");
		return ret;
	}
	/* wait for driver to boot up */
	k_sleep(K_MSEC(2000));
#endif

	/* keep blower off by default */
	struct setting_value val;
	val.val1 = 0; val.val2 = 0;
	ret = app_settings_save_single_with_retry(SETTINGS_KEY_FULL_BST, &val, sizeof(struct setting_value), 10, true);
	if (ret != 0) {
		LOG_ERR("app_settings_save_single_with_retry failed for %s", SETTINGS_KEY_FULL_BST);
	}

	/* get the last mode */
	ret = app_settings_load_single(SETTINGS_KEY_FULL_DEV_BMODE, &val, sizeof(struct setting_value));
	if (!ret) {
		app_blower_oper_mode_set(val.val1);
	} else {
		LOG_ERR("could not load last blower mode");
	}

	/* get test mode params */
	app_settings_load_single(SETTINGS_KEY_FULL_DEV_BRAMP, &m_blower_speed_ramp_ms, sizeof(uint32_t));
	app_settings_load_single(SETTINGS_KEY_FULL_DEV_BRPM, &m_blower_test_mode_rpm, sizeof(uint32_t));

	/*maps the inhale wall pressure sensor id on location*/
	struct setting_value inhale_wall_sensor_id;
	app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSINW, &inhale_wall_sensor_id, sizeof(struct setting_value));
	LOG_INF("Inhale wall pressure id: %d", inhale_wall_sensor_id.val1);
	g_inhale_wall_press_id = inhale_wall_sensor_id.val1;

	/*maps the inhale pito-tube pressure sensor id on location*/
	struct setting_value inhale_pitot_tube_sensor_id;
	app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSINPT, &inhale_pitot_tube_sensor_id, sizeof(struct setting_value));
	LOG_INF("Inhale pitot-tube pressure id: %d", inhale_pitot_tube_sensor_id.val1);
	g_inhale_pitot_tube_press_id = inhale_pitot_tube_sensor_id.val1;

	/*maps the exhale wall pressure sensor id on location*/
	struct setting_value exhale_wall_sensor_id;
	app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSEXW, &exhale_wall_sensor_id, sizeof(struct setting_value));
	LOG_INF("Exhale wall pressure id: %d", exhale_wall_sensor_id.val1);
	g_exhale_wall_press_id = exhale_wall_sensor_id.val1;

	/*maps the exhale pitot-tube pressure sensor id on location*/
	struct setting_value exhale_pitot_tube_sensor_id;
	app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSEXPT, &exhale_pitot_tube_sensor_id, sizeof(struct setting_value));
	LOG_INF("Exhale pitot-tube pressure id: %d", exhale_pitot_tube_sensor_id.val1);
	g_exhale_pitot_tube_press_id = exhale_pitot_tube_sensor_id.val1;

	/*maps the ambient pressure sensor id on location*/
	struct setting_value ambient_sensor_id;
	app_settings_load_single(SETTINGS_KEY_FULL_CS_SP_PRESSAMB, &ambient_sensor_id, sizeof(struct setting_value));
	LOG_INF("Ambient pressure id: %d", ambient_sensor_id.val1);
	g_ambi_press_id = ambient_sensor_id.val1;

#if CONFIG_LIB_FILE_OPER
	/* create / open the sensor log file */
	m_sensor_log_file_handle = lib_file_oper_create_open_file(
			SENSOR_LOG_DIRECTORY_PATH,
			SENSOR_LOG_CURR_FILE_NAME,
			SENSOR_LOG_CURR_FILE_PATH,
			SENSOR_LOG_CURR_FILE_MAX_SIZE_BYTES,
			SENSOR_LOG_MAX_FILE_COUNT,
			(FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND));
#endif
	ret = lib_events_callback_add(&m_cb_poweroff, app_event_handler, LIB_EVENT_POWER_OFF);
	ret = lib_events_callback_add(&m_cb_settings_changed, app_event_handler, LIB_EVENT_SETTINGS_CHANGED);

	/* start blower PID thread */
	m_pid_tid = k_thread_create(&m_pid_data,
			m_pid_stack,
			K_THREAD_STACK_SIZEOF(m_pid_stack),
			blower_pid_control_thread, NULL, NULL, NULL, APP_THREAD_PRIO_PID,
			0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_pid_tid, APP_THREAD_NAME_PID);
#endif
	return ret;
}
