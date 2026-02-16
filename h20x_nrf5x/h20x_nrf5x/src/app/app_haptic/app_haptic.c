/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 8-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_haptic);

#if (CONFIG_APP_HAPTIC_FB_DRIVE_PWM)
#define HAPTIC_FB_NODE	DT_ALIAS(haptic_fb)
#if DT_NODE_HAS_STATUS(HAPTIC_FB_NODE, okay)
static const struct pwm_dt_spec hfb = PWM_DT_SPEC_GET(HAPTIC_FB_NODE);
#else
#error "Unsupported board: haptic-fb devicetree alias is not defined"
#endif
#define PWM_PERIOD 1000
#elif (CONFIG_APP_HAPTIC_FB_DRIVE_GPIO)
#define HAPTIC_FB_NODE	DT_NODELABEL(haptic_gpio)
static const struct gpio_dt_spec hfb = GPIO_DT_SPEC_GET(HAPTIC_FB_NODE, gpios);
// #define HAPTIC_FB_PIN	DT_GPIO_PIN(DT_NODELABEL(haptic_gpio), gpios)
// #define HAPTIC_FB_FLAGS	(GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(haptic_gpio), gpios))
#endif  /*(CONFIG_APP_HAPTIC_FB_DRIVE_PWM)*/


void app_haptic_on()
{
#if (CONFIG_APP_HAPTIC_FB_DRIVE_PWM)
    uint32_t pulse = (PWM_PERIOD * 0.5);
    pwm_set_dt(&hfb, PWM_USEC(PWM_PERIOD), PWM_USEC(pulse));
#elif (CONFIG_APP_HAPTIC_FB_DRIVE_GPIO)
    gpio_pin_set(hfb.port, hfb.pin, 1);   /* active high pin, 1 = HIGH, 0 = LOW */
#endif  /*(CONFIG_APP_HAPTIC_FB_DRIVE_PWM)*/
}

void app_haptic_off()
{
#if (CONFIG_APP_HAPTIC_FB_DRIVE_PWM)
    uint32_t pulse = 0; //(PWM_PERIOD * 0.5);
    pwm_set_dt(&hfb, PWM_USEC(PWM_PERIOD), PWM_USEC(pulse));
#elif (CONFIG_APP_HAPTIC_FB_DRIVE_GPIO)
    gpio_pin_set(hfb.port, hfb.pin, 0);   /* active high pin, 1 = HIGH, 0 = LOW */
#endif  /*(CONFIG_APP_HAPTIC_FB_DRIVE_PWM)*/
}

int app_haptic_init()
{
    int ret = 0;
#if (CONFIG_APP_HAPTIC_FB_DRIVE_PWM)
    if (!device_is_ready(hfb.dev)) {
		LOG_ERR("Error: haptic device %s is not ready\n", hfb.dev->name);
		return -1;
	}
    
    uint32_t pulse = 0; //(PWM_PERIOD * 0.5);
    pwm_set_dt(&hfb, PWM_USEC(PWM_PERIOD), PWM_USEC(pulse));
#elif (CONFIG_APP_HAPTIC_FB_DRIVE_GPIO)
    if (!device_is_ready(hfb.port)) {
        LOG_ERR("Error: haptic device %s is not ready\n", hfb.port->name);
        return -1;
    }
    ret = gpio_pin_configure(hfb.port, hfb.pin, (GPIO_OUTPUT | hfb.dt_flags));
#endif  /*(CONFIG_APP_HAPTIC_FB_DRIVE_PWM)*/
    return ret;
}