/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_GPIO_GPIO_PCA95XX_INT_H_
#define MODULES_GPIO_GPIO_PCA95XX_INT_H_

/** Configuration data */
struct gpio_pca95xx_int_ext_drv_cfg {
	const char **pca95xx_device_names;
	uint32_t pca95xx_device_num;
};

struct gpio_pca95xx_port_data {
	const struct device *pca95xx_device;
	struct {
		uint16_t input_latch;
		uint16_t int_mask;
		uint16_t int_status;
	} reg_cache;
};

/** Runtime driver data */
struct gpio_pca95xx_int_ext_drv_data {
	struct gpio_pca95xx_port_data *port_interrupt_data;
};

typedef int (*pin_configure_t)(const struct device *, gpio_pin_t, gpio_flags_t);
typedef int (*irq_enable_t)(const struct device *, gpio_pin_t);
typedef int (*irq_disable_t)(const struct device *, gpio_pin_t);
typedef int (*latch_enable_t)(const struct device *, gpio_pin_t);
typedef int (*latch_disable_t)(const struct device *, gpio_pin_t);

struct gpio_pca95xx_int_ext_api {
	pin_configure_t pin_configure;
	latch_enable_t latch_enable;
	latch_disable_t latch_disable;
	irq_enable_t pca95xx_irq_enable;
	irq_disable_t pca95xx_irq_disable;
};

#endif /* MODULES_GPIO_GPIO_PCA95XX_INT_H_ */
