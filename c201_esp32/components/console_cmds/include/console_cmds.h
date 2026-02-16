/*
 * Copyright (c) 2021 Acme CPU
 *
 * console_cmds.h
 * Created on: 23-Sep-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_CONSOLE_CMDS_INCLUDE_CONSOLE_CMDS_H_
#define COMPONENTS_CONSOLE_CMDS_INCLUDE_CONSOLE_CMDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief: 	Initializes the usart based command console and maintains an infinite loop tp processes
 * 			the commands
 * @return:	should not return
 * 			-ERRNO for failure
 * */
int console_cmds_init_and_loop();

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_CONSOLE_CMDS_INCLUDE_CONSOLE_CMDS_H_ */
