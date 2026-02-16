/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 23-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr.h>
#include <device.h>
#include <kernel.h>
#include <sys/slist.h>
//#include <sys/printk.h>
#include <version.h>
#include <stdlib.h>
#include <string.h>
#include <storage/disk_access.h>
#include <fs/fs.h>
#if (CONFIG_FAT_FILESYSTEM_ELM)
	#include <ff.h>
#endif
#if (CONFIG_FILE_SYSTEM_LITTLEFS)
	#include <zephyr/fs/littlefs.h>
#endif
#include <zephyr/storage/flash_map.h>
#if CONFIG_DISK_DRIVER_FLASH
#include <storage/flash_map.h>
#endif
#include "app_storage/app_storage.h"
#include <logging/log.h>
LOG_MODULE_REGISTER(app_storage);
#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	#include "bsp_fs_helper.h"
#endif
#if CONFIG_LIB_FILE_OPER
#include "lib_file_oper/lib_file_oper.h"
#endif
#include "lib_events/lib_events.h"
#if (CONFIG_APP_SETTINGS)
	#include "app_settings/app_settings.h"
#endif

/* static variables */
static struct lib_events_callback m_cb_settings_changed;

#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
/*
 * Note the fatfs library is able to mount only strings inside _VOLUME_STRS in ffconf.h
 * */
#define SD_MOUNT_POINT	CONFIG_BSP_FS_SD_CARD_FAT_MOUNT_POINT
#endif

/* mounting info */
#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
static FATFS m_fat_fs;
static struct fs_mount_t m_mp = { .type = FS_FATFS, .fs_data = &m_fat_fs, };	/* sd card */
static bool m_sd_mnt_stat = false;
#endif

#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
bool bsp_fs_sd_card_is_mounted() {
	return m_sd_mnt_stat;
}

static int mount_sd_card() {
	int res=0;

	/* Check if the card is already mounted or not */
	if (m_sd_mnt_stat == true) {
		res = 0;
		goto err;
	}

	do {
		static const char *disk_pdrv = "SD";
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_init(disk_pdrv) != 0) {
			printk("Storage init ERROR!");
			break;
		}
		if (disk_access_ioctl(disk_pdrv,
		DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			printk("Unable to get sector count");
			break;
		}
		printk("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv,
		DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			printk("Unable to get sector size");
			break;
		}
		printk("Sector size %u\n", block_size);

		memory_size_mb = (uint64_t) block_count * block_size;
		printk("Memory Size(MB) %u\n", (uint32_t )(memory_size_mb >> 20));
	} while (0);

	m_mp.mnt_point = SD_MOUNT_POINT;
	res = fs_mount(&m_mp);
	if (res == FR_OK) {
		m_sd_mnt_stat = true;
		printk("Disk mounted, mount point = %s\n", SD_MOUNT_POINT);
	} else {
		m_sd_mnt_stat = false;
		printk("Error mounting disk.\n");
	}

err:
	return res;
}

static int unmount_sd_card() {
	int res = fs_unmount(&m_mp);
	if (res == FR_OK) {
		m_sd_mnt_stat = false;
		printk("Disk unmounted\n");
	} else if (res == -EINVAL) {
		printk("no system has been mounted at given mount point\n");
	} else {
		printk("Error un-mounting disk.\n");
	}

	return res;
}
#endif /*#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)*/

#if (CONFIG_APP_LITTLEFS_STORAGE_FLASH)
static int littlefs_flash_erase(unsigned int id)
{
	const struct flash_area *pfa;
	int rc;

	rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		LOG_ERR("FAIL: unable to find flash area %u: %d\n",
			id, rc);
		return rc;
	}

	LOG_INF("Area %u at 0x%x on %s for %u bytes\n",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	/* Optional wipe flash contents */
	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		LOG_ERR("Erasing flash area ... %d", rc);
	}

	flash_area_close(pfa);
	return rc;
}

#define PARTITION_NODE DT_NODELABEL(lfs1)
#if DT_NODE_EXISTS(PARTITION_NODE)
	FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
	FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
	static struct fs_mount_t lfs_storage_mnt = {
		.type = FS_LITTLEFS,
		.fs_data = &storage,
		.storage_dev = (void *)FIXED_PARTITION_ID(data_partition),
		.mnt_point = APP_STORAGE_MOUNT_POINT_FLASH,
	};
#endif /* PARTITION_NODE */

	struct fs_mount_t *mp =
#if DT_NODE_EXISTS(PARTITION_NODE)
		&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
		&lfs_storage_mnt
#endif
		;

static int littlefs_mount(struct fs_mount_t *mp)
{
	int rc;

	rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
	if (rc < 0) {
		return rc;
	}

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||						\
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
	rc = fs_mount(mp);
	if (rc < 0) {
		LOG_ERR("FAIL: mount id %" PRIuPTR " at %s: %d\n",
		       (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return rc;
	}
	LOG_INF("%s mount: %d\n", mp->mnt_point, rc);
#else
	LOG_INF("%s automounted\n", mp->mnt_point);
#endif

	return 0;
}
static int mount_ext_flash()
{
	int rc = 0;
	rc = littlefs_mount(mp);
	if (rc < 0) {
		return rc;
	}
	return rc;
}

#else	/* CONFIG_APP_LITTLEFS_STORAGE_FLASH */

static FATFS m_fat_fs;
static struct fs_mount_t fs_mnt;	/* external flash memory */

static int mount_ext_flash() {

	struct fs_mount_t *mp = &fs_mnt;
	int rc;

	if (IS_ENABLED(CONFIG_DISK_DRIVER_FLASH)) {
		unsigned int id;
		const struct flash_area *pfa;

		mp->storage_dev = (void*) FLASH_AREA_ID(data);
		id = (uintptr_t) mp->storage_dev;

		rc = flash_area_open(id, &pfa);
		printk("Area %u at 0x%x for %u bytes\n", id, (unsigned int) pfa->fa_off, (unsigned int) pfa->fa_size);

		if (rc < 0 && IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
			printk("Erasing flash area ... ");
			rc = flash_area_erase(pfa, 0, pfa->fa_size);
			printk("%d\n", rc);
		}

		if (rc < 0) {
			flash_area_close(pfa);
			return -1;
		}
	}

	static FATFS fat_fs;
	mp->type = FS_FATFS;
	mp->fs_data = &fat_fs;
	mp->mnt_point = "/NAND:";
	rc = fs_mount(mp);
	if (rc < 0) {
//		LOG_ERR("Failed to mount filesystem");
		return -1;
	}
	return 0;
}
#endif /* CONFIG_APP_LITTLEFS_STORAGE_FLASH */

static void lib_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch (event) {
	case LIB_EVENT_SETTINGS_CHANGED:
		{
#if (CONFIG_APP_SETTINGS)
			char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = {0x00};
			app_settings_changed_latest_get(changed_setting);

			/* todo */
#endif	/* (CONFIG_APP_SETTINGS) */
			break;
		}
		default:
			break;
		}
}

int app_storage_mount()
{
	int ret=0;

#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	ret = mount_sd_card(0);
	if (ret != 0) {
		LOG_ERR("mount_sd_card failed");
	}
#endif	/* (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201) */

	ret |= mount_ext_flash();
	if (ret != 0) {
		LOG_ERR("mount_ext_flash failed");
	}

#if 0//(CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	ret = bsp_fs_init();
	if (ret != 0) {
		LOG_ERR("bsp_fs_init failed");
	}
#endif

	ret = lib_events_callback_add(&m_cb_settings_changed, lib_event_handler, LIB_EVENT_SETTINGS_CHANGED);

#if CONFIG_LIB_FILE_OPER
	lib_file_oper_init();
#endif
	return ret;
}
