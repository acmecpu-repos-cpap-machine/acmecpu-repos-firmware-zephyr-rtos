/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 17-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdio.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>
#include <string.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_push_switch);

#include "app_push_switch/app_push_switch.h"
#include "lib_push_switch/lib_push_switch.h"

static struct push_switch_data m_psw_data[APP_MAX_PUSH_SWITCH] = APP_PUSH_SWITCH_PRESS_ACTION_INIT_DATA;
#if PUSH_SWITCH_TEST
struct lib_push_switch_callback sw_enter_cb;
struct lib_push_switch_callback sw_back_cb;
struct lib_push_switch_callback sw_down_cb;
struct lib_push_switch_callback sw_pwr_cb;
struct lib_push_switch_callback sw_right_cb;
struct lib_push_switch_callback sw_left_cb;
struct lib_push_switch_callback sw_up_cb;
struct lib_push_switch_callback sw_home_cb;

static void switch_handler(struct lib_push_switch_callback *cb, uint32_t pin, LIB_PUSH_SWITCH_PRESSED_TYPE press_type)
{
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_ENTER) && (pin == APP_PUSH_SWITCH_PIN_ENTER)) {
        LOG_INF("Switch ENTER pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_BACK) && (pin == APP_PUSH_SWITCH_PIN_BACK)) {
        LOG_INF("Switch BACK pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_DOWN) && (pin == APP_PUSH_SWITCH_PIN_DOWN)) {
        LOG_INF("Switch DOWN pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_POWER) && (pin == APP_PUSH_SWITCH_PIN_POWER)) {
        LOG_INF("Switch POWER pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_RIGHT) && (pin == APP_PUSH_SWITCH_PIN_RIGHT)) {
        LOG_INF("Switch RIGHT pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_LEFT) && (pin == APP_PUSH_SWITCH_PIN_LEFT)) {
        LOG_INF("Switch LEF pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_UP) && (pin == APP_PUSH_SWITCH_PIN_UP)) {
        LOG_INF("Switch UP pressed");
    }
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_HOME) && (pin == APP_PUSH_SWITCH_PIN_HOME)) {
        LOG_INF("Switch HOME pressed");
    }
}
#endif

int app_push_switch_state_get_power_key()
{
	return lib_push_switch_state_get(APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
}

int app_push_switch_init()
{
    int ret=0;

#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
    /* configure the switches */
    ret = lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER, APP_PUSH_SWITCH_FLAGS_ENTER, APP_PUSH_SWITCH_HASINTR_ENTER);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_PIN_BACK, APP_PUSH_SWITCH_FLAGS_BACK, APP_PUSH_SWITCH_HASINTR_BACK);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_PIN_DOWN, APP_PUSH_SWITCH_FLAGS_DOWN, APP_PUSH_SWITCH_HASINTR_DOWN);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER, APP_PUSH_SWITCH_FLAGS_POWER, APP_PUSH_SWITCH_HASINTR_POWER);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT, APP_PUSH_SWITCH_FLAGS_RIGHT, APP_PUSH_SWITCH_HASINTR_RIGHT);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_PIN_LEFT, APP_PUSH_SWITCH_FLAGS_LEFT, APP_PUSH_SWITCH_HASINTR_LEFT);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_PIN_UP, APP_PUSH_SWITCH_FLAGS_UP, APP_PUSH_SWITCH_HASINTR_UP);
//    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_HOME, APP_PUSH_SWITCH_PIN_HOME, APP_PUSH_SWITCH_FLAGS_HOME, APP_PUSH_SWITCH_HASINTR_HOME);
#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
    ret = lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER, APP_PUSH_SWITCH_FLAGS_POWER, APP_PUSH_SWITCH_HASINTR_POWER);
#endif

    if (ret != 0) {
        LOG_ERR("lib_push_switch_pin_configure failed!");
        return ret;
    }

    /* initalize the push switch library */
    ret = lib_push_switch_init(m_psw_data);
    if (ret != 0) {
        LOG_ERR("lib_push_switch_init failed!");
        return ret;
    }

#if PUSH_SWITCH_TEST
    /* add callbacks */
    lib_push_switch_callback_add(&sw_enter_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER);
    lib_push_switch_callback_add(&sw_back_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_BACK, APP_PUSH_SWITCH_PIN_BACK);
    lib_push_switch_callback_add(&sw_down_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_DOWN, APP_PUSH_SWITCH_PIN_DOWN);
    lib_push_switch_callback_add(&sw_pwr_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
    lib_push_switch_callback_add(&sw_right_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_RIGHT, APP_PUSH_SWITCH_PIN_RIGHT);
    lib_push_switch_callback_add(&sw_left_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_LEFT, APP_PUSH_SWITCH_PIN_LEFT);
    lib_push_switch_callback_add(&sw_up_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_UP, APP_PUSH_SWITCH_PIN_UP);
#endif
    return ret;
}
