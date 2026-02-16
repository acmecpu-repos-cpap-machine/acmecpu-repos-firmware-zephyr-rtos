/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT ti_tps55340

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>

#define LOG_LEVEL CONFIG_TPS55340_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tps55340);

#include "tps55340.h"

#define VOLTAGE_CTRL_DAC_RESOLUTION		12
#define VOLTAGE_FB_ADC_RESOLUTION		12
//#define ADC_GAIN						ADC_GAIN_1
//#define ADC_REFERENCE					ADC_REF_INTERNAL
//#define ADC_ACQUISITION_TIME			ADC_ACQ_TIME_DEFAULT

//#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
static int tps55340_vout_set(const struct device *dev, float vout);
static int tps55340_enable(const struct device *dev);
//#endif

/* the channel configurations with channel not yet filled in */
//static struct dac_channel_cfg m_vctrl_dac_chcfg = {
//	.channel_id = 0, // gets set in runtime
//	.resolution = VOLTAGE_CTRL_DAC_RESOLUTION
//};
//
//static struct adc_channel_cfg m_vfb_chcfg = {
//	.gain             = ADC_GAIN,
//	.reference        = ADC_REFERENCE,
//	.acquisition_time = ADC_ACQUISITION_TIME,
//	.channel_id       = 0, // gets set in runtime
//	.differential	  = 0,
//};

/** Configuration data */
struct tps55340_config {

	/* Enable pin definition */
//	const char *en_gpio_port;
//	gpio_pin_t en_gpio_pin;
//	gpio_flags_t en_gpio_flags;
	struct gpio_dt_spec en_gpio;

	/* Voltage Control DAC channel definition */
//	const char *volt_ctrl_dac_name;
//	uint8_t volt_ctrl_dac_ch;

	/* Voltage Feedback ADC channel definition */
//	const char *volt_fb_adc_name;
//	uint8_t volt_fb_adc_ch;

	/* TPS55340 resistor configurations (in ohms) and reference voltages (in mv) */
	uint32_t res_vi_shunt;
	uint32_t res_vdiv_bot;
	uint32_t res_vdiv_top;
	uint32_t res_vdiv_aux;
	uint32_t volt_ref;
	uint32_t volt_fb;

	uint8_t adc_chan;
	uint8_t dac_chan;
};

struct tps55340_data {
	struct k_sem lock;

	/* Running status of the motor */
	bool run_stat;
};

/* Static functions */
static uint16_t voltage_to_raw(float volts) {
	int multip = 256;

	switch (VOLTAGE_CTRL_DAC_RESOLUTION) {
	default:
	case 8:
		multip = 256;
		break;
	case 10:
		multip = 1024;
		break;
	case 12:
		multip = 4096;
		break;
	case 14:
		multip = 16384;
		break;
	}

	uint16_t raw = (multip * volts) / (float)TPS55340_ADC_DAC_REF_VOLTAGE;
	return raw;
}

static float raw_to_voltage(uint16_t raw) {
	int multip = 256;

	switch (VOLTAGE_FB_ADC_RESOLUTION) {
	default:
	case 8:
		multip = 256;
		break;
	case 10:
		multip = 1024;
		break;
	case 12:
		multip = 4096;
		break;
	case 14:
		multip = 16384;
		break;
	}

	float fout = (raw * TPS55340_ADC_DAC_REF_VOLTAGE / multip);
	return fout;
}

static int32_t adc_resistor_divider_calc_volts(int32_t adc_mvolts) {
	// Vs = Vout(R1+R2)/R2

	uint32_t r1 = TPS55340_ADC_VOLT_DIVIDER_R1;
	uint32_t r2 = TPS55340_ADC_VOLT_DIVIDER_R2;

	int32_t actual_mvolts = ((adc_mvolts *(r1+r2)) / r2);

	return actual_mvolts;
}


/* Init function */
static int tps55340_init(const struct device *dev) {
	const struct tps55340_config *config = dev->config;
	struct tps55340_data *data = dev->data;
	int ret = -1;

	k_sem_init(&data->lock, 1, 1);
	data->run_stat = false;

	/* Disable the device initially */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev == NULL) {
//		LOG_ERR("Could not get enable device");
//		return -ENODEV;
//	}
	if (device_is_ready(config->en_gpio.port)) {
		ret = gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin, (config->en_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)", config->en_gpio.pin, ret);
			return ret;
		}
		ret = gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Error setting GPIO pin (%d)", config->en_gpio.pin);
			return ret;
		}
	}

//#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
#if (CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
	tps55340_vout_set(dev, 15.0);
	tps55340_enable(dev);
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	tps55340_vout_set(dev, 12.0);
	tps55340_enable(dev);
#endif
//#endif

	return 0;
}

/* API Definitions */
static int tps55340_enable(const struct device *dev) {
	const struct tps55340_config *config = dev->config;
	struct tps55340_data *data = dev->data;
	int ret = -1;

	k_sem_take(&data->lock, K_FOREVER);

	/* Enable the device */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev == NULL) {
//		LOG_ERR("Could not get enable device");
//		ret = -ENODEV;
//		goto err;
//	}
	if (device_is_ready(config->en_gpio.port)) {
		ret = gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin, (config->en_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)", config->en_gpio.pin, ret);
			goto err;
		}
		ret = gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Error setting enable GPIO pin (%d)", config->en_gpio.pin);
			goto err;
		}
	}

	data->run_stat = true;

err:
	k_sem_give(&data->lock);
	return ret;
}

static int tps55340_disable(const struct device *dev) {
	const struct tps55340_config *config = dev->config;
	struct tps55340_data *data = dev->data;
	int ret = -1;

	k_sem_take(&data->lock, K_FOREVER);

	/* Disable the device */
//	const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
//	if (en_gpio_dev == NULL) {
//		LOG_ERR("Could not get enable device");
//		ret = -ENODEV;
//		goto err;
//	}
	if (device_is_ready(config->en_gpio.port)) {
		ret = gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin, (config->en_gpio.dt_flags | GPIO_OUTPUT));
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)", config->en_gpio.pin, ret);
			goto err;
		}
		ret = gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Error setting GPIO pin (%d)", config->en_gpio.pin);
			goto err;
		}
	}

	data->run_stat = false;

err:
	k_sem_give(&data->lock);
	return ret;
}

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)
#define DAC_NODE DT_PHANDLE(ZEPHYR_USER_NODE, dac)
#define DAC_CHANNEL_ID DT_PROP(ZEPHYR_USER_NODE, dac_channel_id)
#define DAC_RESOLUTION DT_PROP(ZEPHYR_USER_NODE, dac_resolution)
static const struct device *const dac_dev = DEVICE_DT_GET(DAC_NODE);
static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION
};
static int tps55340_vout_set(const struct device *dev, float vout) {
//	const struct tps55340_config *config = dev->config;
	struct tps55340_data *data = dev->data;
	int ret = -1;

	if ((vout < (float)TPS55340_OUTPUT_VOLTAGE_MIN) || (vout > (float)TPS55340_OUTPUT_VOLTAGE_MAX)) {
		return -EINVAL;
	}

	/* TODO use resistor values for calculation */
	/* Calculate the required input voltage to the TPS55340 to provide the requested output voltage */
	float vin = ((vout - (float)TPS55340_BOOST_CONVERTER_Y_INT) / (float)TPS55340_BOOST_CONVERTER_SLOPE);

	/* Convert the calculated input voltage into DAC value based on set resolution */
	uint16_t dac_raw = voltage_to_raw(vin);
	LOG_DBG("Writing DAC raw value %d for Vout = %0.2f", dac_raw, (double)vout);

	/* setup dac channel */
	if (!device_is_ready(dac_dev)) {
		printk("DAC device %s is not ready\n", dac_dev->name);
		return -1;
	}
	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);

	k_sem_take(&data->lock, K_FOREVER);

	ret = dac_write_value(dac_dev, DAC_CHANNEL_ID, dac_raw);
	if (ret != 0) {
		LOG_ERR("Writing DAC value failed with code %d", ret);
		goto err;
	}

/*
	const struct device *vctrl_dev = device_get_binding(config->volt_ctrl_dac_name);
	if (vctrl_dev == NULL) {
		LOG_ERR("Could not get %s device", config->volt_ctrl_dac_name);
		ret = -ENODEV;
		goto err;
	}

	m_vctrl_dac_chcfg.channel_id = config->volt_ctrl_dac_ch;
	ret = dac_channel_setup(vctrl_dev, &m_vctrl_dac_chcfg);
	if (ret != 0) {
		LOG_ERR("Setting up of DAC channel failed with code %d", ret);
		goto err;
	}

	ret = dac_write_value(vctrl_dev, config->volt_ctrl_dac_ch, dac_raw);
	if (ret != 0) {
		LOG_ERR("Writing DAC value failed with code %d", ret);
		goto err;
	}
*/

err:
	k_sem_give(&data->lock);
	return ret;
}

//******************************************************************
#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};
static int tps55340_vout_get(const struct device *dev, int32_t *mvout) {
	const struct tps55340_config *config = dev->config;
	struct tps55340_data *data = dev->data;
	int ret = -1;

#define BAD_ANALOG_READ -123
	int16_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(buf),
	};

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (adc_channels[i].channel_id == config->adc_chan) {
			if (!device_is_ready(adc_channels[i].dev)) {
				LOG_ERR("ADC controller device not ready\n");
				return -1;
			}
		}
	}

	k_sem_take(&data->lock, K_FOREVER);

	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (adc_channels[i].channel_id == config->adc_chan) {
            int32_t val_mv;
            (void)adc_sequence_init_dt(&adc_channels[i], &sequence);
            ret = adc_read(adc_channels[i].dev, &sequence);
            if (ret < 0) {
                LOG_ERR("Could not read (%d)\n", ret);
                continue;
            } else {
                LOG_DBG("%"PRId16, buf);
            }

            /* conversion to mV may not be supported, skip if not */
            val_mv = buf;
            ret = adc_raw_to_millivolts_dt(&adc_channels[i],
                                &val_mv);
            if (ret < 0) {
                LOG_ERR(" (value in mV not available)\n");
                *mvout = -1;
            } else {
                LOG_DBG(" = %"PRId32" mV\n", val_mv);
//                float t_vout = (float)(val_mv / 1000);
                *mvout = adc_resistor_divider_calc_volts(val_mv);
            }
		}
	}

/*
	const struct device *vfb_dev = device_get_binding(config->volt_fb_adc_name);
	if (vfb_dev == NULL) {
		LOG_ERR("Could not get %s device", config->volt_fb_adc_name);
		ret = -ENODEV;
		goto err;
	}

	m_vfb_chcfg.channel_id = config->volt_fb_adc_ch;
	ret = adc_channel_setup(vfb_dev, &m_vfb_chcfg);
	if (ret != 0) {
		LOG_ERR("Setup of the ADC channel failed with code %d", ret);
		goto err;
	}

	ret = adc_read(vfb_dev, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed with code %d", ret);
		goto err;
	}

	if (vout != NULL) {
		float t_vout = raw_to_voltage(adc_buff);
		// Calculate the output voltage of the TPS55340 from the acquired ADC voltage
		*vout = adc_resistor_divider_calc_volts(t_vout);
	}
*/

//err:
	k_sem_give(&data->lock);
	return ret;
}

static int tps55340_is_running(const struct device *dev) {
	struct tps55340_data *data = dev->data;
	return data->run_stat;
}

static const struct tps55340_driver_api tps55340_drv_api_funcs = {
	.enable = tps55340_enable,
	.disable = tps55340_disable,
	.tps55340_output_voltage_set = tps55340_vout_set,
	.tps55340_output_voltage_get = tps55340_vout_get,
	.tps55340_is_running = tps55340_is_running,
};

#define DEVICE_INSTANCE(inst)																	\
																								\
const static struct tps55340_config tps55340_##inst##_cfg = {									\
	/*.en_gpio_port = DT_INST_GPIO_LABEL(inst, enable_gpios),										\
	.en_gpio_pin = DT_INST_GPIO_PIN(inst, enable_gpios),										\
	.en_gpio_flags = DT_INST_GPIO_FLAGS(inst, enable_gpios),*/									\
	.en_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios), \
																								\
	/*.volt_ctrl_dac_name = DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst),voltage_ctrl),			\
	.volt_ctrl_dac_ch = DT_PHA_BY_NAME(DT_DRV_INST(inst), io_channels, voltage_ctrl, output),	*/\
																								\
	/*.volt_fb_adc_name = DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst),voltage_feedback),		\
	.volt_fb_adc_ch = DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), voltage_feedback),		*/\
\
	.res_vi_shunt = DT_PROP(DT_DRV_INST(inst),v_i_conv_shunt),\
	.res_vdiv_bot = DT_PROP(DT_DRV_INST(inst),vdiv_bot_resistor),\
	.res_vdiv_top = DT_PROP(DT_DRV_INST(inst),vdiv_top_resistor),\
	.res_vdiv_aux = DT_PROP(DT_DRV_INST(inst),vdiv_aux_resistor),\
	.volt_ref = DT_PROP(DT_DRV_INST(inst),vref),\
	.volt_fb = DT_PROP(DT_DRV_INST(inst),vfb),\
\
	.adc_chan = DT_PROP(DT_DRV_INST(inst),adc_channel),\
	.dac_chan = DT_PROP(DT_DRV_INST(inst),dac_channel),\
};																								\
																				\
static struct tps55340_data tps55340_##inst##_drvdata;							\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		tps55340_init,															\
		device_pm_control_nop,													\
		&tps55340_##inst##_drvdata,											\
		&tps55340_##inst##_cfg,												\
		POST_KERNEL, CONFIG_TPS55340_INIT_PRIORITY,							\
		&tps55340_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);

