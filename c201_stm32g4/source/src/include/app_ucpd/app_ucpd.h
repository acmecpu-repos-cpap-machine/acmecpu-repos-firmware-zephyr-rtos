/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UCPD_APP_UCPD_H_
#define SRC_INCLUDE_APP_UCPD_APP_UCPD_H_


/**
 * @brief This function checks if there is a change in the power supply
 * 		  i.e. the source has been attached. This function should be called
 * 		  in a loop
 *
 * @return
 * 	0 no change
 * 	1 change in source
 */
int app_ucpd_check_ps();

/**
 * @brief Initialize and starts the USB C Power Delivery stack
 * 		  also, registers necessary callback functions
 *
 * @return
 * 	0 SUCCESS
 * 	negative number for failure
 */
int app_ucpd_init(void);

#endif /* SRC_INCLUDE_APP_UCPD_APP_UCPD_H_ */
