/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 07-Sep-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef MODULES_LTC294X_LTC294X_H_
#define MODULES_LTC294X_LTC294X_H_

#include <zephyr/drivers/sensor.h>

#define SENSOR_ATTR_CHARGE_THR_HIGH		SENSOR_ATTR_PRIV_START + 1
#define SENSOR_ATTR_CHARGE_THR_LOW		SENSOR_ATTR_PRIV_START + 2
#define SENSOR_ATTR_CHARGE_VALUE_NOW	SENSOR_ATTR_PRIV_START + 3

#endif /* MODULES_LTC294X_LTC294X_H_ */
