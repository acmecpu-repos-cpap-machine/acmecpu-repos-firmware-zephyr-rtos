/*
 * app_fluid_level.h
 *
 *  Created on: 30-Apr-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_FLUID_LEVEL_APP_FLUID_LEVEL_H_
#define SRC_INCLUDE_APP_FLUID_LEVEL_APP_FLUID_LEVEL_H_

/**
 * @brief Measures the fluid level
 * @param 	pval_raw_mm[out]	raw distance value in mm
 * @param 	pval_liquid_mm[out]	liquid level value in mm
 * @return 	0 for success
 * 			-ve for failure
 */
int app_fluid_level_mm(int *pval_raw_mm, int *pval_liquid_mm);
/**
 * @brief	Initializes the app_fluid_level module opens the file and fetches the information
 * @return	0 for success
 * 			-ve for failure
 */
int app_fluid_level_init();



#endif /* SRC_INCLUDE_APP_FLUID_LEVEL_APP_FLUID_LEVEL_H_ */
