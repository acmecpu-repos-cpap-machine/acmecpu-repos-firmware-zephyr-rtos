/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Aug-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UART_M2M_COM_APP_BLDC_DRV_APP_BLDC_DRV_CMDS_H_
#define SRC_INCLUDE_APP_UART_M2M_COM_APP_BLDC_DRV_APP_BLDC_DRV_CMDS_H_

//#include "../app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include <stdint.h>

/**
 * @brief: 	Process an incoming frame from the BLDC motor driver
 * @note:	The frame type can be request, response or stream
 *
 * @param:	in_frame	input frame
 * 			out_frame 	output frame
 *
 * @return:	0 Success when input frame type is request
 * 			1 Success when input frame type is response
 * 			2 Success when input frame type is stream
 * 			negative number for other errors
 * */
int app_bldc_drv_cmd_process(struct m2m_frame_t *in_frame, struct m2m_frame_t *out_frame);

/**
 * @brief: 	Sends commands to the BLDC motor driver over uart interface
 *
 * @param:	in_frame	input frame
 * 			out_frame 	output frame
 *
 * @return:	0 Success when input frame type is request
 * 			1 Success when input frame type is response
 * 			2 Success when input frame type is stream
 * 			negative number for other errors
 * */
int app_bldc_drv_cmd_send(void *data, size_t len);

/**
 * @brief: 	Initialize
 *
 * */
void app_bldc_drv_cmd_init();

int64_t app_bldc_drv_exe_time_get();

#endif /* SRC_INCLUDE_APP_UART_M2M_COM_APP_BLDC_DRV_APP_BLDC_DRV_CMDS_H_ */
