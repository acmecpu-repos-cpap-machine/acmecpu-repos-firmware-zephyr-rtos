/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 25-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_STUSB4500_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(stusb4500);

#include "stusb4500.h"

int stusb4500_read_regs(const struct stusb4500_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	return i2c_burst_read_dt(&config->i2c_bus, reg, data, length);
}

int stusb4500_write_regs(const struct stusb4500_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	return i2c_burst_write_dt(&config->i2c_bus, reg, data, length);
}

int stusb4500_update_bits(const struct stusb4500_config *config, uint8_t reg, uint8_t mask, uint8_t val)
{
    int ret=0;
    uint8_t rw_val=0;

    /* check bits */
    ret = i2c_burst_read_dt(&config->i2c_bus, reg, &rw_val, 1);
    if (ret == 0) {
        if ((rw_val & mask) == val) {
            // no need to update
            ret = 0;
        } else {
            // update bits
            rw_val = (rw_val & ~mask) | (val & mask);
            ret = i2c_burst_write_dt(&config->i2c_bus, reg, &rw_val, 1);
            if (ret == 0) {
                LOG_INF("write [0x%x] 0x%x", reg, rw_val);
            }
        }
    }
    return ret;
}

int stusb4500_alert_pin_read(const struct stusb4500_config *config)
{
	return gpio_pin_get_dt(&config->alert_gpio);
}

int stusb4500_reset_pin_set(const struct stusb4500_config *config)
{
	return gpio_pin_set_dt(&config->reset_gpio, HIGH);
}

int stusb4500_reset_pin_clear(const struct stusb4500_config *config)
{
	return gpio_pin_set_dt(&config->reset_gpio, LOW);
}


