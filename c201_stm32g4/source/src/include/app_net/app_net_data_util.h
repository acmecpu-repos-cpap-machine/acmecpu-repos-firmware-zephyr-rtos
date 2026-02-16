/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 22-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_NET_APP_NET_DATA_UTIL_H_
#define SRC_INCLUDE_APP_NET_APP_NET_DATA_UTIL_H_

/**
 * @brief 	This function sends data to the network processor via uart_m2m_com
 * 			as a data request frame format.
 * 			See lib_m2m_frame and M2M_Command_Processor_Specs to learn about frames.
 * 			It waits for each acknowledgement from the receiver before transmitting again.
 *
 * @note	To understand the implementation, see
 * 				1. app_dfu_netproc_fw_send() and fw_bin_send_thread() in app_dfu_netproc.c
 * 				2. app_net_html_gen_and_send() and html_gen_and_send_thread() in app_net_html_gen.c
 *
 * @param frame_type[in] 		type of frame from enum UART_M2M_FRAME_TYPE
 * @param cmd[in]				the command id
 * @param data_buf[in]			buffer which holds the data to be send. This can be larger than te max payload size UART_M2M_PAYLOAD_SIZE_MAX
 * @param data_len[in]			size of the data buffer in bytes
 * @param tot_payload_sent[out]	number of bytes sent
 * @param last_sequence[out]	the last sequence number of the frame which was sent
 * @param lock[in]				semaphore lock used to wait for acknowledgement from the receiver
 * @return
 * 	0 on success, negative errno code on fail.
 */
int app_net_send_data_resp(int frame_type, int cmd, char *data_buf, int data_len,
		int *tot_payload_sent, uint32_t *last_sequence, struct k_sem *lock);

#endif /* SRC_INCLUDE_APP_NET_APP_NET_DATA_UTIL_H_ */
