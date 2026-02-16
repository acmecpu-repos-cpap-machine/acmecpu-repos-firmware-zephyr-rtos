/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 22-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DFU_APP_DFU_NETPROC_UPDATE_H_
#define SRC_APP_APP_DFU_APP_DFU_NETPROC_UPDATE_H_

/**
 * @brief	Sends C20X_M2M_CMD_NET_FWAPP_AVAIL command to the network processor
 * 			to intimate that an updated firmware app is available and the
 * 			network processor can get it by sending a C20X_M2M_CMD_NET_FWAPP_GET
 * 			command
 * @return
 * 0		Success
 * -1		Failure
 */
int app_dfu_netproc_fw_available_send();

/**
 * @brief	Sends C20X_M2M_CMD_NET_FWAPP_UPDATE command to the network processor
 * 			to reboot and update itself
 * @return
 * 0		Success
 * -1		Failure
 */
int app_dfu_netproc_fw_upgrade();

/**
 * @brief	This function is called when a C20X_M2M_CMD_NET_FWAPP_GET command
 * 			is received. It starts a thread which sends the network processor's
 * 			firmware app binary data in chunks
 * @param cmd	Must be C20X_M2M_CMD_NET_FWAPP_GET
 * @return
 * 	0		Success
 * 	-1		Failure
 */
int app_dfu_netproc_fw_send(uint32_t cmd);

#endif /* SRC_APP_APP_DFU_APP_DFU_NETPROC_UPDATE_H_ */
