/*
 * app_imu_algo.h
 *
 *  Created on: 17-Jul-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_IMU_ALGO_APP_IMU_ALGO_H_
#define SRC_INCLUDE_APP_IMU_ALGO_APP_IMU_ALGO_H_

#include <stdint.h>

typedef enum bmi_algo {
    ANY_MOTION,
	G_DETECTION,
	ORIENT_DETECTION,
	STEP_COUNT
} BMI_DETECTION;

/**
 * @brief	Switch case for various algo such as any motion, g_detection, orient detection and step count
 * @return	0 for success
 * 			-ve for failure
 */
int app_imu_algo (BMI_DETECTION bmi_algo);

#endif /* SRC_INCLUDE_APP_IMU_ALGO_APP_IMU_ALGO_H_ */
