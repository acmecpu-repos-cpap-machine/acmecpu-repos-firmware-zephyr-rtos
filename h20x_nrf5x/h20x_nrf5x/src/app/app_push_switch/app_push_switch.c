/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 29-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
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
struct lib_push_switch_callback sw_pwr_cb;
struct lib_push_switch_callback sw_mic_cb;

static void switch_handler(struct lib_push_switch_callback *cb, uint32_t pin, LIB_PUSH_SWITCH_PRESSED_TYPE press_type)
{
    if ((cb->dev == APP_PUSH_SWITCH_DEVICE_POWER) && (pin == APP_PUSH_SWITCH_PIN_POWER)) {
        LOG_INF("Switch POWER pressed");
    } else if ((cb->dev == APP_PUSH_SWITCH_DEVICE_MIC) && (pin == APP_PUSH_SWITCH_PIN_MIC)) {
        LOG_INF("Switch MIC pressed");
    }
}

int app_push_switch_init()
{
    int ret=0;

    /* configure the switches */
    ret = lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER, APP_PUSH_SWITCH_FLAGS_POWER, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_CANCEL, APP_PUSH_SWITCH_PIN_CANCEL, APP_PUSH_SWITCH_FLAGS_CANCEL, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_HOME, APP_PUSH_SWITCH_PIN_HOME, APP_PUSH_SWITCH_FLAGS_HOME, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_NO, APP_PUSH_SWITCH_PIN_NO, APP_PUSH_SWITCH_FLAGS_NO, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_YES, APP_PUSH_SWITCH_PIN_YES, APP_PUSH_SWITCH_FLAGS_YES, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_ENTER, APP_PUSH_SWITCH_PIN_ENTER, APP_PUSH_SWITCH_FLAGS_ENTER, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_MIC, APP_PUSH_SWITCH_PIN_MIC, APP_PUSH_SWITCH_FLAGS_MIC, true);
    ret |= lib_push_switch_pin_configure(APP_PUSH_SWITCH_DEVICE_SAVE, APP_PUSH_SWITCH_PIN_SAVE, APP_PUSH_SWITCH_FLAGS_SAVE, true);

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

    /* add callbacks */
    lib_push_switch_callback_add(&sw_pwr_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_POWER, APP_PUSH_SWITCH_PIN_POWER);
    lib_push_switch_callback_add(&sw_mic_cb, switch_handler, APP_PUSH_SWITCH_DEVICE_MIC, APP_PUSH_SWITCH_PIN_MIC);

    return ret;
}