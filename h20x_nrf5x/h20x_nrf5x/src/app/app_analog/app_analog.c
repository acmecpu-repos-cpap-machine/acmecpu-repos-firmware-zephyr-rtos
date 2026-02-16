/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 7-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_analog);

#define VBAT_ADC_CHAN   2 //DT_PROP(DT_NODELABEL(vbat), reg)
#define VBUS_ADC_CHAN   3 //DT_PROP(DT_NODELABEL(vbus), reg)

#define VMEASURE_EN_PIN		DT_GPIO_PIN(DT_NODELABEL(vmeasure_en), gpios)
#define VMEASURE_EN_FLAGS	(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(vmeasure_en), gpios))

#define VBAT_R1     CONFIG_APP_ANALOG_VBAT_R1 //2700
#define VBAT_R2     CONFIG_APP_ANALOG_VBAT_R2 //7500
#define ACT_VBAT(x)  (x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

#define VBUS_R1     CONFIG_APP_ANALOG_VBUS_R1 //4700
#define VBUS_R2     CONFIG_APP_ANALOG_VBUS_R2 //7500
#define ACT_VBUS(x)  (x * (VBUS_R1 + VBUS_R2) / VBUS_R2)

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
			     DT_SPEC_AND_COMMA)
};

int app_analog_vbat_mv_get(int32_t *pval_mv)
{
    int err=0;
   	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
        if (adc_channels[i].channel_id == VBAT_ADC_CHAN) {
            int32_t val_mv;

            LOG_DBG("- %s, channel %d: ",
                    adc_channels[i].dev->name,
                    adc_channels[i].channel_id);

            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

            err = adc_read(adc_channels[i].dev, &sequence);
            if (err < 0) {
                LOG_ERR("Could not read (%d)\n", err);
                continue;
            } else {
                LOG_DBG("%"PRId16, buf);
            }

            /* conversion to mV may not be supported, skip if not */
            val_mv = buf;
            err = adc_raw_to_millivolts_dt(&adc_channels[i],
                                &val_mv);
            if (err < 0) {
                LOG_ERR(" (value in mV not available)\n");
                *pval_mv = -1;
            } else {
                LOG_DBG(" = %"PRId32" mV\n", val_mv);
                // *pval_mv = val_mv;
                *pval_mv = ACT_VBAT(val_mv);
            }
        }
    }
    return err;
}

int app_analog_vbus_mv_get(int32_t *pval_mv)
{
    int err=0;
   	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

    for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
        if (adc_channels[i].channel_id == VBUS_ADC_CHAN) {
            int32_t val_mv;

            LOG_DBG("- %s, channel %d: ",
                    adc_channels[i].dev->name,
                    adc_channels[i].channel_id);

            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);

            err = adc_read(adc_channels[i].dev, &sequence);
            if (err < 0) {
                LOG_ERR("Could not read (%d)\n", err);
                continue;
            } else {
                LOG_DBG("%"PRId16, buf);
            }

            /* conversion to mV may not be supported, skip if not */
            val_mv = buf;
            err = adc_raw_to_millivolts_dt(&adc_channels[i],
                                &val_mv);
            if (err < 0) {
                LOG_ERR(" (value in mV not available)\n");
                *pval_mv = -1;
            } else {
                LOG_DBG(" = %"PRId32" mV\n", val_mv);
                // *pval_mv = val_mv;
                *pval_mv = ACT_VBUS(val_mv);
            }
        }
    }
    return err;
}

int app_analog_measure_en()
{
	int ret = 0;
	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(vmeasure_en), gpios));
	if (dev == NULL) {
		// LOG_DBG("Device not found: %s", USB_DSEL_DEV_NAME);
		LOG_ERR("Device not found: %p", dev);
		return -1;
	}
	ret = gpio_pin_configure(dev, VMEASURE_EN_PIN, (GPIO_OUTPUT | VMEASURE_EN_FLAGS));
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return ret;
	}
    ret = gpio_pin_set(dev, VMEASURE_EN_PIN, 1);
    return ret;
}

int app_analog_measure_dis()
{
	int ret = 0;
	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(vmeasure_en), gpios));
	if (dev == NULL) {
		// LOG_ERR("Device not found: %s", USB_DSEL_DEV_NAME);
		LOG_ERR("Device not found: %p", dev);
		return -1;
	}
	ret = gpio_pin_configure(dev, VMEASURE_EN_PIN, (GPIO_OUTPUT | VMEASURE_EN_FLAGS));
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return ret;
	}
    ret = gpio_pin_set(dev, VMEASURE_EN_PIN, 0);
    return ret;    
}

int app_analog_init()
{
    int err=0;
#if CONFIG_APP_ANALOG
	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!device_is_ready(adc_channels[i].dev)) {
			LOG_ERR("ADC controller device not ready\n");
			return -1;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
			return err;
		}
	}
    err = app_analog_measure_en();
#else
    err = -1;
    LOG_ERR("Analog measurement not enabled in application, set CONFIG_APP_ANALOG to y");
#endif

    return err;
}