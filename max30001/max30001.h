/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 11-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef MODULES_MAX30001_MAX30001_H_
#define MODULES_MAX30001_MAX30001_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include "max30001_regs.h"

#define MAX30001_SPI_OPERATION  (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

enum sensor_attribute_max30001 {
    SENSOR_ATTR_MAX30001_SW_RESET = SENSOR_ATTR_PRIV_START + 1,
    SENSOR_ATTR_MAX30001_ECG_INIT_START,
    SENSOR_ATTR_MAX30001_RTOR_INIT_START,
    SENSOR_ATTR_MAX30001_INT_ASSIGN,
    SENSOR_ATTR_MAX30001_SYNCH,
    SENSOR_ATTR_MAX30001_RBIAS_FMSTR_INIT,
    SENSOR_ATTR_MAX30001_CALLBACK,
};

enum sensor_channel_max30001 {
    SENSOR_CHAN_MAX30001_EKG_RTOR = SENSOR_CHAN_PRIV_START + 1,
};

#endif /* MODULES_MAX30001_MAX30001_H_ */