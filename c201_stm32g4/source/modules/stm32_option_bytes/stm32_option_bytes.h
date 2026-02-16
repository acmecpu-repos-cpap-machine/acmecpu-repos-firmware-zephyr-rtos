/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_STM32_OPTION_BYTES_STM32_OPTION_BYTES_H_
#define MODULES_STM32_OPTION_BYTES_STM32_OPTION_BYTES_H_

#include <stdint.h>

typedef int (*stm32_ob_user_opt_read_t)(const struct device *, uint32_t*);
typedef int (*stm32_ob_activate_bootloader_t)(const struct device *);

struct stm32_ob_driver_api {
	stm32_ob_user_opt_read_t	user_opt_read;
	stm32_ob_activate_bootloader_t activate_bootloader;
};

#endif /* MODULES_STM32_OPTION_BYTES_STM32_OPTION_BYTES_H_ */
