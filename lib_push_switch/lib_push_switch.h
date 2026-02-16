/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 29-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#ifndef SRC_LIB_LIB_PUSH_SWITCH_LIB_PUSH_SWITCH_H_
#define SRC_LIB_LIB_PUSH_SWITCH_LIB_PUSH_SWITCH_H_

#include <zephyr/device.h>
#include <zephyr/sys/slist.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define LIB_PUSH_SWITCH_MAX_NUM			CONFIG_PUSH_SWITCH_MAX_COUNT
#define SWITCH_ASSERTED_PHY_LEVEL		CONFIG_SWITCH_ASSERTED_PHY_LEVEL

typedef enum {
	SWITCH_DEASSERTED = 0,				/* Switch is in released state */
	SWITCH_ASSERTED,					/* Switch is in pressed state */
	SWITCH_UNKNOWN,
} LIB_PUSH_SWITCH_PRESS_STATUS;

typedef enum {
	SWITCH_PRESSED_NONE = 0,
	SWITCH_PRESSED_NORMAL = 100, 		/* switch press will be detected by level trigger interrupt, repeated action possible if user holds down the switch */
	SWITCH_PRESSED_NORMAL_EDGE, 		/* switch pressed will be detected by edge trigger interrupt, for single action only */
	SWITCH_PRESSED_LONG, 				/* switch press will be detected by level trigger interrupt if held down for more than LONG_PRESS_DURATION */
	SWITCH_PRESSED_NORMAL_EDGE_LONG, 	/* switch press can act as normal edge or long */
} LIB_PUSH_SWITCH_PRESSED_TYPE;

#define LIB_PUSH_SWITCH_EDGE_NORMAL_PRESS_DURATION_MIN		CONFIG_SWITCH_EDGE_NORMAL_PRESS_DURATION_MS_MIN		/* Duration in ms */
#define LIB_PUSH_SWITCH_EDGE_LONG_PRESS_DURATION_MIN		CONFIG_SWITCH_EDGE_LONG_PRESS_DURATION_MS_MIN		/* Duration in ms */
#define LIB_PUSH_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MIN		CONFIG_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MS_MIN	/* Duration in ms */
#define LIB_PUSH_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MAX		CONFIG_SWITCH_LEVEL_NORMAL_PRESS_DURATION_MS_MAX	/* Duration in ms */

struct push_switch_data {
	const struct device *dev;
	uint32_t asserted_switch_mask;
	LIB_PUSH_SWITCH_PRESSED_TYPE press_type;
	bool detection_continue;
	int64_t pressed_start;
//	const char* asserted_switch_label;
	int asserted_switch_pin;
	struct k_work interrupt_worker;
};

struct lib_push_switch_callback;

typedef void (*lib_push_switch_callback_handler_t)(struct lib_push_switch_callback *cb,
				uint32_t pin, LIB_PUSH_SWITCH_PRESSED_TYPE press_type);

/**
 * @brief Push switch callback structure
 *
 * Used to register a callback in the bsp callback list.
 * As many callbacks as needed can be added as long as each of them
 * are unique pointers of struct lib_push_switch_callback.
 * Beware such structure should not be allocated on stack.
 *
 * Note: To help setting it, see gpio_init_callback() below
 */
struct lib_push_switch_callback {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	/* Actual callback function being called when relevant. */
	lib_push_switch_callback_handler_t handler;

	/* Device reference */
	const struct device *dev;

	/* Name of the switch */
//	const char *switch_label;

	/* Pin number of switch */
	uint32_t pin;
};

static inline int bsp_membr_manage_callback(sys_slist_t *callbacks,
					struct lib_push_switch_callback *callback,
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
					const struct device *dev,
					uint32_t pin,
					LIB_PUSH_SWITCH_PRESSED_TYPE press_type)
{
	struct lib_push_switch_callback *cb, *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, cb, tmp, node) {
		// if ((!strcmp(cb->switch_label, switch_label)) && (cb->pin == pin)) {
		if ((cb->dev == dev) && (cb->pin == pin)) {
			if (cb->handler != NULL) {
//				__ASSERT(cb->handler, "No callback handler!");
				cb->handler(cb, cb->pin, press_type);
			}
		}
	}
}

/* Function declarations */
int lib_push_switch_callback_add(struct lib_push_switch_callback *cb_data,
		lib_push_switch_callback_handler_t cb_handler, const struct device *dev, uint32_t pin);

int lib_push_switch_callback_remove(struct lib_push_switch_callback *cb_data,
		lib_push_switch_callback_handler_t cb_handler, const struct device *dev, uint32_t pin);

/**
 * @brief		Returns the state of a switch.
 * 				Also see CONFIG_SWITCH_ASSERTED_PHY_LEVEL
 * @param dev	device pointer of the gpio
 * @param pin	pin number of the gpio device
 * @return
 * 			SWITCH_DEASSERTED
 * 			SWITCH_ASSERTED
 */
LIB_PUSH_SWITCH_PRESS_STATUS lib_push_switch_state_get(const struct device *dev, int pin);

void lib_push_switch_poll_handler(const struct device *dev, uint32_t switch_pin_mask, uint32_t switch_pin);

int lib_push_switch_pin_configure(const struct device *dev, int pin, int flags, bool has_intr);

int lib_push_switch_init();

#endif /* SRC_LIB_LIB_PUSH_SWITCH_LIB_PUSH_SWITCH_H_ */
