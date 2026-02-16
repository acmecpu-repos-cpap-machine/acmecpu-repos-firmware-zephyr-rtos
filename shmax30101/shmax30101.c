/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 22-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#define DT_DRV_COMPAT maxim_shmax30101

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_SHMAX30101_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(shmax30101);

#include <stdio.h>

#include "shmax30101.h"
#include "SSInterface.h"
#include "SSMAX30101Comm.h"

/** Configuration data */
struct shmax30101_config {
	struct i2c_dt_spec i2c_master;
    
    /* Reset pin definition */
    struct gpio_dt_spec rst_gpio;

    /* MFIO pin definition */
    struct gpio_dt_spec mfio_gpio;
};

struct shmax30101_data {
    /* Self-reference to the driver instance */
	const struct device *instance;

    /* buffer to store PPG data */
    char buffer[256];
    ds_pkt_data_mode1 *ppg_0_data;

    /* spo2 calibation coefficients */
    int32_t spo2_coef_a;
    int32_t spo2_coef_b;
    int32_t spo2_coef_c;
};

static int shmax30101_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    int ret = 0;
    struct shmax30101_data *data = dev->data;

    memset(data->buffer, 0x00, sizeof(data->buffer));
    int data_len = SSMAX30101Comm_data_report_execute(data->buffer, sizeof(data->buffer));
    if (data_len < 0) {
        LOG_ERR("data_len = %d", data_len);
        ret = -1;
    } else if (data_len == 0) {
        LOG_DBG("no data");
        ret = -1;
    } else {
#ifdef ASCII_COMM
        LOG_DBG("%s", data->buffer);
#else
        data->ppg_0_data = (ds_pkt_data_mode1 *)data->buffer;
        LOG_DBG("HR = %d, HR conf = %d", data->ppg_0_data->hr, data->ppg_0_data->hr_conf);
#endif
    }
    
    return ret;
}

static int shmax30101_channel_get(const struct device *dev, enum sensor_channel chan, 
                                    struct sensor_value *valp)
{
    struct shmax30101_data *data = dev->data;

    int ret = 0;
    int channel = chan;

    switch (channel)
    {
    case SENSOR_CHAN_HEART_RATE:
        valp->val1 = data->ppg_0_data->hr;
        valp->val2 = data->ppg_0_data->hr_conf;
        break;
    case SENSOR_CHAN_SPO2:
        valp->val1 = data->ppg_0_data->spo2;
        break;
    case SENSOR_CHAN_HR_AND_SPO2:
        (&valp[0])->val1 = data->ppg_0_data->hr;
        (&valp[0])->val2 = data->ppg_0_data->hr_conf;
        (&valp[1])->val1 = data->ppg_0_data->spo2;
        (&valp[2])->val1 = data->ppg_0_data->status;
        break;
    
    default:
        break;
    }
    return ret;
}

static int shmax30101_attr_set(const struct device *dev, 
                                enum sensor_channel chan,
			                    enum sensor_attribute attr, 
                                const struct sensor_value *val)
{
    struct shmax30101_data *data = dev->data;

    int ret = 0;
    int sens_attr = (int) attr;
    char cmd[256] = {0x00};

    switch (sens_attr) {
        case SENSOR_ATTR_READ_PPG_0:
        {
            SSMAX30101Comm_parse_command("read ppg 0");
        } break;

        case SENSOR_ATTR_READ_BPT_0:
        {
            SSMAX30101Comm_parse_command("read bpt 0");
        } break;
        
        case SENSOR_ATTR_READ_BPT_1:
        {
            SSMAX30101Comm_parse_command("read bpt 1");
        } break;
        
        case SENSOR_ATTR_SET_AGC_DIS:
        {
            SSMAX30101Comm_parse_command("set_cfg ppg agc 0");
        } break;

        case SENSOR_ATTR_SET_AGC_EN:
        {
            SSMAX30101Comm_parse_command("set_cfg ppg agc 1");
        } break;

        case SENSOR_ATTR_SET_CFG_BPT_MED:
        {
            // strcpy(cmd, "set_cfg bpt med");
            int len = 0; //strlen(cmd);
            len += sprintf(cmd+len, "%s", "set_cfg bpt med");
            len += sprintf(cmd+len, " %d", val->val1);
            SSMAX30101Comm_parse_command(cmd);
        } break;
        
        case SENSOR_ATTR_SET_CFG_BPT_SYS_BP:
        {
            // strcpy(cmd, "set_cfg bpt sys_bp");
            int len = 0; //strlen(cmd);
            len += sprintf(cmd+len, "%s", "set_cfg bpt sys_bp");
            len += sprintf(cmd+len, " %d", (&val[0])->val1);
            len += sprintf(cmd+len, " %d", (&val[1])->val1);
            len += sprintf(cmd+len, " %d", (&val[2])->val1);
            SSMAX30101Comm_parse_command(cmd);
        } break;

        case SENSOR_ATTR_SET_CFG_BPT_DIA_BP:
        {
            int len = 0;
            len += sprintf(cmd+len, "%s", "set_cfg bpt dia_bp");
            len += sprintf(cmd+len, " %d", (&val[0])->val1);
            len += sprintf(cmd+len, " %d", (&val[1])->val1);
            len += sprintf(cmd+len, " %d", (&val[2])->val1);
            SSMAX30101Comm_parse_command(cmd);
        } break;

        case SENSOR_ATTR_SET_CFG_BPT_DATE:
        {
            int len = 0;
            len += sprintf(cmd+len, "%s", "set_cfg bpt date");
            len += sprintf(cmd+len, " %d", (&val[0])->val1);
            len += sprintf(cmd+len, " %d", (&val[1])->val1);
            SSMAX30101Comm_parse_command(cmd);
        } break;
        
        case SENSOR_ATTR_SET_CFG_BPT_NONREST:
        {
            int len = 0;
            len += sprintf(cmd+len, "%s", "set_cfg bpt nonrest");
            len += sprintf(cmd+len, " %d", val->val1);
            SSMAX30101Comm_parse_command(cmd);
        } break;

        case SENSOR_ATTR_SET_CFG_SPO2_CAL:
        {
            data->spo2_coef_a = (&val[0])->val1;
            data->spo2_coef_b = (&val[1])->val1;
            data->spo2_coef_c = (&val[2])->val1;
            uint8_t algo_idx = (&val[3])->val1;
            SSMAX30101Comm_SPO2_calCoef_set(data->spo2_coef_a, data->spo2_coef_b, data->spo2_coef_c, algo_idx);
        } break;
        
        // case SENSOR_ATTR_SET_CFG_SPO2_CAL_C:
        // {
        //     data->spo2_coef_c = val->val1;
        //     SSMAX30101Comm_SPO2_calCoef_set(data->spo2_coef_a, data->spo2_coef_b, data->spo2_coef_c);
        // } break;

        case SENSOR_ATTR_STOP_PPG_0:
        {
            SSMAX30101Comm_stop();
        } break;
    }

    return ret;
}

int shmax30101_attr_get(const struct device *dev,
				            enum sensor_channel chan,
				            enum sensor_attribute attr,
				            struct sensor_value *val)
{
    int ret = 0;
    return ret;
}

/* Init function */
static int shmax30101_init(const struct device *dev)
{
   	const struct shmax30101_config *config = dev->config;
	struct shmax30101_data *data = dev->data;
	int ret = 0;
    
    if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

    /* Store self-reference */
	data->instance = dev;

    /* initialize the SS (sensor hub) interface */
    ret = SSInterface_init(&config->i2c_master, &config->rst_gpio, &config->mfio_gpio);
	if (ret != 0) {
		LOG_ERR("SSInterface_init failed (%d)", ret);
		return ret;
	}

    SSMAX30101Comm_init();

    // return ret;
    return 0;
}

static const struct sensor_driver_api shmax30101_driver_api = {
	.sample_fetch = shmax30101_sample_fetch,
	.channel_get = shmax30101_channel_get,
	.attr_set = shmax30101_attr_set,
    .attr_get = shmax30101_attr_get
};

#define DEVICE_INSTANCE(inst) \
\
const static struct shmax30101_config shmax30101_##inst##_cfg = { \
	.i2c_master = I2C_DT_SPEC_INST_GET(inst),		                \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, reset_gpios), (	        \
		.rst_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),       \
	)) \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, mfio_gpios), (	        \
		.mfio_gpio = GPIO_DT_SPEC_INST_GET(inst, mfio_gpios),       \
	)) \
};\
\
static struct shmax30101_data shmax30101_##inst##_drvdata = { \
}; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		shmax30101_init,									\
		device_pm_control_nop,							\
		&shmax30101_##inst##_drvdata,						\
		&shmax30101_##inst##_cfg,							\
		APPLICATION, CONFIG_SHMAX30101_INIT_PRIORITY,		\
		&shmax30101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);