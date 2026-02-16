/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef APPLICATION_MODULES_BSP_FAN_H_
#define APPLICATION_MODULES_BSP_FAN_H_

#include <device.h>

typedef int (*bsp_fan_enable_t)(void);
typedef int (*bsp_fan_disable_t)(void);
typedef int (*bsp_fan_speed_ctrl_pwm_t)(const struct device *, uint8_t);
typedef int (*bsp_fan_speed_ctrl_i2c_t)(const struct device *, uint16_t);
typedef int (*bsp_fan_speed_fb_t)(void);
typedef uint32_t (*bsp_fan_freq_get_t)(const struct device *);

struct bsp_fan_api {
	bsp_fan_enable_t enable;
	bsp_fan_disable_t disable;
	bsp_fan_speed_ctrl_pwm_t speed_ctrl_pwm;
	bsp_fan_speed_ctrl_i2c_t speed_ctrl_i2c;
	bsp_fan_speed_fb_t speed_feedback;
	bsp_fan_freq_get_t fan_freq_get;
};

#endif /* APPLICATION_MODULES_BSP_FAN_H_ */
