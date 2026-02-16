/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <version.h>
#include <stdlib.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>
#include <logging/log.h>
#include <logging/log_ctrl.h>
LOG_MODULE_REGISTER(app_logger);

#include "bsp_fs_helper.h"
#include "app_time/app_time.h"
#include "app_settings/app_settings.h"

#if CONFIG_APP_LOG_STORAGE_SD_CARD
#define MOUNT_POINT			CONFIG_BSP_FS_SD_CARD_FAT_MOUNT_POINT
#elif CONFIG_APP_LOG_STORAGE_FLASH
#define MOUNT_POINT			CONFIG_BSP_FS_FLASH_FAT_MOUNT_POINT
#endif

#if (CONFIG_APP_LOG_STORAGE_SD_CARD || CONFIG_APP_LOG_STORAGE_FLASH)
#define LOG_DIRECTORY_NAME		"log"
#define LOG_DIRECTORY_PATH		MOUNT_POINT "/" LOG_DIRECTORY_NAME
#endif

extern const struct log_backend *app_logger_backend_sdcard_get(void);
extern const struct log_backend *app_logger_backend_storage_get(void);

static uint32_t timestamp_get(void)
{
	return (uint32_t)app_time_value_get_secs();
}

static uint32_t timestamp_freq(void)
{
	return 1;//(uint32_t)K_SECONDS(1);
}


int app_logger_init() {
	int ret = 0;

/*
	ret = bsp_fs_init();
	if (ret != 0) {
		LOG_ERR("bsp_fs_init failed");
		return ret;
	}
*/

//#if CONFIG_APP_LOGGER_SD_CARD
/*
	ret = bsp_fs_mount_sd_card(0);
	if (ret != 0) {
		LOG_ERR("bsp_fs_mount_sd_card failed");
		return ret;
	}
*/

#if (CONFIG_APP_LOG_STORAGE_SD_CARD || CONFIG_APP_LOG_STORAGE_FLASH)
	/* Check and create the log directory if it doesn't exist */
//	if (bsp_fs_sd_card_is_mounted()) {
		if (!bsp_fs_dir_exist(LOG_DIRECTORY_PATH)) {
			bsp_fs_make_dir(LOG_DIRECTORY_PATH);
		}
//	} else {
//		printk("app_logger_backend_sdcard_init due to SD Card not mounted\n");
//	}

	if (!IS_ENABLED(CONFIG_LOG_BACKEND_STORAGE_AUTOSTART)) {
		/* Example how to start the backend if autostart is disabled.
		 * This is useful if the application needs to wait for the storage device
		 * to be mounted before the logger is able to work.
		 */
		const struct log_backend *backend = app_logger_backend_storage_get(); //app_logger_backend_sdcard_get();

		uint8_t start_logger = 0;
		if (app_settings_load_single(SETTINGS_KEY_FULL_LOG_STORE, &start_logger,
				sizeof(start_logger)) == 0) {
			if (start_logger == SETTINGS_LOG_STORE) {

				if (!log_backend_is_active(backend)) {
					if (backend->api->init != NULL) {
						backend->api->init(backend);
					}

					log_backend_activate(backend, NULL);

					/* Set the logging timestamp function */
					log_set_timestamp_func(timestamp_get, timestamp_freq());
				}
			} else {
				log_backend_deactivate(backend);
			}
		}
	}
#endif /*#if (APP_LOG_STORAGE_SD_CARD || APP_LOG_STORAGE_FLASH)*/
//#endif

	return ret;
}
