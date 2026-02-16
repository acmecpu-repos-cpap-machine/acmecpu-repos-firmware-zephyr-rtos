/*
 * Copyright (c) 2021 Acme CPU
 */



#ifndef SRC_INCLUDE_APP_DATA_RECORDER_H_
#define SRC_INCLUDE_APP_DATA_RECORDER_H_

int app_data_recorder_init();

int app_data_recorder_save(const char *data, size_t length);

#endif /* SRC_INCLUDE_APP_DATA_RECORDER_H_ */
