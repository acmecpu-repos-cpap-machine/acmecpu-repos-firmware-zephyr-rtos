/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 7-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_ANALOG_APP_ANALOG_H_
#define SRC_INCLUDE_APP_ANALOG_APP_ANALOG_H_

#include <stdint.h>

/**
 * @brief
 *      Gets the VBAT voltage in milli volts
 * 		
 * @param
 * 		int32_t[out]    output milli volt value as a reference
 *
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_analog_vbat_mv_get(int32_t *pval_mv);

/**
 * @brief
 *      Gets the VBUS voltage in milli volts
 * 		
 * @param
 * 		int32_t[out]    output milli volt value as a reference
 *
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_analog_vbus_mv_get(int32_t *pval_mv);

/**
 * @brief
 *      enables analog voltage measurement
 * 		
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_analog_measure_en();

/**
 * @brief
 *      disables analog voltage measurement
 * 		
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_analog_measure_dis();

/**
 * @brief
 *      Initialize all ADC channels as mentiioned in the device tree
 * 		
 * @return
 * 		0 success
 * 		negative number for failure
 */
int app_analog_init();

#endif  /*SRC_INCLUDE_APP_ANALOG_APP_ANALOG_H_*/