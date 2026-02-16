/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bsp_fw_update);

#include "acpu_c201_modules.h"
#if (CONFIG_STM32_OPTION_BYTES_DRIVER)
#include "stm32_option_bytes.h"
#endif

int bsp_fwupdate_config_bootloader() {
	int ret=0;
#if (CONFIG_STM32_OPTION_BYTES_DRIVER)
	const struct device *ob_dev = device_get_binding(ACPU_C201_MOD_NAME_STM32_FLASH_OB);
	if (!ob_dev) {
		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_STM32_FLASH_OB);
		return -1;
	}

	const struct stm32_ob_driver_api *api = ob_dev->api;
	ret = api->activate_bootloader(ob_dev);
	if (ret) {
		LOG_ERR("Activate bootloader failed!");
	}
#endif
	return ret;
//	sys_reboot(SYS_REBOOT_WARM);
}
