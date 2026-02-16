/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT ti_bq25611d

#define CONFIG_BQ25611D_HAS_ANALOG	0

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#if CONFIG_BQ25611D_HAS_ANALOG
#include <zephyr/drivers/adc.h>
#endif
#define LOG_LEVEL CONFIG_BQ25611D_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bq25611d);

#include "bq25611d.h"
#include "bq25611d_bits.h"

#define BQ25611D_PART_INFO						0x50	/* x1010xxx */

/* Register definitions */
#define REG00_INPUT_CURRENT_LIMIT				0x00	/* Read Write */
#define REG01_CHARGER_CONTROL0					0x01	/* Read Write */
#define REG02_CHARGE_CURRENT_LIMIT				0x02	/* Read Write */
#define REG03_PRECRG_TERM_CURRENT_LIMIT			0x03	/* Read Write */
#define REG04_BATT_VOLTAGE_LIMIT				0x04	/* Read Write */
#define REG05_CHARGER_CONTROL1					0x05	/* Read Write */
#define REG06_CHARGER_CONTROL2					0x06	/* Read Write */
#define REG07_CHARGER_CONTROL3					0x07	/* Read Write */
#define REG08_CHARGER_STATUS0					0x08	/* Read Only */
#define REG09_CHARGER_STATUS1					0x09	/* Read Only */
#define REG0A_CHARGER_STATUS2					0x0A	/* Read Only */
#define REG0B_PART_INFORMATION					0x0B	/* Read Only */
#define REG0C_CHARGER_CONTROL4					0x0C	/* Read Write */

#define REG00_INDPM_MASK						GENMASK(4,0)
#define REG02_ICHG_MASK							GENMASK(5,0)

#if CONFIG_BQ25611D_HAS_ANALOG
/* Voltage sensing constants and variable */
#define ADC_RESOLUTION				12		/* ADC resolution of battery voltage channel */
#define ADC_GAIN						ADC_GAIN_1
#define ADC_REFERENCE					ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME			ADC_ACQ_TIME_DEFAULT

/* VBAT channel configurations with channel not yet filled in */
static struct adc_channel_cfg m_batt_mvolt_chcfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = 0, // gets set in runtime
	.differential	  = 0,
};

/* VBUS channel configurations with channel not yet filled in */
static struct adc_channel_cfg m_vbus_mvolt_chcfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = 0, // gets set in runtime
	.differential	  = 0,
};

static int batsense_enable(const struct device *dev);
static int batsense_disable(const struct device *dev);
#endif	/*CONFIG_BQ25611D_HAS_ANALOG*/

/* Configuration data */
struct bq25611d_config {
	/** The master I2C device's name */
	// const char * const i2c_master_dev_name;

	/* The slave address of the chip */
	// uint16_t i2c_slave_addr;

	struct i2c_dt_spec i2c_master;

	/* Charger status PWM IC pin definition */
	const char * bc_status_dev_name;
	uint32_t bc_status_pwm_pin;
	pwm_flags_t bc_status_pwm_flags;

	/* Enable pin definition */
	// const char *en_gpio_port;
	// gpio_pin_t en_gpio_pin;
	// gpio_flags_t en_gpio_flags;
	struct gpio_dt_spec en_gpio;

	/* Interrupt pin definition */
	// const char *intr_gpio_port;
	// gpio_pin_t intr_gpio_pin;
	// gpio_flags_t intr_gpio_flags;
	struct gpio_dt_spec int_gpio;

#if CONFIG_BQ25611D_HAS_ANALOG
	/* Battery Sense Enable pin definition */
	// const char *batsense_en_gpio_port;
	// gpio_pin_t batsense_en_gpio_pin;
	// gpio_flags_t batsense_en_gpio_flags;
	struct gpio_dt_spec batsense_en_gpio;

	/* Battery voltage ADC channel definition */
	const char *batt_volt_adc_name;
	uint8_t batt_volt_adc_ch;
	
	/* VBUS voltage ADC channel definition */
	const char *vbus_volt_adc_name;
	uint8_t vbus_volt_adc_ch;
#endif
};

struct bq25611d_data {
	/* Master I2C device */
	const struct device *i2c_master;
	struct k_sem lock;

	bq25611d_status_t charger_status;
	bq25611d_intr_handler_t handler;

#if CONFIG_BQ25611D_INTERRUPT
	/* Self-reference to the driver instance */
	const struct device *instance;
	struct gpio_callback gpio_callback;
	struct k_work interrupt_worker;
#endif
};

/*
 * @brief Read a register of the bq25611d
 *
 * @param dev Device struct of the bq25611d
 * @param reg Register to read
 * @param buf Buffer to read data into
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_regs_i2c(const struct device *dev, const uint8_t reg, uint8_t *buf, uint32_t length) {
	const struct bq25611d_config *const config = dev->config;
	// struct bq25611d_data *const drv_data = dev->data;
	// const struct device *i2c_master = drv_data->i2c_master;
	// uint16_t i2c_addr = config->i2c_slave_addr;
	int ret = 0;

	// ret = i2c_burst_read(i2c_master, i2c_addr, reg, (uint8_t*) buf, 1);
	ret = i2c_burst_read_dt(&config->i2c_master, reg, buf, length);
	if (ret != 0) {
		LOG_ERR("[0x%X]: error reading register 0x%X (%d)", config->i2c_master.addr, reg, ret);
		return ret;
	}

	LOG_DBG("[0x%X]: Read: REG[0x%X] = 0x%X", config->i2c_master.addr, reg, *buf);

	return ret;
}

/*
 * @brief Write to a register of the bq25611d
 *
 * @param dev Device struct of the bq25611d
 * @param reg Register to write into
 * @param value New value to set
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_regs_i2c(const struct device *dev, const uint8_t reg, const uint8_t *data, uint32_t length) {
	const struct bq25611d_config *const config = dev->config;
	// struct bq25611d_data *const drv_data = dev->data;
	// const struct device *i2c_master = drv_data->i2c_master;
	// uint16_t i2c_addr = config->i2c_slave_addr;
	int ret;

	LOG_DBG("[0x%X]: Write: REG[0x%X] = 0x%X", config->i2c_master.addr, reg, *data);

	// ret = i2c_burst_write(i2c_master, i2c_addr, reg, (uint8_t*) &value, sizeof(value));
	ret = i2c_burst_write_dt(&config->i2c_master, reg, data, length);
	if (ret != 0) {
		LOG_ERR("[0x%X]: error writing to register 0x%X (%d)", config->i2c_master.addr, reg, ret);
	}

	return ret;
}

static int read_charger_status(const struct device *dev, bq25611d_status_t *p_status) {
	int ret = 0;

	ret |= read_regs_i2c(dev, REG08_CHARGER_STATUS0, (uint8_t*)&(p_status->chrg_status0), 1);
	ret |= read_regs_i2c(dev, REG09_CHARGER_STATUS1, (uint8_t*)&(p_status->chrg_status1), 1);
	ret |= read_regs_i2c(dev, REG0A_CHARGER_STATUS2, (uint8_t*)&(p_status->chrg_status2), 1);

	return ret;
}

#ifdef CONFIG_BQ25611D_INTERRUPT
static void bq25611d_interrupt_worker(struct k_work *work) {
	struct bq25611d_data *const drv_data = CONTAINER_OF(work, struct bq25611d_data, interrupt_worker);
	int ret = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Read and store charger status */
	ret = read_charger_status(drv_data->instance, &drv_data->charger_status);
	if (ret != 0) {
		LOG_ERR("Failed to read charger status (%d)", ret);
		goto err;
	}

	/* Call the higher level callback */
	if (drv_data->handler != NULL) {
		drv_data->handler(drv_data->instance, &drv_data->charger_status);
	}

err:
	k_sem_give(&drv_data->lock);
}

static void bq25611d_interrupt_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins) {
	struct bq25611d_data *const drv_data = CONTAINER_OF(cb, struct bq25611d_data, gpio_callback);

	ARG_UNUSED(pins);

	/* Cannot read BQ25611D registers from ISR context, queue worker */
	k_work_submit(&drv_data->interrupt_worker);
}
#endif /* CONFIG_BQ25611D_INTERRUPT */

static int bq25611d_status_get(const struct device *dev, bq25611d_status_t *charger_status) {
	struct bq25611d_data *drv_data = dev->data;
	int ret = 0;

	/* Read and store charger status */
	ret |= read_charger_status(dev, &drv_data->charger_status);
	if (ret != 0) {
		LOG_ERR("Failed to read charger status (%d)", ret);
		return ret;
	}

	memcpy(charger_status, &drv_data->charger_status, sizeof(bq25611d_status_t));

	return ret;
}

static int bq25611d_enter_ship_mode(const struct device *dev, bool delay, uint32_t *ms_to_off) {
	int ret = 0;
	uint8_t val = 0x00;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG07_CHARGER_CONTROL3, &buf, sizeof(buf));

	if (delay) {
//		val = (BQ25611D_BIT_MASK_BATFET_DIS | BQ25611D_BIT_MASK_BATFET_DLY);
		val = buf | (BQ25611D_BIT_MASK_BATFET_DIS | BQ25611D_BIT_MASK_BATFET_DLY);
	}
	else {
//		val = (BQ25611D_BIT_MASK_BATFET_DIS);
		val = buf | BQ25611D_BIT_MASK_BATFET_DIS;
		val = val & (~BQ25611D_BIT_MASK_BATFET_DLY);
	}

	ret |= write_regs_i2c(dev, REG07_CHARGER_CONTROL3, &val, sizeof(val));
	if (ret != 0) {
		LOG_ERR("write_regs_i2c [0x%x, 0x%x)] failed", REG07_CHARGER_CONTROL3, val);
		return -EIO;
	}

	if (delay)
		*ms_to_off = BQ25611D_TBATFET_DLY_MS_MIN;
	else
		*ms_to_off = 0;

	return ret;
}

static int bq25611d_exit_ship_mode(const struct device *dev) {
	int ret = 0;
	uint8_t val = 0x00;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG00_INPUT_CURRENT_LIMIT, &buf, sizeof(buf));

	val = BQ25611D_BIT_MASK_EN_HIZ;
	val = ~val;

	val = buf & val;
	ret |= write_regs_i2c(dev, REG00_INPUT_CURRENT_LIMIT, &val, sizeof(val));
	if (ret != 0) {
		LOG_ERR("write_regs_i2c [0x%x, 0x%x)] failed", REG00_INPUT_CURRENT_LIMIT, val);
		return -EIO;
	}

	return ret;
}

static int bq25611d_full_system_reset(const struct device *dev) {
	int ret = 0;
	/* TODO */
	return ret;
}

static int bq25611d_chrg_curr_limit_set(const struct device *dev, uint8_t chrg_curr_lim) {
	int ret = 0;
	uint8_t val = 0x00;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG02_CHARGE_CURRENT_LIMIT, &buf, sizeof(buf));
	val = (buf & 0b11000000) | chrg_curr_lim;
	ret |= write_regs_i2c(dev, REG02_CHARGE_CURRENT_LIMIT, &val, sizeof(val));
	if (ret != 0) {
		LOG_ERR("write_regs_i2c [0x%x, 0x%x)] failed", REG02_CHARGE_CURRENT_LIMIT, val);
		return -EIO;
	}
	return ret;
}

static int bq25611d_chrg_curr_setting_get(const struct device *dev, uint32_t *chrg_curr_setting) {
	int ret = 0;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG02_CHARGE_CURRENT_LIMIT, &buf, sizeof(buf));
	uint8_t bmask = REG02_ICHG_MASK;
	buf = (buf & bmask);

	uint32_t ichg_val[] = {60, 120, 240, 480, 960, 1920};
	int n = sizeof(ichg_val) / sizeof(ichg_val[0]);
	uint8_t mask = 0x01;
	uint32_t curr_setting=0;

	for (int i=0; i<n; i++) {
		if (buf & mask) {
			curr_setting += ichg_val[i];
		}
		mask = mask << 1;
	}

	*chrg_curr_setting = curr_setting;

	return ret;
}

static int bq25611d_in_curr_setting_set(const struct device *dev, uint32_t in_curr_lim) {
	int ret = 0;
	uint8_t val = 0x00;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG00_INPUT_CURRENT_LIMIT, &buf, sizeof(buf));
	val = (buf & 0b11100000) | in_curr_lim;
	ret |= write_regs_i2c(dev, REG00_INPUT_CURRENT_LIMIT, &val, sizeof(val));
	if (ret != 0) {
		LOG_ERR("write_regs_i2c [0x%x, 0x%x)] failed", REG00_INPUT_CURRENT_LIMIT, val);
		return -EIO;
	}
	return ret;
}

static int bq25611d_in_curr_setting_get(const struct device *dev, uint32_t *in_curr_setting) {
	int ret = 0;
	uint8_t buf = 0x00;

	ret |= read_regs_i2c(dev, REG00_INPUT_CURRENT_LIMIT, &buf, sizeof(buf));
	uint8_t bmask = REG00_INDPM_MASK;
	buf = (buf & bmask);

	uint32_t iindpm_val[] = {100, 200, 400, 800, 1600};
	int n = sizeof(iindpm_val) / sizeof(iindpm_val[0]);
	uint8_t mask = 0x01;
	uint32_t iindpm_setting=0;

	for (int i=0; i<n; i++) {
		if (buf & mask) {
			iindpm_setting += iindpm_val[i];
		}
		mask = mask << 1;
	}

	*in_curr_setting = iindpm_setting;

	return ret;
}

static int bq25611d_intr_handler_set(const struct device *dev, bq25611d_intr_handler_t handler) {
	struct bq25611d_data *drv_data = dev->data;
	drv_data->handler = handler;
	return 0;
}

#if CONFIG_BQ25611D_HAS_ANALOG
/* Battery voltage sensing functions */
static uint32_t raw_to_mvolt(uint16_t raw) {
	int multip = 256;

	switch (ADC_RESOLUTION) {
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

	uint32_t mvolt_out = ((raw * BQ25611D_ADC_REF_VOLTAGE / multip) * 1000);
	return mvolt_out;
}

static uint32_t adc_source_mvolts_from_resistor_divider(uint32_t adc_mvolts, uint32_t r1, uint32_t r2) {
	// Vs = Vout(R1+R2)/R2

//	uint32_t r1 = BQ25611D_VBAT_ADC_VOLT_DIVIDER_R1;
//	uint32_t r2 = BQ25611D_VBAT_ADC_VOLT_DIVIDER_R2;

	uint32_t actual_mvolts = ((adc_mvolts *(r1+r2)) / r2);

	return actual_mvolts;
}

static int batsense_enable(const struct device *dev) {
	const struct bq25611d_config *config = dev->config;
//	struct bq25611d_data *drv_data = dev->data;
	int ret = 0;

	/* Configure batsense enable GPIO enable pin */
	// const struct device *en_gpio_dev = device_get_binding(config->batsense_en_gpio_port);
	if (device_is_ready(config->batsense_en_gpio.port)) {
		ret |= gpio_pin_configure(config->batsense_en_gpio.port, config->batsense_en_gpio.pin,
				(config->batsense_en_gpio.dt_flags | GPIO_OUTPUT));

		/* Enable Battery sensing */
		ret |= gpio_pin_set(config->batsense_en_gpio.port, config->batsense_en_gpio.pin, 1);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->batsense_en_gpio.pin, ret);
			return ret;
		}
	}
	return ret;
}
static int batsense_disable(const struct device *dev) {
	const struct bq25611d_config *config = dev->config;
//	struct bq25611d_data *drv_data = dev->data;
	int ret = 0;

	/* Configure batsense enable GPIO enable pin */
	// const struct device *en_gpio_dev = device_get_binding(config->batsense_en_gpio_port);
	if (device_is_ready(config->batsense_en_gpio.port)) {
		ret |= gpio_pin_configure(config->batsense_en_gpio.port, config->batsense_en_gpio.pin,
				(config->batsense_en_gpio.dt_flags | GPIO_OUTPUT));

		/* Disable Battery sensing */
		ret |= gpio_pin_set(config->batsense_en_gpio.port, config->batsense_en_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)",
					config->batsense_en_gpio.pin, ret);

			return ret;
		}
	}
	return ret;
}

static int bq25611d_batt_mvolts_get(const struct device *dev, uint32_t *batt_mvolt) {
	const struct bq25611d_config *config = dev->config;
	struct bq25611d_data *drv_data = dev->data;
	int ret = -1;

#define BAD_ANALOG_READ -123
	uint16_t adc_buff=0;
	const struct adc_sequence sequence = {
		.options     = NULL,							// extra samples and callback
		.channels    = BIT(config->batt_volt_adc_ch),	// bit mask of channels to read
		.buffer      = &adc_buff,						// where to put samples read
		.buffer_size = sizeof(adc_buff),
		.resolution  = ADC_RESOLUTION,		// desired resolution
		.oversampling = 0,								// don't oversample
		.calibrate = 0									// don't calibrate
	};

	k_sem_take(&drv_data->lock, K_FOREVER);

	const struct device *batt_mvolt_dev = device_get_binding(config->batt_volt_adc_name);
	if (batt_mvolt_dev == NULL) {
		LOG_ERR("Could not get %s device", config->batt_volt_adc_name);
		ret = -ENODEV;
		goto err;
	}

	m_batt_mvolt_chcfg.channel_id = config->batt_volt_adc_ch;
	ret = adc_channel_setup(batt_mvolt_dev, &m_batt_mvolt_chcfg);
	if (ret != 0) {
		LOG_ERR("Setup of the ADC channel %d failed with code %d", config->batt_volt_adc_ch, ret);
		goto err;
	}

	ret = adc_read(batt_mvolt_dev, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed with code %d", ret);
		goto err;
	}

	if (batt_mvolt != NULL) {
		uint32_t t_mvout = raw_to_mvolt(adc_buff);
		/* Calculate the output voltage of the battery from the acquired ADC voltage */
		*batt_mvolt = adc_source_mvolts_from_resistor_divider(t_mvout,
				BQ25611D_VBAT_ADC_VOLT_DIVIDER_R1,
				BQ25611D_VBAT_ADC_VOLT_DIVIDER_R2);
	}

err:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int bq25611d_vbus_mvolts_get(const struct device *dev, uint32_t *vbus_mvolt) {
	const struct bq25611d_config *config = dev->config;
	struct bq25611d_data *drv_data = dev->data;
	int ret = -1;

#define BAD_ANALOG_READ -123
	uint16_t adc_buff=0;
	const struct adc_sequence sequence = {
		.options     = NULL,							// extra samples and callback
		.channels    = BIT(config->vbus_volt_adc_ch),	// bit mask of channels to read
		.buffer      = &adc_buff,						// where to put samples read
		.buffer_size = sizeof(adc_buff),
		.resolution  = ADC_RESOLUTION,		// desired resolution
		.oversampling = 0,								// don't oversample
		.calibrate = 0									// don't calibrate
	};

	k_sem_take(&drv_data->lock, K_FOREVER);

	const struct device *vbus_mvolt_dev = device_get_binding(config->vbus_volt_adc_name);
	if (vbus_mvolt_dev == NULL) {
		LOG_ERR("Could not get %s device", config->vbus_volt_adc_name);
		ret = -ENODEV;
		goto err;
	}

	m_vbus_mvolt_chcfg.channel_id = config->vbus_volt_adc_ch;
	ret = adc_channel_setup(vbus_mvolt_dev, &m_vbus_mvolt_chcfg);
	if (ret != 0) {
		LOG_ERR("Setup of the ADC channel %d failed with code %d", config->vbus_volt_adc_ch, ret);
		goto err;
	}

	ret = adc_read(vbus_mvolt_dev, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed with code %d", ret);
		goto err;
	}

	if (vbus_mvolt != NULL) {
		uint32_t t_mvout = raw_to_mvolt(adc_buff);
		/* Calculate the output voltage of the VBUS line from the acquired ADC voltage */
		*vbus_mvolt = adc_source_mvolts_from_resistor_divider(t_mvout,
				BQ25611D_VBUS_ADC_VOLT_DIVIDER_R1,
				BQ25611D_VBUS_ADC_VOLT_DIVIDER_R2);
	}

err:
	k_sem_give(&drv_data->lock);
	return ret;
}
#endif	/*CONFIG_BQ25611D_HAS_ANALOG*/

static int bq25611d_init(const struct device *dev)
{
	const struct bq25611d_config *config = dev->config;
	struct bq25611d_data *drv_data = dev->data;
	int ret = 0;

	// const struct device *i2c_master;

	/* Find out the device struct of the I2C master */
	// i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	// if (!i2c_master) {
	// 	return -EINVAL;
	// }
	// drv_data->i2c_master = i2c_master;

   	if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	/* Configure GPIO enable pin */
	// const struct device *en_gpio_dev = device_get_binding(config->en_gpio_port);
	if (device_is_ready(config->en_gpio.port)) {
		ret |= gpio_pin_configure(config->en_gpio.port, config->en_gpio.pin, 
									(GPIO_OUTPUT | GPIO_PUSH_PULL | config->en_gpio.dt_flags));
		/* Battery charging is ON by default */
		ret |= gpio_pin_set(config->en_gpio.port, config->en_gpio.pin, 0);
		if (ret != 0) {
			LOG_ERR("Failed to configure enable pin %d (%d)", config->en_gpio.pin, ret);
			return ret;
		}
	}

	/* Validate part information */
	uint8_t part_info = 0x00;
	ret |= read_regs_i2c(dev, REG0B_PART_INFORMATION, &part_info, sizeof(part_info));
	if (ret != 0) {
		LOG_ERR("Failed to get part information (%d)", ret);
		return ret;
	}
	if ((part_info & BQ25611D_PART_INFO) == BQ25611D_PART_INFO) {
		LOG_INF("Part info matched");
	} else {
		LOG_ERR("Part info did not match! (0x%x)", part_info);
		return -ENXIO;
	}

	/* Read and store charger status */
	ret |= read_charger_status(dev, &drv_data->charger_status);
	if (ret != 0) {
		LOG_ERR("Failed to read charger status (%d)", ret);
		return ret;
	}
#if CONFIG_BQ25611D_HAS_ANALOG
	/* Enable the battery sensing */
	ret |= batsense_enable(dev);
	if (ret != 0) {
		LOG_ERR("Failed to enable battery voltage sensing (%d)", ret);
		return ret;
	}
#endif

#if CONFIG_BQ25611D_INTERRUPT
	/* Store self-reference for interrupt handling */
	drv_data->instance = dev;

	/* Prepare interrupt worker */
	k_work_init(&drv_data->interrupt_worker, bq25611d_interrupt_worker);

	/* Configure interrupt GPIO pin */
	// const struct device *intr_gpio_dev = device_get_binding(config->intr_gpio_port);
	if (device_is_ready(config->int_gpio.port)) {
		ret |= gpio_pin_configure(config->int_gpio.port, config->int_gpio.pin, 
									(GPIO_INPUT | config->int_gpio.dt_flags));
		ret |= gpio_pin_interrupt_configure(config->int_gpio.port, config->int_gpio.pin, 
									(GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt pin %d (%d)", config->int_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&drv_data->gpio_callback, bq25611d_interrupt_callback, BIT(config->int_gpio.pin));
		gpio_add_callback(config->int_gpio.port, &drv_data->gpio_callback);
	}
#endif

	return ret;
}

static const struct bq25611d_driver_api bq25611d_drv_api_funcs = {
	.status_get = bq25611d_status_get,
	.enter_ship_mode = bq25611d_enter_ship_mode,
	.exit_ship_mode = bq25611d_exit_ship_mode,
	.full_system_reset = bq25611d_full_system_reset,
	.chrg_curr_lim_set = bq25611d_chrg_curr_limit_set,
	.chrg_curr_setting_get = bq25611d_chrg_curr_setting_get,
	.in_curr_setting_set = bq25611d_in_curr_setting_set,
	.in_curr_setting_get = bq25611d_in_curr_setting_get,
	.intr_handler_set = bq25611d_intr_handler_set,
#if CONFIG_BQ25611D_HAS_ANALOG
	.batt_mvolts_get = bq25611d_batt_mvolts_get,
	.vbus_mvolts_get = bq25611d_vbus_mvolts_get,
#endif
};

#define DEVICE_INSTANCE(inst)													\
																				\
const static struct bq25611d_config bq25611d_##inst##_cfg = {					\
	/*.i2c_master_dev_name = DT_INST_BUS_LABEL(inst),								\
	.i2c_slave_addr = DT_INST_REG_ADDR(inst),*/									\
    .i2c_master = I2C_DT_SPEC_INST_GET(inst),		       \
																					\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, bc_status), (	\
		.bc_status_dev_name = DT_PWMS_LABEL_BY_NAME(DT_DRV_INST(inst),bc_status),	\
		.bc_status_pwm_pin = DT_INST_PWMS_CHANNEL_BY_NAME(inst,bc_status),			\
		.bc_status_pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst,bc_status),			\
	)) \
																				\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, enable_gpios), (	\
		/*.en_gpio_port = DT_INST_GPIO_LABEL(inst, enable_gpios),						\
		.en_gpio_pin = DT_INST_GPIO_PIN(inst, enable_gpios),						\
		.en_gpio_flags = DT_INST_GPIO_FLAGS(inst, enable_gpios),*/					\
		.en_gpio = GPIO_DT_SPEC_INST_GET(inst, enable_gpios), \
	)) \
\
	IF_ENABLED(CONFIG_BQ25611D_INTERRUPT, ( \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios), (	\
			/*.intr_gpio_port = DT_INST_GPIO_LABEL(inst, interrupt_gpios),				\
			.intr_gpio_pin = DT_INST_GPIO_PIN(inst, interrupt_gpios),					\
			.intr_gpio_flags = DT_INST_GPIO_FLAGS(inst, interrupt_gpios),*/				\
			.int_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios), \
	)))) \
\
	IF_ENABLED(CONFIG_BQ25611D_HAS_ANALOG, (			\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, batsense_enable_gpios), (	\
			/*.batsense_en_gpio_port = DT_INST_GPIO_LABEL(inst, batsense_enable_gpios),	\
			.batsense_en_gpio_pin = DT_INST_GPIO_PIN(inst, batsense_enable_gpios),		\
			.batsense_en_gpio_flags = DT_INST_GPIO_FLAGS(inst, batsense_enable_gpios),*/	\
			.batsense_en_gpio = GPIO_DT_SPEC_INST_GET(inst, batsense_enable_gpios), \
	)))) \
																				\
	IF_ENABLED(CONFIG_BQ25611D_HAS_ANALOG, (			\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, batt_voltage), (	\
			.batt_volt_adc_name = DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst),batt_voltage),		\
			.batt_volt_adc_ch = DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), batt_voltage),		\
	)))) \
	IF_ENABLED(CONFIG_BQ25611D_HAS_ANALOG, (			\
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, vbus_voltage), (	\
			.vbus_volt_adc_name = DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst),vbus_voltage),		\
			.vbus_volt_adc_ch = DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), vbus_voltage),		\
	)))) \
};																				\
																				\
static struct bq25611d_data bq25611d_##inst##_drvdata;							\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		bq25611d_init,															\
		device_pm_control_nop,													\
		&bq25611d_##inst##_drvdata,												\
		&bq25611d_##inst##_cfg,													\
		APPLICATION, CONFIG_BQ25611D_INIT_PRIORITY,								\
		&bq25611d_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
