/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 23-Feb-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_
#define SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_

#define APP_STORAGE_MOUNT_POINT_FLASH	"/lfs"

/**
 * Mounts all storage partitions (sd card, flash etc.)
 */
int app_storage_mount();

#endif /* SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_ */
