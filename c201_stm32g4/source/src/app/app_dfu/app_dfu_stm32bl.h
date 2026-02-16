/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 4-Jul-2024
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_APP_APP_DFU_APP_DFU_STM32BL_H_
#define SRC_APP_APP_DFU_APP_DFU_STM32BL_H_

/**
 * @brief Erase, program and verify a BIN file to the target device
 * @param img_bin[in]	The binary file to program (must have the entire path with file name)
 * @return
 * 0 success
 * -ve failed
 */
int stm32bl_flashbin(const char *img_path);

#endif /* SRC_APP_APP_DFU_APP_DFU_STM32BL_H_ */
