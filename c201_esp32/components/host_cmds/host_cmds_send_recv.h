/*
 * Copyright (c) 2021 Acme CPU
 *
 * host_cmds_send_recv.h
 * Created on: 28-Apr-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_HOST_CMDS_SEND_RECV_H_
#define COMPONENTS_HOST_CMDS_HOST_CMDS_SEND_RECV_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HOST_CMDS_RECEIVE_TASK_STACK		(1024*8)
#define HOST_CMDS_REQ_SERVICE_TASK_STACK	(1024*4)
#define HOST_CMDS_CBFIRE_TASK_STACK		(1024*8)

#define HOST_CMDS_RECEIVE_TASK_PRIO			(5)
#define HOST_CMDS_REQ_SERVICE_TASK_PRIO		(6)
#define HOST_CMDS_CBFIRE_TASK_PRIO		(6)

typedef enum {
	SEND_RECV_MODE_CMD,
	SEND_RECV_MODE_DFU,
	SEND_RECV_MODE_STREAM,

	MODE_MAX
} HOST_CMDS_SEND_RECV_MODE;

int host_cmds_send_recv_init();
int host_cmds_send_recv_start();

int host_cmds_send_recv_init_for_cmd();
int host_cmds_send_recv_reinit_for_cmd();
//int host_cmds_send_recv_reinit_for_dfu();
int host_cmds_send_recv_reinit_for_dfu(int baud_rate, int data_bits, int parity, int stop_bits, int flow_ctrl, int source_clk);
void host_cmds_send_recv_set_mode(HOST_CMDS_SEND_RECV_MODE mode);
int host_cmd_send_rev_dfu_check();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_HOST_CMDS_SEND_RECV_H_ */
