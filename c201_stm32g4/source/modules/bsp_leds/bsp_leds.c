/*
 * Copyright (c) 2021 Acme CPU
 *
 */

#define DT_DRV_COMPAT bsp_leds

#include <zephyr.h>

#define LOG_LEVEL CONFIG_LED_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(bsp_leds);

#include "bsp_leds.h"

static int bsp_leds_config(const struct device *dev)
{
	return 0;
}

static int bsp_led_init(const struct device *dev)
{

/*
	const struct bsp_leds_cfg *cfg = dev->config;
	const struct bsp_leds_data *data = dev->data;

	LOG_INF("bsp_led_init: %s, %d", cfg->led_drv_name, cfg->led_idx);
*/


	return 0;
}

static const struct bsp_leds_driver_api bsp_led_api = {
	.config = bsp_leds_config,
};

#define BSP_LEDS(node_id)												\
	static const struct													\
		bsp_leds_cfg led_##node_id##_cfg = {							\
			.led_drv_name = DT_LABEL(DT_PHANDLE(node_id, pwms)),		\
			.led_idx  = DT_PHA_BY_IDX(node_id, pwms, 0, channel),		\
	};																	\
																		\
	static struct														\
		bsp_leds_data led_##node_id##_data;								\
																		\
	DEVICE_DEFINE(bsp_led_##node_id, DT_LABEL(node_id), bsp_led_init, NULL,\
						&led_##node_id##_data, &led_##node_id##_cfg,	\
						POST_KERNEL, CONFIG_BSP_LEDS_INIT_PRIORITY,		\
						&bsp_led_api);


#define CHILD_NODES(inst) \
		DT_FOREACH_CHILD(DT_DRV_INST(inst), BSP_LEDS)

DT_INST_FOREACH_STATUS_OKAY(CHILD_NODES);
