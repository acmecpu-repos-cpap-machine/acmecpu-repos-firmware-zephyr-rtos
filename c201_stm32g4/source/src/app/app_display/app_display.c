/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_display);

#include <zephyr/drivers/display.h>
#include <lvgl.h>

#include "lib_events/lib_events.h"
#include "bsp_display/bsp_display.h"
#include "app_thread_configs.h"
#include "app_settings/app_settings.h"

#if (CONFIG_BSP_MEMBRANE_SWITCH)
#include "bsp_membrane_switch/bsp_membrane_switch.h"
#endif

#if (CONFIG_APP_PUSH_SWITCH)
	#include "app_push_switch/app_push_switch.h"
#endif

#if (CONFIG_APP_MATRIX_KEYPAD && CONFIG_LIB_MATRIX_KEYPAD)
	#include "app_matrix_keypad/app_matrix_keypad.h"
	#include "lib_matrix_keypad/lib_matrix_keypad.h"
#endif

#include "bsp_buzzer/bsp_buzzer.h"

#include "app_display/app_display.h"
#include "app_display/app_display_menu.h"
#include "c20x_screen_manager.h"

#define DEFAULT_DISPLAY_BRIGHTNESS		CONFIG_APP_DISPLAY_BRIGHTNESS_LEVEL
//#define DISPLAY_DEV_NAME				CONFIG_LVGL_DISPLAY_DEV_NAME

/* Display thread static variables */
K_THREAD_STACK_DEFINE(app_display_refresh_stack, APP_THREAD_STACK_SIZE_DISPLAY_REFRESH);
struct k_thread app_display_refresh_thread_data;
k_tid_t app_display_refresh_tid;

/* Display automatic dimming static variables and defines */
#define APP_DISPLAY_DIMMED_LEVEL	CONFIG_APP_DISPLAY_DIMMED_LEVEL
#define DISP_DIMM_STATE_DIM		1
#define DISP_DIMM_STATE_OFF		0
struct disp_dimm {
	struct k_timer dimm_tmr;
//	struct k_timer m_off_timer;
	struct k_work dimm_worker;
	int dimm_secs;
	uint32_t off_secs;
	uint8_t dimm_set_level;
	uint8_t dimm_level;
	uint8_t bright_level;
	bool dimm_state;
};

static struct disp_dimm m_dim;
static struct lib_events_callback m_cb_boot_done;
static bool m_boot_done = false;

/* Keypad static variables */
#if (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_APP_MATRIX_KEYPAD)

#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
//static struct bsp_membr_callback key_enter_data;
//static struct bsp_membr_callback key_back_data;
//static struct bsp_membr_callback key_left_data;
//static struct bsp_membr_callback key_right_data;
//static struct bsp_membr_callback key_up_data;
//static struct bsp_membr_callback key_down_data;
//static struct bsp_membr_callback key_power_data;
//static struct bsp_membr_callback key_home_data;

static struct lib_push_switch_callback sw_enter_cb;
static struct lib_push_switch_callback sw_back_cb;
static struct lib_push_switch_callback sw_down_cb;
static struct lib_push_switch_callback sw_pwr_cb;
static struct lib_push_switch_callback sw_right_cb;
static struct lib_push_switch_callback sw_left_cb;
static struct lib_push_switch_callback sw_up_cb;
static struct lib_push_switch_callback sw_home_cb;

#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
//static struct bsp_membr_callback key_check_data;
//static struct bsp_membr_callback key_back_data;
//static struct bsp_membr_callback key_left_data;
//static struct bsp_membr_callback key_right_data;
//static struct bsp_membr_callback key_up_data;
//static struct bsp_membr_callback key_down_data;
//static struct bsp_membr_callback key_power_data;

static struct lib_push_switch_callback sw_enter_cb;
static struct lib_push_switch_callback sw_back_cb;
static struct lib_push_switch_callback sw_down_cb;
static struct lib_push_switch_callback sw_pwr_cb;
static struct lib_push_switch_callback sw_right_cb;
static struct lib_push_switch_callback sw_left_cb;
static struct lib_push_switch_callback sw_up_cb;

#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
static struct lib_push_switch_callback sw_pwr_cb;
#endif	/* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205) */

typedef enum {
	CHECK = 0,
	BACK,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	POWER,
	HOME,
	MIC,
	VOL_UP,
	VOL_DOWN,
	MUTE,

	NONE
} KEY_PRESSED;

typedef enum {
	KEY_PR=0, KEY_REL, UNKNOWN
} PRESS_TYPE;

static KEY_PRESSED m_key_pressed;
static PRESS_TYPE m_press_type;
#endif	/* (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_APP_MATRIX_KEYPAD) */

static app_display_key_cb m_screen_key_cb = NULL;
static app_display_key_cb m_screen_spl_key_cb = NULL;

static lv_indev_drv_t m_keypad_drv;
static lv_indev_t * m_keypad_indev = NULL;
static lv_group_t * m_lvgl_grp = NULL;

/*
 * *******************************************
 * THREAD / TIMER / WORKER Functions
 * *******************************************
 * */
static void dimm_worker(struct k_work *work) {
#if CONFIG_APP_DISPLAY_DIMMING_SUPPORT
	struct disp_dimm *dim = CONTAINER_OF(work, struct disp_dimm, dimm_worker);
	bsp_display_set_brightness(dim->dimm_set_level);
#else
#endif
}
static void dimm_timer_handler(struct k_timer *tmr) {
	struct disp_dimm *dim = CONTAINER_OF(tmr, struct disp_dimm, dimm_tmr);

	if (dim->dimm_state == DISP_DIMM_STATE_DIM) {
		/* dimm the display */
		dim->dimm_set_level = dim->dimm_level;
		k_work_submit(&dim->dimm_worker);

		/* start timer to turn display off */
		if (m_dim.dimm_secs > 0)
			k_timer_start(&dim->dimm_tmr, K_SECONDS(dim->off_secs - dim->dimm_secs), K_NO_WAIT);
		dim->dimm_state = DISP_DIMM_STATE_OFF;
	} else if (dim->dimm_state == DISP_DIMM_STATE_OFF) {
		/* turn display off */
		dim->dimm_set_level = 0;
		k_work_submit(&dim->dimm_worker);
	}
}

static void app_display_refresh_thread(void *p1, void *p2, void *p3) {
	/* start the display timers */
	if (m_dim.dimm_secs > 0)
		k_timer_start(&m_dim.dimm_tmr, K_SECONDS(m_dim.dimm_secs), K_NO_WAIT);
	LOG_DBG("app_display_refresh_thread started");

	while (1) {
		lv_task_handler();
		k_sleep(K_MSEC(10));
	}
}

/*
 * *******************************************
 * STATIC Functions
 * *******************************************
 * */
#if (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_APP_MATRIX_KEYPAD)
static void keypad_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {

	/* this function should return the pressed key */

	static int call_count = 0;	/* used to simulate a full switch press event */

	switch (m_key_pressed) {
	case CHECK:
		data->key = LV_KEY_ENTER;
		break;
	case BACK:
//		data->key = LV_KEY_ESC;
		break;
	case LEFT:
		data->key = LV_KEY_LEFT;
		break;
	case RIGHT:
		data->key = LV_KEY_RIGHT;
		break;
	case UP:
		data->key = LV_KEY_UP;
		break;
	case DOWN:
		data->key = LV_KEY_DOWN;
		break;
	case POWER:
		data->key = APP_DISPLAY_KEY_POWER;
		break;
	case HOME:
		data->key = APP_DISPLAY_KEY_HOME; // not implemented
		break;
	case MIC:
		data->key = APP_DISPLAY_KEY_MIC;	// not implemented
		break;
	case VOL_UP:
		data->key = APP_DISPLAY_KEY_VOL_UP;	// not implemented
		break;
	case VOL_DOWN:
		data->key = APP_DISPLAY_KEY_VOL_DOWN;	// not implemented
		break;
	case MUTE:
		data->key = APP_DISPLAY_KEY_MUTE;	// not implemented
		break;
	default:
		data->key = 0;	// not implemented
		break;
	}

	/* key press simulation */

	if (m_key_pressed == BACK) {	// special case for BACK functionality
		/* play sound */
		bsp_buzzer_play_switch_pressed();

		/* call the screen's key handler function to handle BACK / ESC key press */
		if (m_screen_key_cb != NULL)
			m_screen_key_cb(LV_KEY_ESC);
		m_key_pressed = NONE;
		m_press_type = UNKNOWN;
	}
	else if (m_key_pressed != NONE) {	// keys other than BACK
		if (call_count == 0) {
			data->state = LV_INDEV_STATE_PR;
			call_count = 1;
		} else if (call_count == 1) {
			call_count = 2;
			data->state = LV_INDEV_STATE_REL;

			/* special keys */
			if (	(m_key_pressed == POWER) || (m_key_pressed == HOME) ||
					(m_key_pressed == MIC) || (m_key_pressed == VOL_UP) ||
					(m_key_pressed == VOL_DOWN) || (m_key_pressed == MUTE) )
			{
				if (m_screen_spl_key_cb != NULL)
					m_screen_spl_key_cb(data->key);
			} else {
				/* play sound */
				bsp_buzzer_play_switch_pressed();

				/* call the screen's key handler function to handle key presses that are not
				 * processed by lvgl group */
				if (m_screen_key_cb != NULL)
					m_screen_key_cb(data->key);
			}
		}
	}

	/* reset the keys */
	if ((m_key_pressed != NONE) && (call_count == 2)) {
		call_count = 0;
		m_key_pressed = NONE;
		m_press_type = UNKNOWN;
	}

	return;
}
#endif	/* (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_APP_MATRIX_KEYPAD) */

#if (CONFIG_LIB_MATRIX_KEYPAD)
void libmk_callback_handler(int key_id)
{
	if (!m_boot_done)	return;
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	switch (key_id) {
	case APP_MK_KEY_HOME:
        LOG_DBG("HOME pressed");
		m_key_pressed = HOME;
		m_press_type = KEY_REL;
		break;
	case APP_MK_KEY_BACK:
        LOG_DBG("BACK pressed");
		m_key_pressed = BACK;
		m_press_type = KEY_REL;
		break;
	case APP_MK_KEY_MIC:
        LOG_DBG("MIC pressed");
		m_key_pressed = MIC;
		m_press_type = KEY_REL;
		break;
	case APP_MK_KEY_UP:
        LOG_DBG("UP pressed");
		m_key_pressed = UP;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_LEFT:
        LOG_DBG("LEFT pressed");
		m_key_pressed = LEFT;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_ENTER:
        LOG_DBG("ENTER pressed");
		m_key_pressed = CHECK;
		m_press_type = KEY_REL;
		break;
	case APP_MK_KEY_RIGHT:
        LOG_DBG("RIGHT pressed");
		m_key_pressed = RIGHT;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_DOWN:
        LOG_DBG("DOWN pressed");
		m_key_pressed = DOWN;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_VOL_UP:
        LOG_DBG("VOL_UP pressed");
		m_key_pressed = VOL_UP;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_VOL_DOWN:
        LOG_DBG("VOL_DOWN pressed");
		m_key_pressed = VOL_DOWN;
		m_press_type = KEY_PR;
		break;
	case APP_MK_KEY_MUTE:
        LOG_DBG("MUTE pressed");
		m_key_pressed = MUTE;
		m_press_type = KEY_PR;
		break;
	}
#endif
}
#endif

#if (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH)
//static void keypad_app_cb_handler(struct bsp_membr_callback *cb, const char *switch_label, uint32_t pin, BSP_MEMBR_SWITCH_PRESSED_TYPE press_type)
static void keypad_app_cb_handler(struct lib_push_switch_callback *cb, uint32_t pin, LIB_PUSH_SWITCH_PRESSED_TYPE press_type)
{
	if (!m_boot_done)	return;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_ENTER) && (pin == APP_PUSH_SWITCH_PIN_ENTER)) {
        LOG_DBG("Switch ENTER pressed");
		m_key_pressed = CHECK;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_BACK) && (pin == APP_PUSH_SWITCH_PIN_BACK)) {
        LOG_DBG("Switch BACK pressed");
		m_key_pressed = BACK;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_DOWN) && (pin == APP_PUSH_SWITCH_PIN_DOWN)) {
        LOG_DBG("Switch DOWN pressed");
		m_key_pressed = DOWN;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_POWER) && (pin == APP_PUSH_SWITCH_PIN_POWER)) {
        LOG_DBG("Switch POWER pressed");
		m_key_pressed = POWER;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_RIGHT) && (pin == APP_PUSH_SWITCH_PIN_RIGHT)) {
        LOG_DBG("Switch RIGHT pressed");
		m_key_pressed = RIGHT;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_LEFT) && (pin == APP_PUSH_SWITCH_PIN_LEFT)) {
        LOG_DBG("Switch LEFT pressed");
		m_key_pressed = LEFT;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_UP) && (pin == APP_PUSH_SWITCH_PIN_UP)) {
        LOG_DBG("Switch UP pressed");
		m_key_pressed = UP;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_HOME) && (pin == APP_PUSH_SWITCH_PIN_HOME)) {
        LOG_DBG("Switch HOME pressed");
		m_key_pressed = HOME;
		m_press_type = KEY_REL;
    }

/*	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_ENTER)) && (pin == BSP_MEMBR_SWITCH_PIN_ENTER)) {
		m_key_pressed = CHECK;
		m_press_type = KEY_REL;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_BACK)) && (pin == BSP_MEMBR_SWITCH_PIN_BACK)) {
		m_key_pressed = BACK;
		m_press_type = KEY_REL;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_LEFT)) && (pin == BSP_MEMBR_SWITCH_PIN_LEFT)) {
		m_key_pressed = LEFT;
		m_press_type = KEY_PR;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_RIGHT)) && (pin == BSP_MEMBR_SWITCH_PIN_RIGHT)) {
		m_key_pressed = RIGHT;
		m_press_type = KEY_PR;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_UP)) && (pin == BSP_MEMBR_SWITCH_PIN_UP)) {
		m_key_pressed = UP;
		m_press_type = KEY_PR;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_DOWN)) && (pin == BSP_MEMBR_SWITCH_PIN_DOWN)) {
		m_key_pressed = DOWN;
		m_press_type = KEY_PR;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_POWER)) && (pin == BSP_MEMBR_SWITCH_PIN_POWER)) {
		m_key_pressed = POWER;
		m_press_type = KEY_REL;
	}

	if ((!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_HOME)) && (pin == BSP_MEMBR_SWITCH_PIN_HOME)) {
		m_key_pressed = HOME;
		m_press_type = KEY_REL;
	}*/
#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_ENTER) && (pin == APP_PUSH_SWITCH_PIN_ENTER)) {
        LOG_INF("Switch ENTER pressed");
		m_key_pressed = CHECK;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_BACK) && (pin == APP_PUSH_SWITCH_PIN_BACK)) {
    	LOG_INF("Switch BACK pressed");
		m_key_pressed = BACK;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_DOWN) && (pin == APP_PUSH_SWITCH_PIN_DOWN)) {
    	LOG_INF("Switch DOWN pressed");
		m_key_pressed = DOWN;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_POWER) && (pin == APP_PUSH_SWITCH_PIN_POWER)) {
    	LOG_INF("Switch POWER pressed");
		m_key_pressed = POWER;
		m_press_type = KEY_REL;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_RIGHT) && (pin == APP_PUSH_SWITCH_PIN_RIGHT)) {
    	LOG_INF("Switch RIGHT pressed");
		m_key_pressed = RIGHT;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_LEFT) && (pin == APP_PUSH_SWITCH_PIN_LEFT)) {
    	LOG_INF("Switch LEFT pressed");
		m_key_pressed = LEFT;
		m_press_type = KEY_PR;
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_UP) && (pin == APP_PUSH_SWITCH_PIN_UP)) {
    	LOG_INF("Switch UP pressed");
		m_key_pressed = UP;
		m_press_type = KEY_PR;
    }

//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_CHECK)) {
//		m_key_pressed = CHECK;
//		m_press_type = KEY_REL;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_BACK)) {
//		m_key_pressed = BACK;
//		m_press_type = KEY_REL;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_LEFT)) {
//		m_key_pressed = LEFT;
//		m_press_type = KEY_PR;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_RIGHT)) {
//		m_key_pressed = RIGHT;
//		m_press_type = KEY_PR;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_UP)) {
//		m_key_pressed = UP;
//		m_press_type = KEY_PR;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_DOWN)) {
//		m_key_pressed = DOWN;
//		m_press_type = KEY_PR;
//	}
//
//	if (!strcmp(switch_label, BSP_MEMBR_SWITCH_LABEL_POWER)) {
//		m_key_pressed = POWER;
//		m_press_type = KEY_REL;
//	}

#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_POWER) && (pin == APP_PUSH_SWITCH_PIN_POWER)) {
        LOG_DBG("Switch POWER pressed");
		m_key_pressed = POWER;
		m_press_type = KEY_REL;
    }
#endif	/* CONFIG_BOARD_C204_CORE */

	/* reset dimmer level */
	m_dim.dimm_state = DISP_DIMM_STATE_DIM;
	m_dim.dimm_set_level = m_dim.bright_level;
	k_work_submit(&m_dim.dimm_worker);
	if (m_dim.dimm_secs > 0)
		k_timer_start(&m_dim.dimm_tmr, K_SECONDS(m_dim.dimm_secs), K_NO_WAIT);
}
#endif	/* (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH) */

static int setup_keypad() {
	int ret = 0;

	lv_indev_drv_init(&m_keypad_drv);      /* Basic initialization */

	m_keypad_drv.type = LV_INDEV_TYPE_KEYPAD;
	m_keypad_drv.read_cb = keypad_read;

#if (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_LIB_MATRIX_KEYPAD)
	m_key_pressed = NONE;
	m_press_type = UNKNOWN;

	/* register callback for all switches */
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
//	ret = bsp_membrane_callback_add(&key_enter_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_ENTER, BSP_MEMBR_SWITCH_PIN_ENTER);
//	ret |= bsp_membrane_callback_add(&key_back_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_BACK, BSP_MEMBR_SWITCH_PIN_BACK);
//	ret |= bsp_membrane_callback_add(&key_left_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_LEFT, BSP_MEMBR_SWITCH_PIN_LEFT);
//	ret |= bsp_membrane_callback_add(&key_right_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_RIGHT, BSP_MEMBR_SWITCH_PIN_RIGHT);
//	ret |= bsp_membrane_callback_add(&key_up_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_UP, BSP_MEMBR_SWITCH_PIN_UP);
//	ret |= bsp_membrane_callback_add(&key_down_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_DOWN, BSP_MEMBR_SWITCH_PIN_DOWN);
//	ret |= bsp_membrane_callback_add(&key_power_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_POWER, BSP_MEMBR_SWITCH_PIN_POWER);
//	ret |= bsp_membrane_callback_add(&key_home_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_HOME, BSP_MEMBR_SWITCH_PIN_HOME);

	ret = lib_push_switch_callback_add(&sw_enter_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER);
	ret |= lib_push_switch_callback_add(&sw_back_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_PIN_BACK);
	ret |= lib_push_switch_callback_add(&sw_down_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_PIN_DOWN);
	ret |= lib_push_switch_callback_add(&sw_pwr_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
	ret |= lib_push_switch_callback_add(&sw_right_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT);
	ret |= lib_push_switch_callback_add(&sw_left_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_PIN_LEFT);
	ret |= lib_push_switch_callback_add(&sw_up_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_PIN_UP);
	ret |= lib_push_switch_callback_add(&sw_home_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_HOME, APP_PUSH_SWITCH_PIN_HOME);

#elif (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
//	ret = bsp_membrane_callback_add(&key_check_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_CHECK, BSP_MEMBR_SWITCH_PIN_CHECK);
//	ret |= bsp_membrane_callback_add(&key_back_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_BACK, BSP_MEMBR_SWITCH_PIN_BACK);
//	ret |= bsp_membrane_callback_add(&key_left_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_LEFT, BSP_MEMBR_SWITCH_PIN_LEFT);
//	ret |= bsp_membrane_callback_add(&key_right_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_RIGHT, BSP_MEMBR_SWITCH_PIN_RIGHT);
//	ret |= bsp_membrane_callback_add(&key_up_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_UP, BSP_MEMBR_SWITCH_PIN_UP);
//	ret |= bsp_membrane_callback_add(&key_down_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_DOWN, BSP_MEMBR_SWITCH_PIN_DOWN);
//	ret |= bsp_membrane_callback_add(&key_power_data, keypad_app_cb_handler, BSP_MEMBR_SWITCH_LABEL_POWER, BSP_MEMBR_SWITCH_PIN_POWER);

	ret = lib_push_switch_callback_add(&sw_enter_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER);
	ret |= lib_push_switch_callback_add(&sw_back_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_PIN_BACK);
	ret |= lib_push_switch_callback_add(&sw_down_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_PIN_DOWN);
	ret |= lib_push_switch_callback_add(&sw_pwr_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
	ret |= lib_push_switch_callback_add(&sw_right_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT);
	ret |= lib_push_switch_callback_add(&sw_left_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_PIN_LEFT);
	ret |= lib_push_switch_callback_add(&sw_up_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_PIN_UP);

#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	ret |= lib_push_switch_callback_add(&sw_pwr_cb, keypad_app_cb_handler, APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
	lib_mk_callback_register(libmk_callback_handler);
#endif	/* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205) */
	if (ret != 0) {
		LOG_ERR("bsp_membrane_callback_add failed");
		return ret;
	}
#endif	/* (CONFIG_BSP_MEMBRANE_SWITCH || CONFIG_APP_PUSH_SWITCH || CONFIG_LIB_MATRIX_KEYPAD) */
	return ret;
}

static void display_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch(event) {
	case LIB_EVENT_SYSTEM_BOOTING_COMPLETE:
		m_boot_done = true;
		break;
	default:
		break;
	}
}

/*
 * *******************************************
 * GLOBAL Functions
 * *******************************************
 * */
lv_group_t * app_display_lvgl_group_instance_get() {
	return m_lvgl_grp;
}

void app_display_lvgl_group_set_current(lv_obj_t * obj) {
	if (m_lvgl_grp != NULL) {
		lv_group_remove_all_objs(m_lvgl_grp);
		lv_group_add_obj(m_lvgl_grp, obj);
	}
}

void app_display_key_cb_set(app_display_key_cb key_cb) {
	m_screen_key_cb = key_cb;
}

void app_display_spl_key_cb_set(app_display_key_cb spl_key_cb) {
	m_screen_spl_key_cb = spl_key_cb;
}

const struct device* app_display_device_get(void) {
	return DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
}

int app_display_init() {
	int ret = 0;

	/* initialize bsp */
	ret = bsp_display_init();

#if CONFIG_APP_DISPLAY_DIMMING_SUPPORT
	uint8_t bright_def = 100;
//	if (app_settings_load_single(SETTINGS_KEY_FULL_PERS_BRIGHT, &bright_def, sizeof(bright_def)) != 0) {
//		LOG_ERR("failed to load settings: %s", SETTINGS_KEY_FULL_PERS_BRIGHT);
//		bright_def = 100;
//	}
	/* turn display on and set brightness */
	ret = bsp_display_set_brightness(bright_def);
#else
	uint8_t bright_def = 0;
#endif

	/* Initialize display auto dimming timer and related variables */
	k_work_init(&m_dim.dimm_worker, dimm_worker);
	k_timer_init(&m_dim.dimm_tmr, dimm_timer_handler, NULL);
	m_dim.dimm_state = DISP_DIMM_STATE_DIM;
	m_dim.dimm_level = APP_DISPLAY_DIMMED_LEVEL;
	m_dim.bright_level = bright_def;
	uint32_t screenon_tm = 60*30;
//	if (app_settings_load_single(SETTINGS_KEY_FULL_PERS_SCREENON_TM, &screenon_tm, sizeof(screenon_tm)) != 0) {
//		LOG_ERR("failed to load settings: %s", SETTINGS_KEY_FULL_PERS_SCREENON_TM);
//		screenon_tm = 30;
//	}
	m_dim.off_secs = screenon_tm;
	m_dim.dimm_secs = -1;	// don't dim or turn off display

	/* Initialize a keypad and register it to LVGL for screen navigation */
	setup_keypad();
	m_keypad_indev = lv_indev_drv_register(&m_keypad_drv);	/* Register the keypad driver in LVGL and save the created input device object */
	m_lvgl_grp = lv_group_create();							/* Create a lvgl group and add the input device to the group */
	lv_indev_set_group(m_keypad_indev, m_lvgl_grp);

	/* initialize display menu data */
	app_display_menu_init();

	/* Initialize and start screens */
	c20x_screen_manager_init(m_lvgl_grp);

	/* display blanking on */
//	const struct device *disp_dev = device_get_binding(DISPLAY_DEV_NAME);
//	if (disp_dev == NULL) {
//		LOG_ERR("device %s not found!", DISPLAY_DEV_NAME);
//		return -1;
//	}
	const struct device *disp_dev = app_display_device_get();
	if (!device_is_ready(disp_dev)) {
		LOG_ERR("Display device not ready");
		return -1;
	}

	display_blanking_off(disp_dev);

	/* Start display refresh thread */
	app_display_refresh_tid = k_thread_create(&app_display_refresh_thread_data, app_display_refresh_stack,
			K_THREAD_STACK_SIZEOF(app_display_refresh_stack), app_display_refresh_thread, NULL, NULL, NULL,
			APP_THREAD_PRIO_DISPLAY_REFRESH, 0, K_NO_WAIT);
#if (CONFIG_THREAD_NAME)
	ret = k_thread_name_set(app_display_refresh_tid, APP_THREAD_NAME_DISPLAY_REFRESH);
#endif

#if CONFIG_APP_DISPLAY_DIMMING_SUPPORT
	/* turn display on and set brightness */
	ret = bsp_display_set_brightness(bright_def);
#else
#endif

	ret = lib_events_callback_add(&m_cb_boot_done, display_event_handler, LIB_EVENT_SYSTEM_BOOTING_COMPLETE);
	return ret;
}
