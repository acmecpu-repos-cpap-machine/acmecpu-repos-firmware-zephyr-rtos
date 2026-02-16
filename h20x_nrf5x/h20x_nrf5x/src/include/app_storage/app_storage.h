/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 1-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_
#define SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_

int app_storage_init();
int app_storage_deinit();
const char* app_storage_mp_get();

#endif /* SRC_INCLUDE_APP_STORAGE_APP_STORAGE_H_ */