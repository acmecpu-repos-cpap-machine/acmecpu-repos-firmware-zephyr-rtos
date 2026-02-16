/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 25-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef MODULES_STUSB4500_STUSB4500_I2C_H_
#define MODULES_STUSB4500_STUSB4500_I2C_H_

#include <stdint.h>
#include "stusb4500.h"

int stusb4500_read_regs(const struct stusb4500_config *config, uint8_t reg, uint8_t *data, uint16_t length);
int stusb4500_write_regs(const struct stusb4500_config *config, uint8_t reg, uint8_t *data, uint16_t length);
int stusb4500_update_bits(const struct stusb4500_config *config, uint8_t reg, uint8_t mask, uint8_t val);
int stusb4500_alert_pin_read(const struct stusb4500_config *config);
int stusb4500_reset_pin_set(const struct stusb4500_config *config);
int stusb4500_reset_pin_clear(const struct stusb4500_config *config);



#endif /* MODULES_STUSB4500_STUSB4500_I2C_H_ */
