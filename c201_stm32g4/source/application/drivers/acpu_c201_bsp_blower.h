/*
 * Copyright (c) 2021 Acme CPU
 */


#ifndef APPLICATION_MODULES_BSP_BLOWER_DRIVER_H_
#define APPLICATION_MODULES_BSP_BLOWER_DRIVER_H_

#include <stdint.h>

typedef int (*bsp_fan_enable_t)(const struct device *);
typedef int (*bsp_fan_disable_t)(const struct device *);
typedef int (*bsp_fan_voltage_ctrl_t)(const struct device *, uint16_t);
typedef uint16_t (*bsp_fan_voltage_fb_t)(const struct device *);
typedef int (*bsp_fan_pwm_ctrl_t)(const struct device *, uint16_t);
typedef int (*bsp_fan_dir_ctrl_t)(const struct device *, uint8_t);
typedef uint16_t (*bsp_fan_speed_get_t)(const struct device *);

struct bsp_blower_driver_api {
	bsp_fan_enable_t enable;
	bsp_fan_disable_t disable;
	bsp_fan_voltage_ctrl_t ps_ctrl;
	bsp_fan_voltage_fb_t ps_feedback;
	bsp_fan_pwm_ctrl_t pwm_ctrl;
	bsp_fan_dir_ctrl_t dir_ctrl;
	bsp_fan_speed_get_t speed_get;
};

#endif /* APPLICATION_MODULES_BSP_BLOWER_DRIVER_H_ */
