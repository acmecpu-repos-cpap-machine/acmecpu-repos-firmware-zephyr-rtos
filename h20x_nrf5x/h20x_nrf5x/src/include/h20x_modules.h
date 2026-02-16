/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 15-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_H20X_MODULES_H_
#define SRC_INCLUDE_H20X_MODULES_H_

// #include <zephyr.h>

#define SENSOR_PRESS_MB_LABEL		DT_PROP(DT_NODELABEL(press_mb), label)
#define SENSOR_IMU_MB_LABEL			DT_PROP(DT_NODELABEL(imu_mb), label)
#define SENSOR_IMU_AB_LABEL			DT_PROP(DT_NODELABEL(imu_ab), label)
#define SENSOR_HRM_LABEL            DT_PROP(DT_NODELABEL(sh_oxh), label)
#define SENSOR_EKG_LABEL            DT_PROP(DT_NODELABEL(ekg_rtor), label)

#define BATT_CHRG_LABEL 	        DT_PROP(DT_NODELABEL(batt_chrg), label)
#define SENSOR_FG_LABEL             DT_PROP(DT_NODELABEL(fuelgauge), label)

#endif /* SRC_INCLUDE_H20X_MODULES_H_ */