/*
 * Copyright (c) 2021 Acme CPU
 */

#include <init.h>
#include "bsp_gpios.h"
//#include "gpio_pca95xx_int.h"

#define DT_DRV_COMPAT bsp_gpios

static int bsp_gpio_init(const struct device *dev)
{
#if 0
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	data->port = device_get_binding(cfg->port_name);
	if (data->port != NULL)
	{
		if (cfg->gpio_pca95xx_compat)
		{
			const struct device *pca95xx_int_ext_dev = device_get_binding("gpio_pca95xx_int_ext");
			const struct gpio_pca95xx_int_ext_api *pca95xx_int_ext_api = pca95xx_int_ext_dev->api;
			int ret = pca95xx_int_ext_api->pin_configure(data->port, cfg->pin, cfg->flags);
			if (ret != 0) {
				printk("ret = %d\n", ret);
			}
			/* TODO:
			 * For now, we are ignoring error here as it does not create any problems.
			 * The error returned is primarily because of gpio_pca95xx does not support interrupt
			 * configuration as required by pcal9639 chip
			 * Either, we need to add error handling here or we need to enhance the
			 * gpio_pca95xx device driver later
			 * */
			return 0;
		}
		else
		{
			/* For pins other than pca95xx we configure them here */
			return gpio_pin_configure(data->port, cfg->pin, cfg->flags);
		}

//		int ret = gpio_pin_configure(data->port, cfg->pin, cfg->flags);
//		if (ret != 0) {
//			printk("ret = %d\n", ret);
//		}
//		return 0;
	}
	else
	{
		return -EINVAL;
	}
#endif
	return 0;
}

static int bsp_gpio_set (const struct device *dev)
{
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	return gpio_pin_set(data->port, cfg->pin, 1);
}

static int bsp_gpio_reset (const struct device *dev)
{
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	return gpio_pin_set(data->port, cfg->pin, 0);
}

static int bsp_gpio_toggle (const struct device *dev)
{
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	return gpio_pin_toggle(data->port, cfg->pin);

}

static int bsp_gpio_config (const struct device *dev)
{
	const struct bsp_gpios_cfg *cfg = dev->config;
	struct bsp_gpios_data *data = dev->data;
	return gpio_pin_configure(data->port, cfg->pin, cfg->flags);
}

static const struct bsp_gpio_driver_api bsp_gpio_api = {
		.config = bsp_gpio_config,
		.set = bsp_gpio_set,
		.reset = bsp_gpio_reset,
		.toggle = bsp_gpio_toggle,
};

#define BSP_GPIOS(node_id)												\
	static const struct												\
		bsp_gpios_cfg gpio_##node_id##_cfg = {						\
			.port_name = DT_LABEL(DT_PHANDLE(node_id, gpios)),\
			.pin = DT_GPIO_PIN(node_id, gpios),						\
			.flags = DT_GPIO_FLAGS(node_id, gpios),					\
			.gpio_pca95xx_compat = DT_NODE_HAS_COMPAT(\
								DT_PHANDLE(node_id,gpios), nxp_pca95xx),\
};																\
\
	static struct													\
		bsp_gpios_data gpio_##node_id##_data;						\
\
	DEVICE_DEFINE(bsp_gpio_##node_id, DT_LABEL(node_id), bsp_gpio_init, NULL,\
						&gpio_##node_id##_data, &gpio_##node_id##_cfg,	\
						POST_KERNEL, CONFIG_BSP_GPIO_DRIVER_INIT_PRIORITY,	\
						&bsp_gpio_api);

#define CHILD_NODES(inst) \
		DT_FOREACH_CHILD(DT_DRV_INST(inst),BSP_GPIOS)

DT_INST_FOREACH_STATUS_OKAY(CHILD_NODES);
