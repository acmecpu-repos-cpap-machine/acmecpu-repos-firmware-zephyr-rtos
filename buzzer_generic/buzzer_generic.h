/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_BUZZER_GENERIC_BUZZER_GENERIC_H_
#define MODULES_BUZZER_GENERIC_BUZZER_GENERIC_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <zephyr/device.h>

/* API type defines */
typedef int (*buzzer_on_t)(const struct device *);
typedef int (*buzzer_off_t)(const struct device *);

struct buzzer_driver_api {
	buzzer_on_t buzzer_on;
	buzzer_off_t buzzer_off;
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_BUZZER_GENERIC_BUZZER_GENERIC_H_ */
