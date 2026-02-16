/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_BSP_LED_BSP_LED_NTF_H_
#define SRC_INCLUDE_BSP_LED_BSP_LED_NTF_H_

#include <stdint.h>

#define BSP_LED_NTF_NORMAL			(0x01)
#define BSP_LED_NTF_BLINK			(0x02)
#define BSP_LED_NTF_BRIGHTNESS		(0x04)
#define BSP_LED_NTF_ALL				(0x07)

struct bsp_led_config {
	uint8_t ntf_type;
	uint8_t led_state;
	uint8_t brightness;
	uint32_t blink_delay_on_ms;
	uint32_t blink_delay_off_ms;
};

int bsp_led_show_notification(uint8_t led_idx, struct bsp_led_config *led_config);

#endif /* SRC_INCLUDE_BSP_LED_BSP_LED_NTF_H_ */
