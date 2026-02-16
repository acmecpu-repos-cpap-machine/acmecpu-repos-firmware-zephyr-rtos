/*
 * Copyright (c) 2022 Acme CPU
 *
 * host_cmds_uart_config.h
 * Created on: 20-Sep-2022
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_HOST_CMDS_HOST_CMDS_UART_CONFIG_H_
#define COMPONENTS_HOST_CMDS_HOST_CMDS_UART_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif


/*
 * #define CONFIG_HOST_IF_UART_NUMBER 2
#define CONFIG_HW_FLOWCTRL_CTS_RTS 1
#define CONFIG_HOST_IF_UART_RX_PIN 16
#define CONFIG_HOST_IF_UART_TX_PIN 17
#define CONFIG_HOST_IF_UART_RTS_PIN 33
#define CONFIG_HOST_IF_UART_CTS_PIN 35
 * */

#if CONFIG_BOARD_C201
#define UART_NUM		2

#define HW_FLOWCTRL_DISABLE 0
#define HW_FLOWCTRL_RTS 0
#define HW_FLOWCTRL_CTS 0
#define HW_FLOWCTRL_CTS_RTS 1

#define TXD_PIN 		17
#define RXD_PIN 		16
#define RTS_PIN 		33
#define CTS_PIN 		35

#elif CONFIG_BOARD_C204

#define UART_NUM		1

#define HW_FLOWCTRL_DISABLE 0
#define HW_FLOWCTRL_RTS 0
#define HW_FLOWCTRL_CTS 0
#define HW_FLOWCTRL_CTS_RTS 1

#define TXD_PIN 		26
#define RXD_PIN 		25
#define RTS_PIN 		14
#define CTS_PIN 		27

#elif CONFIG_BOARD_E206

#define UART_NUM		1

#define HW_FLOWCTRL_DISABLE 0
#define HW_FLOWCTRL_RTS 0
#define HW_FLOWCTRL_CTS 0
#define HW_FLOWCTRL_CTS_RTS 1

#define TXD_PIN 		26
#define RXD_PIN 		25
#define RTS_PIN 		14
#define CTS_PIN 		27

#elif CONFIG_BOARD_H205C

#define UART_NUM		1

#define HW_FLOWCTRL_DISABLE 0
#define HW_FLOWCTRL_RTS 0
#define HW_FLOWCTRL_CTS 0
#define HW_FLOWCTRL_CTS_RTS 1

#define TXD_PIN 		18
#define RXD_PIN 		17
#define RTS_PIN 		20
#define CTS_PIN 		19

#endif

#define RX_BUF_SIZE		4096
#define RETRY_COUNT		10

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_HOST_CMDS_HOST_CMDS_UART_CONFIG_H_ */
