/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_stepper);

#include "app_stepper/bsp_stepper.h"
#include "app_stepper/app_stepper.h"
#include "app_settings/app_settings.h"
#include "lib_events/lib_events.h"
#include "app_thread_configs.h"

#define DEVCMD_SPIN_TEST	1

#define DEFAULT_RESET_POSITION			CONFIG_STEPPER_RESET_POSITION
#define DEFAULT_STEP_SPEED_HZ			CONFIG_STEPPER_STEP_SPEED_HZ
#define DEFAULT_RESET_ROTATIONS			CONFIG_STEPPER_NUM_ROTATION_TO_RESET
#define DEFAULT_RESET_DIRECTION			CONFIG_STEPPER_RESET_DIRECTION
#define DEFAULT_STEP_MODE				CONFIG_STEPPER_STEP_MODE
#define FULL_STEP_ANGLE					CONFIG_FULL_STEP_ANGLE

#if DEVCMD_SPIN_TEST
typedef enum {
	SPIN_POS_TYPE_RELATIVE,
	SPIN_POS_TYPE_ABSOLUTE,
} SPIN_POS_TYPE;

struct stepper_spin {
	uint8_t pos_type;
	uint16_t pos_degrees;
};
//static struct stepper_spin m_spin;
static struct k_fifo m_spin_fifo;
#endif

/* static variables */
static struct stepper_params m_stepper_params;
static struct lib_events_callback m_cb_poweroff;
static struct k_sem m_params_lock;

K_THREAD_STACK_DEFINE(m_app_stepper_stack, APP_THREAD_STACK_SIZE_STEPPER);
static struct k_thread m_app_stepper_data;
static k_tid_t m_app_stepper_tid;

static void app_stepper_load_settings(uint16_t *prst_pos,
		uint32_t *pstep_speed_hz, uint32_t *prst_rot, uint8_t *prst_dir,
		uint8_t *pstep_mode, uint16_t *pstep_angle) {
#if (SETTINGS_NEEDED)
	/* Get the reset position setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_RESET_POS, prst_pos,
			sizeof(*prst_pos)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_RESET_POS);
		*prst_pos = DEFAULT_RESET_POSITION;
	}

	/* Get the step speed setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_STEP_SPEED_HZ,
			pstep_speed_hz, sizeof(*pstep_speed_hz)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_STEP_SPEED_HZ);
		*pstep_speed_hz = DEFAULT_STEP_SPEED_HZ;
	}

	/* Get the reset rotation count setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_RESET_ROT_CNT,
			prst_rot, sizeof(*prst_rot)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_RESET_ROT_CNT);
		*prst_rot = DEFAULT_RESET_ROTATIONS;
	}

	/* Get the reset direction setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_RESET_DIR, prst_dir,
			sizeof(*prst_dir)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_RESET_DIR);
		*prst_dir = DEFAULT_RESET_DIRECTION;
	}

	/* Get the step mode setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_STEP_MODE,
			pstep_mode, sizeof(*pstep_mode)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_STEP_MODE);
		*pstep_mode = DEFAULT_STEP_MODE;
	}

	/* Get the step angle setting */
	if (app_settings_load_single(SETTINGS_KEY_FULL_STEPPER_STEP_ANGLE,
			pstep_angle, sizeof(*pstep_angle)) != 0) {
		LOG_ERR("failed to load settings: %s",
				SETTINGS_KEY_FULL_STEPPER_STEP_ANGLE);
		*pstep_angle = FULL_STEP_ANGLE;
	}
#else
	*prst_pos = DEFAULT_RESET_POSITION;
	*pstep_speed_hz = DEFAULT_STEP_SPEED_HZ;
	*prst_rot = DEFAULT_RESET_ROTATIONS;
	*prst_dir = DEFAULT_RESET_DIRECTION;
	*pstep_mode = DEFAULT_STEP_MODE;
	*pstep_angle = FULL_STEP_ANGLE;
#endif
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event) {
	switch (event) {
	case LIB_EVENT_POWER_OFF: {
		LOG_INF("LIB_EVENT_POWER_OFF");
	}
		break;
	default:
		LOG_INF("%d", event);
		break;
	}
}

int app_stepper_direction_set(uint8_t dir) {
	if ((dir != BSP_STEPPER_DIR_CLOCKWISE) && (dir != BSP_STEPPER_DIR_ANTICLOCKWISE))
		return -1;

	/* save new values */
//	ret = app_settings_save_single(SETTINGS_KEY_FULL_STEPPER_RESET_DIR, &dir, sizeof(dir), true);

	k_sem_take(&m_params_lock, K_FOREVER);
	m_stepper_params.dir = dir;
	k_sem_give(&m_params_lock);

	return 0;
}

int app_stepper_speed_hz_set(uint32_t speed_hz) {
	if (speed_hz == 0)
		return -1;
	int ret = 0;
#if (SETTINGS_NEEDED)
	/* save new values */
	ret = app_settings_save_single(SETTINGS_KEY_FULL_STEPPER_STEP_SPEED_HZ, &speed_hz, sizeof(speed_hz), true);
#endif
	k_sem_take(&m_params_lock, K_FOREVER);
	m_stepper_params.step_speed_hz = speed_hz;
	k_sem_give(&m_params_lock);

	return ret;
}

int app_stepper_num_rot_set(uint32_t num_rot) {
	int ret = 0;
#if (SETTINGS_NEEDED)
	/* save new values */
	ret = app_settings_save_single(SETTINGS_KEY_FULL_STEPPER_RESET_ROT_CNT, &num_rot, sizeof(num_rot), true);
#endif
	k_sem_take(&m_params_lock, K_FOREVER);
	m_stepper_params.num_rot = num_rot;
	k_sem_give(&m_params_lock);

	return ret;
}

int app_stepper_pos_rel_set(uint16_t pos_rel) {
	if (pos_rel > 360) {
		return -1;
	}
#if DEVCMD_SPIN_TEST
	struct stepper_spin *pspin = (struct stepper_spin *) calloc(1, sizeof(struct stepper_spin));
	if (pspin == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return -1;
	}

	pspin->pos_type = SPIN_POS_TYPE_RELATIVE;
	pspin->pos_degrees = pos_rel;

	k_fifo_put(&m_spin_fifo, pspin);
#endif
	return 0;
}

int app_stepper_pos_abs_set(uint16_t pos_abs) {
	if (pos_abs > 359) {
		return -1;
	}
#if DEVCMD_SPIN_TEST
	struct stepper_spin *pspin = (struct stepper_spin *) calloc(1, sizeof(struct stepper_spin));
	if (pspin == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		return -1;
	}

	pspin->pos_type = SPIN_POS_TYPE_ABSOLUTE;
	pspin->pos_degrees = pos_abs;

	k_fifo_put(&m_spin_fifo, pspin);
#endif
	return 0;
}

int app_stepper_pos_curr_get(uint16_t *pos_curr) {
	k_sem_take(&m_params_lock, K_FOREVER);
	*pos_curr = bsp_stepper_current_pos_get();
	k_sem_give(&m_params_lock);

	return 0;
}

int app_stepper_params_get(struct stepper_params *params) {
	if (params == NULL)
		return -1;

	k_sem_take(&m_params_lock, K_FOREVER);
	memcpy(params, &m_stepper_params, sizeof (m_stepper_params));
	k_sem_give(&m_params_lock);

	return 0;
}

int app_stepper_zero_set() {
	return bsp_stepper_zero_set();
}

static void app_stepper_thread(void *p1, void *p2, void *p3) {
#if DEVCMD_SPIN_TEST
	struct stepper_spin *pspin;
	int ret=0;

	/* power save */
	bsp_stepper_standby();

	while (1) {
		pspin = k_fifo_get(&m_spin_fifo, K_FOREVER);
		if (pspin == NULL) {
			continue;
		}

		/* resume from standby */
		bsp_stepper_resume();

		k_sleep(K_MSEC(10));

		if (pspin->pos_type == SPIN_POS_TYPE_RELATIVE) {
			ret = bsp_stepper_goto_pos_rel(pspin->pos_degrees, m_stepper_params.step_speed_hz, m_stepper_params.dir);
		} else if (pspin->pos_type == SPIN_POS_TYPE_ABSOLUTE) {
			ret = bsp_stepper_goto_pos_abs(pspin->pos_degrees, m_stepper_params.step_speed_hz, m_stepper_params.num_rot);
		}

		/* free memory */
		free(pspin);

		k_sleep(K_MSEC(50));

		/* power save */
		bsp_stepper_standby();
	}
#else
	while (1) {
		k_sleep(K_MSEC(5000));
	}
#endif
}

int app_stepper_init() {
	int ret = 0;

	/* get init values from settings */
	app_stepper_load_settings(
			&m_stepper_params.rst_pos,
			&m_stepper_params.step_speed_hz,
			&m_stepper_params.num_rot,
			&m_stepper_params.dir,
			&m_stepper_params.step_mode,
			&m_stepper_params.step_angle);

	/* Make the stepper motor go into reset position
	 * This is a blocking function and the stepper motor
	 * will spin and go into reset position */
	ret = bsp_stepper_init(m_stepper_params.step_mode, m_stepper_params.step_angle);
	ret |= bsp_stepper_goto_reset_pos(
			m_stepper_params.rst_pos,
			m_stepper_params.step_speed_hz,
			m_stepper_params.num_rot,
			m_stepper_params.dir);
	if (ret) {
		LOG_ERR("bsp_stepper_init failed");
		return ret;
	}
	bsp_stepper_standby();


	ret = lib_events_callback_add(&m_cb_poweroff, app_event_handler, LIB_EVENT_POWER_OFF);
	k_sem_init(&m_params_lock, 1, 1);

#if DEVCMD_SPIN_TEST
	k_fifo_init(&m_spin_fifo);
#endif

	m_app_stepper_tid = k_thread_create(&m_app_stepper_data,
			m_app_stepper_stack,
			K_THREAD_STACK_SIZEOF(m_app_stepper_stack),
			app_stepper_thread, NULL, NULL, NULL, APP_THREAD_PRIO_STEPPER,
			0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(m_app_stepper_tid, APP_THREAD_NAME_STEPPER);
#endif

#if 0
	ret = bsp_stepper_init(m_stepper_params.step_mode, m_stepper_params.step_angle);
	ret = bsp_stepper_goto_reset_pos(
			m_stepper_params.rst_pos,
			m_stepper_params.step_speed_hz,
			1,
			m_stepper_params.dir);

	LOG_INF("Stepper position = %d", bsp_stepper_current_pos_get());

	k_sleep(K_MSEC(1000));

	bsp_stepper_goto_pos_rel(30, m_stepper_params.step_speed_hz*2, BSP_STEPPER_DIR_CLOCKWISE);
	LOG_INF("Stepper position = %d", bsp_stepper_current_pos_get());

	k_sleep(K_MSEC(1000));

	bsp_stepper_goto_pos_abs(bsp_stepper_current_pos_get(), m_stepper_params.step_speed_hz*8, 2);
	LOG_INF("Stepper position = %d", bsp_stepper_current_pos_get());

//	k_sleep(K_MSEC(250));
//	bsp_stepper_goto_pos_abs(45, m_stepper_params.step_speed_hz, 0);
//	k_sleep(K_MSEC(250));
//	bsp_stepper_goto_pos_abs(157, m_stepper_params.step_speed_hz, 0);

#endif

	return ret;
}
