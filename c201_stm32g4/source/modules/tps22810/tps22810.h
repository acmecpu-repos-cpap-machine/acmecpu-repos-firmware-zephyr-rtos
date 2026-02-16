/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_TPS22810_TPS22810_H_
#define MODULES_TPS22810_TPS22810_H_


typedef int (*tps22810_enable_t)(const struct device *);
typedef int (*tps22810_disable_t)(const struct device *);

struct tps22810_driver_api {
	tps22810_enable_t enable;
	tps22810_disable_t disable;
};

#endif /* MODULES_TPS22810_TPS22810_H_ */
