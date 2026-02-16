/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jul-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef __UART_M2M_COMM_MC_CMDS_H
#define __UART_M2M_COMM_MC_CMDS_H

#ifdef __cplusplus
extern "C" {
#endif

/* M2M command IDs */
#define M2M_CMD_ID_DO_SW_RST			1000
#define M2M_CMD_ID_COMM_CHK				1001

#define M2M_CMD_ID_BLOWER_STATE			18
#define M2M_CMD_ID_BLOWER_VOLT_MV		19
#define M2M_CMD_ID_BLOWER_SPEED_RPM		20
#define M2M_CMD_ID_BLOWER_DUTY			21
#define M2M_CMD_ID_BLOWER_RUNSTAT		22
#define M2M_CMD_ID_BLOWER_FLTACK		23
#define M2M_CMD_ID_BLOWER_SPEED_RAMP	24
#define M2M_CMD_ID_BLOWER_POWER			25


#define M2M_CMD_PAYLOAD_DELIM			','
#define M2M_CMD_PAYLOAD_TERM			'\n'
#define M2M_CMD_PAYLOAD_GET_CHAR		'?'

#define M2M_CMD_RESP_OK					"OK"
#define M2M_CMD_RESP_ERR				"ERR"


#define BLOWER_OFF	0
#define BLOWER_ON	1

#ifdef __cplusplus
}
#endif

#endif /* __UART_M2M_COMM_FRAME_H */
