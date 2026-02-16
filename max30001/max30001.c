/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 11-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#define DT_DRV_COMPAT maxim_max30001

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>

#define LOG_LEVEL CONFIG_MAX30001_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max30001);

#include "max30001.h"
#include "max30001_comm.h"

static const struct spi_dt_spec *m_spi;

struct max30001_config {
	struct spi_dt_spec spi;

#ifdef CONFIG_MAX30001_INTERRUPT_B  /* Interrupt B pin definition */
	// const char *intb_port;
	// gpio_pin_t intb_pin;
	// gpio_flags_t intb_flags;
	struct gpio_dt_spec intb_gpio;
#endif
#ifdef CONFIG_MAX30001_INTERRUPT_2B  /* Interrupt 2B pin definition */
	// const char *int2b_port;
	// gpio_pin_t int2b_pin;
	// gpio_flags_t int2b_flags;
	struct gpio_dt_spec int2b_gpio;
#endif
};

struct max30001_data {
	const struct device *instance;          /* Self-reference to the driver instance */
#if CONFIG_MAX30001_INTERRUPT_B
	struct gpio_callback intb_callback;
	struct k_work int_worker;
#endif
#if CONFIG_MAX30001_INTERRUPT_2B
	struct gpio_callback int2b_callback;
	// struct k_work int2b_worker;
#endif

};

static int max30001_reg_read_spi(const struct spi_dt_spec *spi, uint8_t addr, uint32_t *return_data)
{
	int ret;
    uint8_t data_array[1];
    uint8_t result[4] = {0x00};

    data_array[0] = ((addr << 1) & 0xff) | 1; // For Read, Or with 1

	const struct spi_buf tx_buf = {
		.buf = &data_array[0],
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = &result[0],
		.len = 4
	};
    // struct spi_buf rx_buf[4];
    // rx_buf[0].buf = &result[0];
    // rx_buf[0].len = 1;
    // rx_buf[1].buf = &result[1];
    // rx_buf[1].len = 1;
    // rx_buf[2].buf = &result[2];
    // rx_buf[2].len = 1;
    // rx_buf[3].buf = &result[3];
    // rx_buf[3].len = 1;
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
        .count = 1
        // .buffers = rx_buf,
		// .count = ARRAY_SIZE(rx_buf)
	};

    ret = spi_transceive_dt(spi, &tx, &rx);
    if (ret < 0) {
        LOG_ERR("spi_transceive failed %i", ret);
        return ret;
    }

    LOG_INF("[3] 0x%x [2] 0x%x [1] 0x%x [0] 0x%x", result[3], result[2], result[1], result[0]);

    *return_data = /*result[0] + */ (uint32_t)(result[1] << 16) + (result[2] << 8) + result[3];

	// k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int max30001_reg_read_spi_burst(const struct spi_dt_spec *spi, uint8_t addr, uint8_t *rx_data, uint32_t rx_size)
{
	int ret = 0;
	uint8_t data_array[1];

	data_array[0] = ((addr << 1) & 0xff) | 1; // For Read, Or with 1

	const struct spi_buf tx_buf = {
		.buf = &data_array[0],
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = rx_size
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
        .count = 1
	};

	ret = spi_transceive_dt(spi, &tx, &rx);
    if (ret < 0) {
        LOG_ERR("spi_transceive failed %i", ret);
        return ret;
    }

	return ret;
}

static int max30001_reg_write_spi(const struct spi_dt_spec *spi, uint8_t addrw, uint32_t data)
{
	int ret;
    uint8_t data_array[4];

    data_array[0] = (addrw << 1) & 0xff;

    data_array[3] = data & 0xff;
    data_array[2] = (data >> 8) & 0xff;
    data_array[1] = (data >> 16) & 0xff;

	LOG_INF("[3] 0x%x [2] 0x%x [1] 0x%x [0] 0x%x", data_array[3], data_array[2], data_array[1], data_array[0]);

	const struct spi_buf tx_buf = {
		.buf = &data_array[0],
		.len = 4
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};

	ret = spi_write_dt(spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}

	// k_usleep(BMI270_SPI_ACC_DELAY_US);
	return 0;
}

static int max30001_reg_read(uint8_t addr, uint32_t *return_data)
{
	return max30001_reg_read_spi(m_spi, addr, return_data);
}

static int max30001_reg_read_burst(uint8_t addr, uint8_t *rx_buf, uint32_t rx_size)
{
	return max30001_reg_read_spi_burst(m_spi, addr, rx_buf, rx_size);
}

static int max30001_reg_write(uint8_t addr, uint32_t data)
{
	return max30001_reg_write_spi(m_spi, addr, data);
}

static int max30001_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    int ret = 0;
    return ret;
}

static int max30001_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
    int ret = 0;
	int channel = chan;

	switch (channel)
	{
		case SENSOR_CHAN_MAX30001_EKG_RTOR:
		{
			max30001_bledata_t data;
			max30001_comm_ReadHeartrateData(&data);
			val->val1 = data.R2R;
			val->val2 = data.fmstr;
		}
		break;	
	default:
		break;
	}
    return ret;
}

static int max30001_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
    int ret = 0;
	int sens_attr = (int) attr;

	switch (sens_attr) {
		case SENSOR_ATTR_CONFIGURATION:
		{
			uint32_t addr = val->val1;
			uint32_t data = val->val2;
			ret = max30001_comm_reg_write(addr, data);
		}
		break;

		case SENSOR_ATTR_MAX30001_SW_RESET:
		{
			max30001_comm_sw_rst();
		}
		break;

		case SENSOR_ATTR_MAX30001_ECG_INIT_START:
		{
			uint8_t En_ecg 		= (&val[0])->val1;
			uint8_t Openp		= (&val[1])->val1;
			uint8_t Openn		= (&val[2])->val1;
            uint8_t Pol			= (&val[3])->val1;
			uint8_t Calp_sel	= (&val[4])->val1;
			uint8_t Caln_sel	= (&val[5])->val1;
            uint8_t E_fit		= (&val[6])->val1;
			uint8_t Rate		= (&val[7])->val1;
			uint8_t Gain		= (&val[8])->val1;
            uint8_t Dhpf		= (&val[9])->val1;
			uint8_t Dlpf		= (&val[10])->val1;

			ret = max30001_comm_ECG_InitStart(En_ecg, Openp, Openn, Pol,
                    		Calp_sel, Caln_sel, E_fit,
                    		Rate, Gain, Dhpf, Dlpf);
		}
		break;

		case SENSOR_ATTR_MAX30001_RTOR_INIT_START:
		{
			uint8_t En_rtor		= (&val[0])->val1;
			uint8_t Wndw		= (&val[1])->val1;
			uint8_t Gain		= (&val[2])->val1;
            uint8_t Pavg		= (&val[3])->val1;
			uint8_t Ptsf		= (&val[4])->val1;
			uint8_t Hoff		= (&val[5])->val1;
            uint8_t Ravg		= (&val[6])->val1;
			uint8_t Rhsf		= (&val[7])->val1;
			uint8_t Clr_rrint	= (&val[8])->val1;

			ret = max30001_comm_RtoR_InitStart(En_rtor, Wndw, Gain,
                             Pavg, Ptsf, Hoff,
                             Ravg, Rhsf, Clr_rrint);
		}
		break;

		case SENSOR_ATTR_MAX30001_INT_ASSIGN:
		{
			max30001_intrpt_Location_t en_enint_loc			= (&val[0])->val1;
            max30001_intrpt_Location_t en_eovf_loc			= (&val[1])->val1;
            max30001_intrpt_Location_t en_fstint_loc		= (&val[2])->val1;
            max30001_intrpt_Location_t en_dcloffint_loc		= (&val[3])->val1;
            max30001_intrpt_Location_t en_bint_loc			= (&val[4])->val1;
            max30001_intrpt_Location_t en_bovf_loc			= (&val[5])->val1;
            max30001_intrpt_Location_t en_bover_loc			= (&val[6])->val1;
            max30001_intrpt_Location_t en_bundr_loc			= (&val[7])->val1;
            max30001_intrpt_Location_t en_bcgmon_loc		= (&val[8])->val1;
            max30001_intrpt_Location_t en_pint_loc			= (&val[9])->val1;
            max30001_intrpt_Location_t en_povf_loc			= (&val[10])->val1;
            max30001_intrpt_Location_t en_pedge_loc			= (&val[11])->val1;
            max30001_intrpt_Location_t en_lonint_loc		= (&val[12])->val1;
            max30001_intrpt_Location_t en_rrint_loc			= (&val[13])->val1;
            max30001_intrpt_Location_t en_samp_loc			= (&val[14])->val1;
            max30001_intrpt_type_t intb_Type				= (&val[15])->val1;
            max30001_intrpt_type_t int2b_Type				= (&val[16])->val1;

			ret = max30001_comm_INT_assignment(en_enint_loc,
                              en_eovf_loc,
                              en_fstint_loc,
                              en_dcloffint_loc,
                              en_bint_loc,
                              en_bovf_loc,
                              en_bover_loc,
                              en_bundr_loc,
                              en_bcgmon_loc,
                              en_pint_loc,
                              en_povf_loc,
                              en_pedge_loc,
                              en_lonint_loc,
                              en_rrint_loc,
                              en_samp_loc,
                              intb_Type,
                              int2b_Type);
		}
		break;

		case SENSOR_ATTR_MAX30001_SYNCH:
		{
			ret = max30001_comm_synch();
		}
		break;

		case SENSOR_ATTR_MAX30001_RBIAS_FMSTR_INIT:
		{
			uint8_t En_rbias	= (&val[0])->val1;
			uint8_t Rbiasv		= (&val[1])->val1;
			uint8_t Rbiasp		= (&val[2])->val1;
            uint8_t Rbiasn		= (&val[3])->val1;
			uint8_t Fmstr		= (&val[4])->val1;

			ret = max30001_comm_Rbias_FMSTR_Init(En_rbias, Rbiasv, Rbiasp,
                        Rbiasn, Fmstr);
		}
		break;

		case SENSOR_ATTR_MAX30001_CALLBACK:
		{
			PtrFunction cb = (PtrFunction) val->val1;
			max30001_comm_onDataAvailable(cb);
		}
		break;

	default:
		break;
	}
    return ret;
}

static int max30001_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
    int ret = 0;
	int sens_attr = (int) attr;

	switch (sens_attr) {
		case SENSOR_ATTR_CONFIGURATION:
		{
			uint32_t addr = val->val1;
			ret = max30001_comm_reg_read(addr, &val->val2);
		}
		break;

	default:
		break;
	}

    return ret;
}

#if (CONFIG_MAX30001_INTERRUPT_B || CONFIG_MAX30001_INTERRUPT_2B)
static void max30001_int_worker(struct k_work *work)
{
	max30001_comm_int_handler();
}
#endif

#ifdef CONFIG_MAX30001_INTERRUPT_B
static void max30001_intb_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct max30001_data *const drv_data = CONTAINER_OF(cb, struct max30001_data, intb_callback);

	ARG_UNUSED(pins);

	/* Cannot do read/write from ISR context, queue worker */
	k_work_submit(&drv_data->int_worker);
}
#endif  /*CONFIG_MAX30001_INTERRUPT_B*/

#ifdef CONFIG_MAX30001_INTERRUPT_2B
// static void max30001_int2b_worker(struct k_work *work)
// {

// }
static void max30001_int2b_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins)
{
	struct max30001_data *const drv_data = CONTAINER_OF(cb, struct max30001_data, int2b_callback);

	ARG_UNUSED(pins);

	/* Cannot do read/write from ISR context, queue worker */
	k_work_submit(&drv_data->int_worker);
}
#endif  /*CONFIG_MAX30001_INTERRUPT_2B*/

static int max30001_init(const struct device *dev)
{
	const struct max30001_config *config = dev->config;
	struct max30001_data *data = dev->data;
	int ret = 0;

	if (!spi_is_ready(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	}

    /* Store self-reference */
	data->instance = dev;

	/* Store spi instance */
	m_spi = &config->spi;

	/* Check part id */
    uint32_t id;
    int part_version;

    /* the example app specifically states the id has to be read twice */
    ret = max30001_reg_read_spi(&config->spi, 0x0F, &id);
    ret = max30001_reg_read_spi(&config->spi, 0x0F, &id);
    if (ret < 0 ) {
        LOG_ERR("max30001_reg_read_spi failed");
        return ret;
    }

    part_version = (id >> 12) & 0x3;
    if (part_version == 0) {
        LOG_INF("Device: MAX30004\r\n");
    } else if (part_version == 1) {
        LOG_INF("Device: MAX30001\r\n");
    } else if (part_version == 2) {
        LOG_INF("Device: MAX30002\r\n");
    } else if (part_version == 3) {
        LOG_INF("Device: MAX30003\r\n");
    }

	/* initialize max30001 comm module */
	max30001_comm_init(max30001_reg_read, max30001_reg_read_burst, max30001_reg_write);

#if (CONFIG_MAX30001_INTERRUPT_B || CONFIG_MAX30001_INTERRUPT_2B)
	/* Prepare interrupt worker */
	k_work_init(&data->int_worker, max30001_int_worker);
#endif

    /* Configure interrupt b pin */
#ifdef CONFIG_MAX30001_INTERRUPT_B
	/* Configure interrupt GPIO pin */
	if (!device_is_ready(config->intb_gpio.port)) {
		LOG_ERR("INTB GPIO device is not ready");
		return -ENODEV;
    }
	// const struct device *intb_dev = device_get_binding(config->intb_port);
	else {
		ret = gpio_pin_configure(config->intb_gpio.port, config->intb_gpio.pin,
								(GPIO_INPUT | config->intb_gpio.dt_flags));
		ret |= gpio_pin_interrupt_configure(config->intb_gpio.port, config->intb_gpio.pin, 
								(GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt pin %d (%d)", config->intb_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&data->intb_callback, max30001_intb_callback, BIT(config->intb_gpio.pin));
		gpio_add_callback(config->intb_gpio.port, &data->intb_callback);
	}
#endif  /*CONFIG_MAX30001_INTERRUPT_B*/

    /* Configure interrupt 2b pin */
#ifdef CONFIG_MAX30001_INTERRUPT_2B
	/* Prepare interrupt worker */
	// k_work_init(&data->int2b_worker, max30001_int2b_worker);

	/* Configure interrupt GPIO pin */
	if (!device_is_ready(config->int2b_gpio.port)) {
		LOG_ERR("INT2B GPIO device is not ready");
		return -ENODEV;
    }
	// const struct device *int2b_dev = device_get_binding(config->int2b_port);
	else {
		ret = gpio_pin_configure(config->int2b_gpio.port, config->int2b_gpio.pin,
								(GPIO_INPUT | config->int2b_gpio.dt_flags));
		ret |= gpio_pin_interrupt_configure(config->int2b_gpio.port, config->int2b_gpio.pin, 
								(GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt pin %d (%d)", config->int2b_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&data->int2b_callback, max30001_int2b_callback, BIT(config->int2b_gpio.pin));
		gpio_add_callback(config->int2b_gpio.port, &data->int2b_callback);
	}
#endif  /*CONFIG_MAX30001_INTERRUPT_2B*/

    return ret;
}

static const struct sensor_driver_api max30001_driver_api = {
	.sample_fetch = max30001_sample_fetch,
	.channel_get = max30001_channel_get,
	.attr_set = max30001_attr_set,
    .attr_get = max30001_attr_get,
};

#define DEVICE_INSTANCE(inst)                                           \
                                                                        \
const static struct max30001_config max30001_##inst##_cfg = {           \
    .spi = SPI_DT_SPEC_INST_GET(inst, MAX30001_SPI_OPERATION, 0),       \
	IF_ENABLED(CONFIG_MAX30001_INTERRUPT_B, (			                \
	    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, intb_gpios), (	        \
            /*.intb_port = DT_INST_GPIO_LABEL(inst, intb_gpios),	        \
            .intb_pin = DT_INST_GPIO_PIN(inst, intb_gpios),	            \
            .intb_flags = DT_INST_GPIO_FLAGS(inst, intb_gpios),*/	        \
			.intb_gpio = GPIO_DT_SPEC_INST_GET(inst, intb_gpios), 		\
	))))								                                \
	IF_ENABLED(CONFIG_MAX30001_INTERRUPT_2B, (			                \
	    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, int2b_gpios), (	        \
            /*.int2b_port = DT_INST_GPIO_LABEL(inst, int2b_gpios),	    \
            .int2b_pin = DT_INST_GPIO_PIN(inst, int2b_gpios),	        \
            .int2b_flags = DT_INST_GPIO_FLAGS(inst, int2b_gpios),*/       \
			.int2b_gpio = GPIO_DT_SPEC_INST_GET(inst, int2b_gpios), 	\
	))))								                                \
};                                                                      \
                                                                        \
static struct max30001_data max30001_##inst##_drvdata;                  \
                                                                        \
DEVICE_DT_INST_DEFINE(inst,								                \
		max30001_init,									                \
		device_pm_control_nop,							                \
		&max30001_##inst##_drvdata,						                \
		&max30001_##inst##_cfg,							                \
		APPLICATION,                                                    \
        CONFIG_MAX30001_INIT_PRIORITY,		                            \
		&max30001_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);