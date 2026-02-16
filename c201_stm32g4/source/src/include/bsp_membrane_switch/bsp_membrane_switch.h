/*
 * Copyright (c) 2021 Acme CPU
 */



#ifndef SRC_INCLUDE_BSP_MEMBRANE_SWITCH_BSP_MEMBRANE_SWITCH_H_
#define SRC_INCLUDE_BSP_MEMBRANE_SWITCH_BSP_MEMBRANE_SWITCH_H_

#include <zephyr.h>
#include <device.h>
#include <sys/slist.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if CONFIG_BOARD_C204_CORE
#define BSP_MEMBR_MAX_SWITCH	8
#else
#define BSP_MEMBR_MAX_SWITCH	7
#endif

typedef enum {
	SWITCH_DEASSERTED = 0,				/* Switch is in released state */
	SWITCH_ASSERTED,					/* Switch is in pressed state */
	SWITCH_UNKNOWN,
} BSP_MEMBR_SWITCH_PRESS_STATUS;

typedef enum {
	SWITCH_PRESSED_NONE = 0,
	SWITCH_PRESSED_NORMAL = 100, 		/* switch press will be detected by level trigger interrupt, repeated action possible if user holds down the switch */
	SWITCH_PRESSED_NORMAL_EDGE, 		/* switch pressed will be detected by edge trigger interrupt, for single action only */
	SWITCH_PRESSED_LONG, 				/* switch press will be detected by level trigger interrupt if held down for more than LONG_PRESS_DURATION */
	SWITCH_PRESSED_NORMAL_EDGE_LONG, 	/* switch press can act as normal edge or long */
} BSP_MEMBR_SWITCH_PRESSED_TYPE;

#define BSP_MEMBR_SWITCH_EDGE_NORMAL_PRESS_DURATION_MIN		CONFIG_SWITCH_EDGE_NORMAL_PRESS_DURATION_MS_MIN		/* Duration in ms */
#define BSP_MEMBR_SWITCH_EDGE_LONG_PRESS_DURATION_MIN		CONFIG_SWITCH_EDGE_LONG_PRESS_DURATION_MS_MIN		/* Duration in ms */
#define BSP_MEMBR_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MIN	CONFIG_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MS_MIN	/* Duration in ms */
#define BSP_MEMBR_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX	CONFIG_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MS_MAX	/* Duration in ms */

/* Switch labels */
#if (CONFIG_BOARD_C204_CORE)
#define BSP_MEMBR_SWITCH_LABEL_ENTER	DT_GPIO_LABEL(DT_NODELABEL(switch_1), gpios)
#define BSP_MEMBR_SWITCH_LABEL_BACK		DT_GPIO_LABEL(DT_NODELABEL(switch_2), gpios)
#define BSP_MEMBR_SWITCH_LABEL_DOWN		DT_GPIO_LABEL(DT_NODELABEL(switch_3), gpios)
#define BSP_MEMBR_SWITCH_LABEL_POWER	DT_GPIO_LABEL(DT_NODELABEL(switch_4), gpios)
#define BSP_MEMBR_SWITCH_LABEL_RIGHT	DT_GPIO_LABEL(DT_NODELABEL(switch_5), gpios)
#define BSP_MEMBR_SWITCH_LABEL_LEFT		DT_GPIO_LABEL(DT_NODELABEL(switch_6), gpios)
#define BSP_MEMBR_SWITCH_LABEL_UP		DT_GPIO_LABEL(DT_NODELABEL(switch_7), gpios)
#define BSP_MEMBR_SWITCH_LABEL_HOME		DT_GPIO_LABEL(DT_NODELABEL(switch_8), gpios)

#define BSP_MEMBR_SWITCH_PIN_ENTER		DT_GPIO_PIN(DT_NODELABEL(switch_1), gpios)
#define BSP_MEMBR_SWITCH_PIN_BACK		DT_GPIO_PIN(DT_NODELABEL(switch_2), gpios)
#define BSP_MEMBR_SWITCH_PIN_DOWN		DT_GPIO_PIN(DT_NODELABEL(switch_3), gpios)
#define BSP_MEMBR_SWITCH_PIN_POWER		DT_GPIO_PIN(DT_NODELABEL(switch_4), gpios)
#define BSP_MEMBR_SWITCH_PIN_RIGHT		DT_GPIO_PIN(DT_NODELABEL(switch_5), gpios)
#define BSP_MEMBR_SWITCH_PIN_LEFT		DT_GPIO_PIN(DT_NODELABEL(switch_6), gpios)
#define BSP_MEMBR_SWITCH_PIN_UP			DT_GPIO_PIN(DT_NODELABEL(switch_7), gpios)
#define BSP_MEMBR_SWITCH_PIN_HOME		DT_GPIO_PIN(DT_NODELABEL(switch_8), gpios)

#define BSP_MEMBR_SWITCH_FLAGS_ENTER	(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_1), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_BACK		(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_2), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_DOWN		(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_3), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_POWER	(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_4), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_RIGHT	(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_5), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_LEFT		(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_6), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_UP		(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_7), gpios))
#define BSP_MEMBR_SWITCH_FLAGS_HOME		(GPIO_INPUT | DT_GPIO_FLAGS(DT_NODELABEL(switch_8), gpios))

#define BSP_MEMBR_SWITCH_MASK_ENTER		(1 << BSP_MEMBR_SWITCH_PIN_ENTER)
#define BSP_MEMBR_SWITCH_MASK_BACK		(1 << BSP_MEMBR_SWITCH_PIN_BACK)
#define BSP_MEMBR_SWITCH_MASK_LEFT		(1 << BSP_MEMBR_SWITCH_PIN_LEFT)
#define BSP_MEMBR_SWITCH_MASK_POWER		(1 << BSP_MEMBR_SWITCH_PIN_POWER)
#define BSP_MEMBR_SWITCH_MASK_RIGHT		(1 << BSP_MEMBR_SWITCH_PIN_RIGHT)
#define BSP_MEMBR_SWITCH_MASK_DOWN		(1 << BSP_MEMBR_SWITCH_PIN_DOWN)
#define BSP_MEMBR_SWITCH_MASK_UP		(1 << BSP_MEMBR_SWITCH_PIN_UP)
#define BSP_MEMBR_SWITCH_MASK_HOME		(1 << BSP_MEMBR_SWITCH_PIN_HOME)

#else

#define BSP_MEMBR_SWITCH_LABEL_CHECK	"CHECK_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_BACK		"BACK_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_LEFT		"LEFT_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_RIGHT	"RIGHT_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_UP		"UP_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_DOWN		"DOWN_SWITCH"
#define BSP_MEMBR_SWITCH_LABEL_POWER	"POWER_SWITCH"

/* TODO remove hard coded pin numbers and get them from the device tree */
#define BSP_MEMBR_SWITCH_PIN_CHECK		8 //DT_GPIO_PIN(DT_NODELABEL(CHECK_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_BACK		9 //DT_GPIO_PIN(DT_NODELABEL(BACK_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_LEFT		10 //DT_GPIO_PIN(DT_NODELABEL(LEFT_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_RIGHT		12 //DT_GPIO_PIN(DT_NODELABEL(RIGHT_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_UP			14 //DT_GPIO_PIN(DT_NODELABEL(UP_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_DOWN		13 //DT_GPIO_PIN(DT_NODELABEL(DOWN_SWITCH), gpios)
#define BSP_MEMBR_SWITCH_PIN_POWER		11 //DT_GPIO_PIN(DT_NODELABEL(POWER_SWITCH), gpios)

#define BSP_MEMBR_SWITCH_MASK_CHECK		(1 << BSP_MEMBR_SWITCH_PIN_CHECK)
#define BSP_MEMBR_SWITCH_MASK_BACK		(1 << BSP_MEMBR_SWITCH_PIN_BACK)
#define BSP_MEMBR_SWITCH_MASK_LEFT		(1 << BSP_MEMBR_SWITCH_PIN_LEFT)
#define BSP_MEMBR_SWITCH_MASK_RIGHT		(1 << BSP_MEMBR_SWITCH_PIN_RIGHT)
#define BSP_MEMBR_SWITCH_MASK_UP		(1 << BSP_MEMBR_SWITCH_PIN_UP)
#define BSP_MEMBR_SWITCH_MASK_DOWN		(1 << BSP_MEMBR_SWITCH_PIN_DOWN)
#define BSP_MEMBR_SWITCH_MASK_POWER		(1 << BSP_MEMBR_SWITCH_PIN_POWER)

#endif	/* CONFIG_BOARD_C204_CORE */

struct bsp_membr_callback;

typedef void (*bsp_membr_callback_handler_t)(struct bsp_membr_callback *cb,
		const char *switch_label, uint32_t pin, BSP_MEMBR_SWITCH_PRESSED_TYPE press_type);

/**
 * @brief BSP membrane switch callback structure
 *
 * Used to register a callback in the bsp callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct bsp_membr_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct bsp_membr_callback {
	/* This is meant to be used in the BSP and the user should not mess with it */
	sys_snode_t node;

	/* Actual callback function being called when relevant. */
	bsp_membr_callback_handler_t handler;

	/* Name of the switch */
	const char *switch_label;

	/* Pin number of switch */
	uint32_t pin;
};

static inline int bsp_membr_manage_callback(sys_slist_t *callbacks,
					struct bsp_membr_callback *callback,
					bool set)
{
	__ASSERT(callback, "No callback!");
	__ASSERT(callback->handler, "No callback handler!");

	if (!sys_slist_is_empty(callbacks)) {
		if (!sys_slist_find_and_remove(callbacks, &callback->node)) {
			if (!set) {
				return -EINVAL;
			}
		}
	}

	if (set) {
		sys_slist_prepend(callbacks, &callback->node);
	}

	return 0;
}

static inline void bsp_membr_fire_callbacks(sys_slist_t *list,
					const char *switch_label, uint32_t pin,
					BSP_MEMBR_SWITCH_PRESSED_TYPE press_type)
{
	struct bsp_membr_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		if ((!strcmp(cb->switch_label, switch_label)) && (cb->pin == pin)) {
			if (cb->handler != NULL) {
//				__ASSERT(cb->handler, "No callback handler!");
				cb->handler(cb, cb->switch_label, cb->pin, press_type);
			}
		}
	}
}

/* Function declarations */
int bsp_membrane_callback_add(struct bsp_membr_callback *cb_data,
		bsp_membr_callback_handler_t cb_handler, const char *switch_label, uint32_t pin);

int bsp_membrane_callback_remove(struct bsp_membr_callback *cb_data,
		bsp_membr_callback_handler_t cb_handler, const char *switch_label, uint32_t pin);

BSP_MEMBR_SWITCH_PRESS_STATUS bsp_membrane_switch_state_get(const char *switch_name, int pin);

void bsp_membrane_switch_poll_handler(const struct device *dev, const char *sw_label, uint32_t pins);

int bsp_membrane_switch_init();

#endif /* SRC_INCLUDE_BSP_MEMBRANE_SWITCH_BSP_MEMBRANE_SWITCH_H_ */
