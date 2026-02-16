/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT st_stm32_ob

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#define LOG_LEVEL STM32_OPTION_BYTES_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stm32_ob);

#include "stm32_option_bytes.h"

struct stm32_ob_config {
	uint8_t dummy;
};

struct stm32_ob_data {
	struct k_sem lock;
};

static int stm32_ob_user_opt_read(const struct device *dev, uint32_t *puser_opt) {
	int ret = 0;

	/* Get the current option byte configurations */
	FLASH_OBProgramInitTypeDef ob_cfg;
	HAL_FLASHEx_OBGetConfig(&ob_cfg);

	/* */
	*puser_opt = ob_cfg.USERConfig;

	return ret;
}

static int stm32_ob_activate_bootloader(const struct device *dev) {
	int ret=0;

	/* Get the current option byte configurations */
	FLASH_OBProgramInitTypeDef ob_cfg;
	HAL_FLASHEx_OBGetConfig(&ob_cfg);

	/* set the required value */
	ob_cfg.OptionType = OPTIONBYTE_USER;
	ob_cfg.USERType = OB_USER_nBOOT1 | OB_USER_nSWBOOT0 | OB_USER_nBOOT0;
	ob_cfg.USERConfig = OB_BOOT1_SYSTEM | OB_BOOT0_FROM_OB | OB_nBOOT0_RESET;

	/* unlock option byte, */
	ret = HAL_FLASH_Unlock();
	ret = HAL_FLASH_OB_Unlock();
	ret = HAL_FLASHEx_OBProgram(&ob_cfg);
//	ret = HAL_FLASH_OB_Lock();
//	ret = HAL_FLASH_Lock();
	ret = HAL_FLASH_OB_Launch();
//	LOG_INF("ret = %d", ret);

//	HAL_FLASHEx_OBGetConfig(&ob_cfg);

	return ret;
}

static int stm32_ob_init(const struct device *dev) {
	int ret = 0;

	return ret;
}

static const struct stm32_ob_driver_api stm32_ob_api = {
		.user_opt_read = stm32_ob_user_opt_read,
		.activate_bootloader = stm32_ob_activate_bootloader,
};

static const struct stm32_ob_config stm32_ob_0_config = {
	.dummy = 0,
};
static struct stm32_ob_data stm32_ob_0_data;

DEVICE_DT_INST_DEFINE(0, stm32_ob_init, device_pm_control_nop, &stm32_ob_0_data,
		    &stm32_ob_0_config,
		    POST_KERNEL, CONFIG_STM32_OB_DRIVER_INIT_PRIORITY,
		    &stm32_ob_api);
