/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef __UART_M2M_COMM_CONFIG_H
#define __UART_M2M_COMM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_UART_M2M_BUFFER_SIZE		32

#define DEF_RAMP_SPEED_01HZ				(-1700) 	//01Hz (-10200 rpm / 6)
#define DEF_RAMP_DURATION_MS			(3000)	// ms


#ifdef __cplusplus
}
#endif

#endif /* __UART_M2M_COMM_CONFIG_H */
