/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 7-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
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

#include "app_analog/app_analog.h"

#define HIGH	1
#define LOW		0

#define VMEASURE_EN_PIN		DT_GPIO_PIN(DT_NODELABEL(vbat_sense_en), gpios)
#define VMEASURE_EN_FLAGS	(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(vbat_sense_en), gpios))

#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
#define IOCHAN_IDX_VM		0
#define VM_ADC_CHAN   		6
#define VM_R1     			(390*1000)
#define VM_R2     			(75*1000)
#define ACT_VM(x)  			(x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

#define IOCHAN_IDX_VBUS		1
#define VBUS_ADC_CHAN   	7
#define VBUS_R1     		(390*1000)
#define VBUS_R2     		(75*1000)
#define ACT_VBUS(x)  		(x * (VBUS_R1 + VBUS_R2) / VBUS_R2)

#define IOCHAN_IDX_VBAT		2
#define VBAT_ADC_CHAN   	8
#define VBAT_R1     		(27*1000)
#define VBAT_R2     		(75*1000)
#define ACT_VBAT(x)  		(x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205)
#define IOCHAN_IDX_VBUS		0
#define VBUS_ADC_CHAN   	7
#define VBUS_R1     		(390*1000)
#define VBUS_R2     		(75*1000)
#define ACT_VBUS(x)  		(x * (VBUS_R1 + VBUS_R2) / VBUS_R2)

#define IOCHAN_IDX_VM		1
#define VM_ADC_CHAN   		8
#define VM_R1     			(390*1000)
#define VM_R2     			(75*1000)
#define ACT_VM(x)  			(x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

#define IOCHAN_IDX_VBAT		2
#define VBAT_ADC_CHAN   	10
#define VBAT_R1     		(27*1000)
#define VBAT_R2     		(75*1000)
#define ACT_VBAT(x)  		(x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

#elif (CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
/* USB voltage */
#define IOCHAN_IDX_VBUS		0
#define VBUS_ADC_CHAN   	3
#define VBUS_R1     		(390*1000)
#define VBUS_R2     		(75*1000)
#define ACT_VBUS(x)  		(x * (VBUS_R1 + VBUS_R2) / VBUS_R2)

/* battery voltage */
#define IOCHAN_IDX_VBAT		1
#define VBAT_ADC_CHAN   	7
#define VBAT_R1     		(390*1000)
#define VBAT_R2     		(75*1000)
#define ACT_VBAT(x)  		(x * (VBAT_R1 + VBAT_R2) / VBAT_R2)

/* thermistor 1 voltage */
#define IOCHAN_IDX_NTC1		2
#define NTC1_ADC_CHAN   	8
#define NTC1_R1     		(5.1*1000)
#define NTC1_R2     		(10*1000)
#define ACT_NTC1(x)  		(x * (NTC1_R1 + NTC1_R2) / NTC1_R2)

/* DC jack voltage */
#define IOCHAN_IDX_PWRJACK	3
#define PWRJACK_ADC_CHAN	14
#define PWRJACK_R1     		(390*1000)
#define PWRJACK_R2     		(75*1000)
#define ACT_PWRJACK(x)  		(x * (PWRJACK_R1 + PWRJACK_R2) / PWRJACK_R2)

/* thermistor 2 voltage */
#define IOCHAN_IDX_NTC2		4
#define NTC2_ADC_CHAN   	19
#define NTC2_R1     		(5.1*1000)
#define NTC2_R2     		(10*1000)
#define ACT_NTC2(x)  		(x * (NTC2_R1 + NTC2_R2) / NTC2_R2)

/* blower voltage */
#define IOCHAN_IDX_VM		5
#define VM_ADC_CHAN   		2
#define VM_R1     			(470*1000)
#define VM_R2     			(75*1000)
#define ACT_VM(x)  			(x * (VM_R1 + VM_R2) / VM_R2)

#endif

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

#if 0
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
#endif

int app_analog_vbat_mv_get(int32_t *pval_mv)
{
	int ret = 0;
   	int16_t buf;
   	int32_t val_mv;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	const struct adc_dt_spec adc_chan = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), IOCHAN_IDX_VBAT);
	ret = adc_sequence_init_dt(&adc_chan, &sequence);
	if (ret < 0) {
		LOG_ERR("adc_sequence_init_dt failed, %d", ret);
		return ret;
	}
	ret = adc_read(adc_chan.dev, &sequence);
    if (ret < 0) {
        LOG_ERR("Could not read (%d)", ret);
        return ret;
    } else {
        LOG_DBG("%"PRId16, buf);
    }

    /* conversion to mV may not be supported, skip if not */
    val_mv = buf;
    ret = adc_raw_to_millivolts_dt(&adc_chan, &val_mv);
    if (ret < 0) {
        LOG_ERR("value in mV not available, %d", ret);
        *pval_mv = -1;
        return ret;
    } else {
        LOG_DBG(" = %"PRId32" mV\n", val_mv);
        // *pval_mv = val_mv;
        *pval_mv = ACT_VBAT(val_mv);
    }

	return ret;
}

int app_analog_vbus_mv_get(int32_t *pval_mv)
{
	int ret = 0;
   	int16_t buf;
   	int32_t val_mv;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	const struct adc_dt_spec adc_chan = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), IOCHAN_IDX_VBUS);
	ret = adc_sequence_init_dt(&adc_chan, &sequence);
	if (ret < 0) {
		LOG_ERR("adc_sequence_init_dt failed, %d", ret);
		return ret;
	}
	ret = adc_read(adc_chan.dev, &sequence);
    if (ret < 0) {
        LOG_ERR("Could not read (%d)", ret);
        return ret;
    } else {
        LOG_DBG("%"PRId16, buf);
    }

    /* conversion to mV may not be supported, skip if not */
    val_mv = buf;
    ret = adc_raw_to_millivolts_dt(&adc_chan, &val_mv);
    if (ret < 0) {
        LOG_ERR("value in mV not available, %d", ret);
        *pval_mv = -1;
        return ret;
    } else {
        LOG_DBG(" = %"PRId32" mV\n", val_mv);
        // *pval_mv = val_mv;
        *pval_mv = ACT_VBUS(val_mv);
    }

	return ret;
}
int app_analog_pwrjack_mv_get(int32_t *pval_mv)
{
	int ret = 0;
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
   	int16_t buf;
   	int32_t val_mv;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};
	const struct adc_dt_spec adc_chan = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), IOCHAN_IDX_PWRJACK);
	ret = adc_sequence_init_dt(&adc_chan, &sequence);
	if (ret < 0) {
		LOG_ERR("adc_sequence_init_dt failed, %d", ret);
		return ret;
	}
	ret = adc_read(adc_chan.dev, &sequence);
    if (ret < 0) {
        LOG_ERR("Could not read (%d)", ret);
        return ret;
    } else {
        LOG_DBG("%"PRId16, buf);
    }

    /* conversion to mV may not be supported, skip if not */
    val_mv = buf;
    ret = adc_raw_to_millivolts_dt(&adc_chan, &val_mv);
    if (ret < 0) {
        LOG_ERR("value in mV not available, %d", ret);
        *pval_mv = -1;
        return ret;
    } else {
        LOG_DBG(" = %"PRId32" mV\n", val_mv);
        // *pval_mv = val_mv;
        *pval_mv = ACT_PWRJACK(val_mv);
    }
#endif	/* (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W) */
	return ret;
}

int app_analog_measure_en(APP_ANALOG_DEVICES ana_dev)
{
	int ret = 0;
	switch (ana_dev) {
	case APP_ANALOG_VBAT:
	{
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(vbat_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, HIGH);
	}
		break;
	case APP_ANALOG_VBUS:
	{
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W || CONFIG_BOARD_C208T)
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(vbus_in_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, HIGH);
#endif	/* (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W) */
	}
		break;
	case APP_ANALOG_PWRJACK:
	{
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(pwr_jack_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, HIGH);
#endif	/* (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W) */
	}
		break;
	case APP_ANALOG_VM:
	{

	}
		break;
	case APP_ANALOG_NTC1:
	{

	}
		break;
	case APP_ANALOG_NTC2:
	{

	}
		break;
	default:
		break;
	}

//	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(vbat_sense_en), gpios));
//	if (dev == NULL) {
//		LOG_ERR("Device not found: %p", dev);
//		return -1;
//	}
//	ret = gpio_pin_configure(dev, VMEASURE_EN_PIN, (GPIO_OUTPUT | VMEASURE_EN_FLAGS));
//	if (ret < 0) {
//		LOG_ERR("gpio_pin_configure failed");
//		return ret;
//	}
//    ret = gpio_pin_set(dev, VMEASURE_EN_PIN, 1);

    return ret;
}

int app_analog_measure_dis(APP_ANALOG_DEVICES ana_dev)
{
	int ret = 0;
	switch (ana_dev) {
	case APP_ANALOG_VBAT:
	{
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(vbat_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, LOW);
	}
		break;
	case APP_ANALOG_VBUS:
	{
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W || CONFIG_BOARD_C208T)
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(vbus_in_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, LOW);
#endif	/* (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W) */
	}
		break;
	case APP_ANALOG_PWRJACK:
	{
#if (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
		const struct gpio_dt_spec dev = GPIO_DT_SPEC_GET(DT_NODELABEL(pwr_jack_sense_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}

		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}
		ret = gpio_pin_set_dt(&dev, LOW);
#endif	/* (CONFIG_BOARD_E206 || CONFIG_BOARD_E206W) */
	}
		break;
	case APP_ANALOG_VM:
	{

	}
		break;
	case APP_ANALOG_NTC1:
	{

	}
		break;
	case APP_ANALOG_NTC2:
	{

	}
		break;
	default:
		break;
	}
//	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(vbat_sense_en), gpios));
//	if (dev == NULL) {
//		// LOG_ERR("Device not found: %s", USB_DSEL_DEV_NAME);
//		LOG_ERR("Device not found: %p", dev);
//		return -1;
//	}
//	ret = gpio_pin_configure(dev, VMEASURE_EN_PIN, (GPIO_OUTPUT | VMEASURE_EN_FLAGS));
//	if (ret < 0) {
//		LOG_ERR("gpio_pin_configure failed");
//		return ret;
//	}
//    ret = gpio_pin_set(dev, VMEASURE_EN_PIN, 0);
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
//    err = app_analog_measure_en();
#else
    err = -1;
    LOG_ERR("Analog measurement not enabled in application, set CONFIG_APP_ANALOG to y");
#endif

    return err;
}
