/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_TPS61169_TPS61169_H_
#define MODULES_TPS61169_TPS61169_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/device.h>

/* API type defines */
typedef int (*tps61169_set_brightness_t)(const struct device *, uint8_t);

struct tps61169_driver_api {
	tps61169_set_brightness_t tps61169_set_brightness;
};

#ifdef __cplusplus
}
#endif

#endif /* MODULES_TPS61169_TPS61169_H_ */
