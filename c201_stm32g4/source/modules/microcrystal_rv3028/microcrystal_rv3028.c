/*
 * Copyright (c) 2021 Acme CPU
 */
#define DT_DRV_COMPAT microcrystal_rv3028

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(microcrystal_rv3028, CONFIG_MICROCRYSTAL_RV3028_LOG_LEVEL);

#include "microcrystal_rv3028.h"

#define RV3028_SEC					0x00
#define RV3028_MIN					0x01
#define RV3028_HOUR					0x02
#define RV3028_WDAY					0x03
#define RV3028_DAY					0x04
#define RV3028_MONTH				0x05
#define RV3028_YEAR					0x06
#define RV3028_ALARM_MIN			0x07
#define RV3028_ALARM_HOUR			0x08
#define RV3028_ALARM_DAY			0x09
#define RV3028_STATUS				0x0E
#define RV3028_CTRL1				0x0F
#define RV3028_CTRL2				0x10
#define RV3028_EVT_CTRL				0x13
#define RV3028_TS_COUNT				0x14
#define RV3028_TS_SEC				0x15
#define RV3028_RAM1					0x1F
#define RV3028_EEPROM_ADDR			0x25
#define RV3028_EEPROM_DATA			0x26
#define RV3028_EEPROM_CMD			0x27
#define RV3028_CLKOUT				0x35
#define RV3028_OFFSET				0x36
#define RV3028_BACKUP				0x37

#define RV3028_STATUS_PORF			BIT(0)
#define RV3028_STATUS_EVF			BIT(1)
#define RV3028_STATUS_AF			BIT(2)
#define RV3028_STATUS_TF			BIT(3)
#define RV3028_STATUS_UF			BIT(4)
#define RV3028_STATUS_BSF			BIT(5)
#define RV3028_STATUS_CLKF			BIT(6)
#define RV3028_STATUS_EEBUSY		BIT(7)

#define RV3028_CLKOUT_FD_MASK		GENMASK(2, 0)
#define RV3028_CLKOUT_PORIE			BIT(3)
#define RV3028_CLKOUT_CLKSY			BIT(6)
#define RV3028_CLKOUT_CLKOE			BIT(7)

#define RV3028_CTRL1_EERD			BIT(3)
#define RV3028_CTRL1_WADA			BIT(5)

#define RV3028_CTRL2_RESET			BIT(0)
#define RV3028_CTRL2_12_24			BIT(1)
#define RV3028_CTRL2_EIE			BIT(2)
#define RV3028_CTRL2_AIE			BIT(3)
#define RV3028_CTRL2_TIE			BIT(4)
#define RV3028_CTRL2_UIE			BIT(5)
#define RV3028_CTRL2_TSE			BIT(7)

#define RV3028_EVT_CTRL_TSR			BIT(2)

#define RV3028_EEPROM_CMD_UPDATE	0x11
#define RV3028_EEPROM_CMD_WRITE		0x21
#define RV3028_EEPROM_CMD_READ		0x22

#define RV3028_EEBUSY_POLL			10000
#define RV3028_EEBUSY_TIMEOUT		100000

#define RV3028_BACKUP_TCE			BIT(5)
#define RV3028_BACKUP_TCR_MASK		GENMASK(1,0)
#define RV3028_BACKUP_BSM_MASK		GENMASK(3,2)
#define RV3028_BACKUP_DISABLED		0x00
#define RV3028_BACKUP_DSM			BIT(2)
#define RV3028_BACKUP_LSM			(BIT(2) | BIT(3))


#define OFFSET_STEP_PPT				953674

static uint16_t rv3028_trickle_resistors[] = {3000, 5000, 9000, 15000};

//struct gpios {
//	const char *ctrl;
//	gpio_pin_t pin;
//	gpio_flags_t flags;
//};

struct rv3028_config {
	/* Common structure first because generic API expects this here. */
	struct counter_config_info generic;
//	const char *bus_name;
	struct i2c_dt_spec i2c;
//	struct gpios int_gpios;
	struct gpio_dt_spec int_gpio;
	/* The slave address of the chip */
//	uint16_t i2c_slave_addr;
	uint32_t trickle_resistor_ohms;
};

struct rv3028_data {
	/* Master I2C device */
//	const struct device *i2c_master;

	struct k_sem lock;

	/* Work structures */
//	struct k_work alarm_work;

	/* Forward ISW interrupt to proper worker. */
//	struct gpio_callback isw_callback;

	/* RTC input power, Vbackup or Vdd */
	bool vbackup_pwr;
};


/******************* Utility Functions *******************/
static int read_regs_i2c(const struct device *dev, const uint8_t reg,
		uint8_t *buf) {
	const struct rv3028_config *const config = dev->config;
//	struct rv3028_data *const drv_data = (struct rv3028_data* const ) dev->data;
//	const struct device *i2c_master = drv_data->i2c_master;
//	uint16_t i2c_addr = config->i2c_slave_addr;
//	int ret;
//
//	ret = i2c_burst_read(i2c_master, i2c_addr, reg, (uint8_t*) buf, 1);
//	if (ret != 0) {
//		LOG_ERR("[0x%X]: error reading register 0x%X (%d)", i2c_addr, reg, ret);
//		return ret;
//	}
//
//	LOG_DBG("[0x%X]: Read: REG[0x%X] = 0x%X", i2c_addr, reg, *buf);
//
//	return 0;

	return i2c_burst_read_dt(&config->i2c, reg, buf, 1);
}

static int read_bulk_regs_i2c(const struct device *dev, const uint8_t reg,
		uint8_t *val, size_t val_count) {

	int ret = 0;
	for (int i = 0; i < val_count; i++) {
		ret = read_regs_i2c(dev, reg + i, val + i);
	}
	return ret;
}

static int write_regs_i2c(const struct device *dev, const uint8_t reg,
		const uint8_t value) {
	const struct rv3028_config *const config = dev->config;
//	struct rv3028_data *const drv_data = (struct rv3028_data* const ) dev->data;
//	const struct device *i2c_master = drv_data->i2c_master;
//	uint16_t i2c_addr = config->i2c_slave_addr;
//	int ret;
//
//	LOG_DBG("[0x%X]: Write: REG[0x%X] = 0x%X", i2c_addr, reg, value);
//
//	ret = i2c_burst_write(i2c_master, i2c_addr, reg, (uint8_t*) &value,
//			sizeof(value));
//	if (ret != 0) {
//		LOG_ERR("[0x%X]: error writing to register 0x%X (%d)", i2c_addr, reg,
//				ret);
//	}
//
//	return ret;

	return i2c_burst_write_dt(&config->i2c, reg, &value, sizeof(value));
}

static int write_bulk_regs_i2c(const struct device *dev, const uint8_t reg,
		uint8_t *val, size_t val_count) {

	int ret = 0;
	for (int i = 0; i < val_count; i++) {
		ret = write_regs_i2c(dev, reg + i, val[i]);
	}
	return ret;
}

static int update_bits_i2c(const struct device *dev, uint8_t reg, uint8_t mask,
									uint8_t val)
{
	int ret;
	uint8_t tmp, orig;

	ret = read_regs_i2c(dev, reg, &orig);
	if (ret != 0)
		return ret;

	tmp = orig & ~mask;
	tmp |= val & mask;

	if (tmp != orig) {
		ret = write_regs_i2c(dev, reg, tmp);
	}

	return ret;
}

static int rv3028_exit_eerd(const struct device *rv3028, uint32_t eerd)
{
	if (eerd)
		return 0;

	return update_bits_i2c(rv3028, RV3028_CTRL1, RV3028_CTRL1_EERD, 0);
}

static int rv3028_enter_eerd(const struct device *rv3028, uint32_t *eerd)
{
	uint8_t ctrl1, status;
	int ret;

	ret = read_regs_i2c(rv3028, RV3028_CTRL1, &ctrl1);
	if (ret)
		return ret;

	*eerd = ctrl1 & RV3028_CTRL1_EERD;
	if (*eerd)
		return 0;

	ret = update_bits_i2c(rv3028, RV3028_CTRL1, RV3028_CTRL1_EERD, RV3028_CTRL1_EERD);
	if (ret)
		return ret;

	ret = read_poll_timeout(rv3028, RV3028_STATUS, status,
				       !(status & RV3028_STATUS_EEBUSY),
				       RV3028_EEBUSY_POLL, RV3028_EEBUSY_TIMEOUT);
	if (ret) {
		LOG_ERR("read_poll_timeout failed, %d", ret);
		rv3028_exit_eerd(rv3028, *eerd);

		return ret;
	}

	return 0;
}

static int rv3028_update_eeprom(const struct device *rv3028, uint32_t eerd)
{
	uint8_t status;
	int ret;

	ret = write_regs_i2c(rv3028, RV3028_EEPROM_CMD, 0x0);
	if (ret)
		goto exit_eerd;

	ret = write_regs_i2c(rv3028, RV3028_EEPROM_CMD, RV3028_EEPROM_CMD_UPDATE);
	if (ret)
		goto exit_eerd;

	k_usleep(RV3028_EEBUSY_TIMEOUT);

	ret = read_poll_timeout(rv3028, RV3028_STATUS, status,
				       !(status & RV3028_STATUS_EEBUSY),
				       RV3028_EEBUSY_POLL, RV3028_EEBUSY_TIMEOUT);

exit_eerd:
	rv3028_exit_eerd(rv3028, eerd);

	return ret;
}

static int rv3028_update_cfg(const struct device *rv3028, uint8_t reg, uint8_t mask,
		uint8_t val) {
	uint32_t eerd;
	int ret;

	ret = rv3028_enter_eerd(rv3028, &eerd);
	if (ret)
		return ret;

	ret = update_bits_i2c(rv3028, reg, mask, val);
	if (ret) {
		rv3028_exit_eerd(rv3028, eerd);
		return ret;
	}

	return rv3028_update_eeprom(rv3028, eerd);
}

static int rtc_bcd2bin(uint8_t val)
{
	return (val & 0x0f) + (val >> 4) * 10;
}

static uint8_t rtc_bin2bcd(int val)
{
	return ((val / 10) << 4) + val % 10;
}

/******************* APIs and Other Functions *******************/
int rv3028_activate_vbackup(const struct device *dev) {
	struct rv3028_data *data = dev->data;

	/* The application is going into backup power, so this
	 * function is called. So we set the vbackup_pwr to true.
	 * RV3028 I2C operation is not allowed after this
	 * */
	data->vbackup_pwr = true;
	return 0;
}

int rv3028_set_time(const struct device *dev, struct tm *tm)
{
	struct rv3028_data *data = dev->data;
	if (data->vbackup_pwr) {
		LOG_WRN("Backup switchover activated, can't do I2C operation");
		return -1;
	}

	uint8_t date[7];
	int ret;

	date[RV3028_SEC]   = rtc_bin2bcd(tm->tm_sec);
	date[RV3028_MIN]   = rtc_bin2bcd(tm->tm_min);
	date[RV3028_HOUR]  = rtc_bin2bcd(tm->tm_hour);
	date[RV3028_WDAY]  = tm->tm_wday;
	date[RV3028_DAY]   = rtc_bin2bcd(tm->tm_mday);
	date[RV3028_MONTH] = rtc_bin2bcd(tm->tm_mon + 1);
	date[RV3028_YEAR]  = rtc_bin2bcd(tm->tm_year - 100);

	/*
	 * Writing to the Seconds register has the same effect as setting RESET
	 * bit to 1
	 */
	ret = write_bulk_regs_i2c(dev, RV3028_SEC, date, sizeof(date));
	if (ret) {
		LOG_ERR("write_bulk_regs_i2c failed, %d", ret);
		return ret;
	}

	ret = update_bits_i2c(dev, RV3028_STATUS, RV3028_STATUS_PORF, 0);

	return ret;
}

static int rv3028_get_time(const struct device *dev, struct tm *tm)
{
	struct rv3028_data *data = dev->data;
	if (data->vbackup_pwr) {
		LOG_WRN("Backup switchover activated, can't do I2C operation");
		return -1;
	}

	uint8_t date[7], status;
	int ret;

	ret = read_regs_i2c(dev, RV3028_STATUS, &status);
	if (ret < 0)
		return ret;

	if (status & RV3028_STATUS_PORF)
		return -EINVAL;

	ret = read_bulk_regs_i2c(dev, RV3028_SEC, date, sizeof(date));
	if (ret) {
		LOG_ERR("read_bulk failed for register 0x%x, %d", RV3028_SEC, ret);
		return ret;
	}

	tm->tm_sec  = rtc_bcd2bin(date[RV3028_SEC] & 0x7f);
	tm->tm_min  = rtc_bcd2bin(date[RV3028_MIN] & 0x7f);
	tm->tm_hour = rtc_bcd2bin(date[RV3028_HOUR] & 0x3f);
	tm->tm_wday = date[RV3028_WDAY] & 0x7f;
	tm->tm_mday = rtc_bcd2bin(date[RV3028_DAY] & 0x3f);
	tm->tm_mon  = rtc_bcd2bin(date[RV3028_MONTH] & 0x1f) - 1;
	tm->tm_year = rtc_bcd2bin(date[RV3028_YEAR]) + 100;

	return 0;
}

static int rv3028_init(const struct device *dev)
{
	struct rv3028_data *data = dev->data;
	const struct rv3028_config *config = dev->config;
	int ret = 0;
	uint8_t status=0;
	uint32_t ohms = rv3028_trickle_resistors[0];

	/* The driver init function is called when the main
	 * power is available, so we set backup power
	 * boolean to false. When vbackup_pwr=true, no
	 * I2C operation must be performed. I2C must be
	 * terminated in a proper way when switchover happens
	 * see datasheet section 5.10
	 * */
	data->vbackup_pwr = false;

//	const struct device *i2c_master;
//	/* Find out the device struct of the bus */
//	i2c_master = device_get_binding((char *)config->bus_name);
//	if (!i2c_master) {
//		return -EINVAL;
//	}
//	data->i2c_master = i2c_master;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* Initialize the lock */
	k_sem_init(&data->lock, 1, 1);

	/* Read the status */
	ret = read_regs_i2c(dev, RV3028_STATUS, &status);
	if (ret != 0) {
		LOG_ERR("read failed for register 0x%x, %d", RV3028_STATUS, ret);
		return ret;
	}

	if (status & RV3028_STATUS_AF)
		LOG_WRN("An alarm may have been missed.");

	if (status & RV3028_STATUS_PORF) {
		LOG_WRN("POR detected");
		/* set PORF flag to 0 */
		ret = update_bits_i2c(dev, RV3028_STATUS, RV3028_STATUS_PORF, 0);
		if (ret) {
			LOG_ERR("update_bits_i2c failed for register 0x%x, bitmask 0x%lx, %d",
					RV3028_STATUS, RV3028_STATUS_PORF, ret);
			return ret;
		}
	}
#if CONFIG_RV3028_INTERRUPT
	/* TODO configure interrupt gpio and interrupt handler */
#endif


	/* select date alarm */
	ret = update_bits_i2c(dev, RV3028_CTRL1,
							RV3028_CTRL1_WADA, RV3028_CTRL1_WADA);
	if (ret) {
		LOG_ERR("update_bits_i2c failed for register 0x%x, bitmask 0x%lx, %d",
				RV3028_CTRL1, RV3028_CTRL1_WADA, ret);
		return ret;
	}

#if CONFIG_RV3028_EVENT_TIMESTAMP
	/* setup timestamping */
	ret = update_bits_i2c(dev, RV3028_CTRL2,
				 RV3028_CTRL2_EIE | RV3028_CTRL2_TSE,
				 RV3028_CTRL2_EIE | RV3028_CTRL2_TSE);
	if (ret) {
		LOG_ERR("update_bits_i2c failed for register 0x%x, bitmask 0x%x, %d",
				RV3028_CTRL2, RV3028_CTRL2_EIE | RV3028_CTRL2_TSE, ret);
		return ret;
	}
#endif

	/* setup backup switchover mode */
	ret = rv3028_update_cfg(dev, RV3028_BACKUP, RV3028_BACKUP_BSM_MASK,
			RV3028_BACKUP_LSM);
	if (ret)
		return ret;

	/* setup trickle charger */
	if (config->trickle_resistor_ohms != 0) {
		ohms = config->trickle_resistor_ohms;
	}
	int i;
	for (i = 0; i < ARRAY_SIZE(rv3028_trickle_resistors); i++)
		if (ohms == rv3028_trickle_resistors[i])
			break;

	if (i < ARRAY_SIZE(rv3028_trickle_resistors)) {
		ret = rv3028_update_cfg(dev, RV3028_BACKUP, RV3028_BACKUP_TCE |
		RV3028_BACKUP_TCR_MASK, RV3028_BACKUP_TCE | i);
		if (ret)
			return ret;
	} else {
		LOG_WRN("invalid trickle resistor value");
	}

	return ret;
}

static int rv3028_counter_start(const struct device *dev)
{
	return -EALREADY;
}

static int rv3028_counter_stop(const struct device *dev)
{
	return -ENOTSUP;
}

static int rv3028_counter_get_value(const struct device *dev,
				    uint32_t *ticks)
{
	int ret=0;
	struct tm now;
	time_t ts;

	struct rv3028_data *data = dev->data;

	k_sem_take(&data->lock, K_FOREVER);
	ret = rv3028_get_time(dev, &now);
	k_sem_give(&data->lock);

	if (ret) {
		LOG_ERR("rv3028_get_time failed, %d", ret);
		return ret;
	}

	ts = timeutil_timegm(&now);
	*ticks = ts;

	return 0;
}

static int rv3028_counter_set_alarm(const struct device *dev,
			     uint8_t id,
			     const struct counter_alarm_cfg *alarm_cfg)
{
	return -ENOTSUP;
}

static int rv3028_counter_cancel_alarm(const struct device *dev,
				       uint8_t id)
{
	return -ENOTSUP;
}

static int rv3028_counter_set_top_value(const struct device *dev,
					const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static uint32_t rv3028_counter_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t rv3028_counter_get_top_value(const struct device *dev)
{
	return UINT32_MAX;
}

static const struct counter_driver_api rv3028_api = {
	.start = rv3028_counter_start,
	.stop = rv3028_counter_stop,
	.get_value = rv3028_counter_get_value,
	.set_alarm = rv3028_counter_set_alarm,
	.cancel_alarm = rv3028_counter_cancel_alarm,
	.set_top_value = rv3028_counter_set_top_value,
	.get_pending_int = rv3028_counter_get_pending_int,
	.get_top_value = rv3028_counter_get_top_value,
};

static const struct rv3028_config rv3028_0_config = {
	.generic = {
		.max_top_value = UINT32_MAX,
		.freq = 1,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1,
	},
//	.bus_name = DT_INST_BUS_LABEL(0),
	.i2c = I2C_DT_SPEC_INST_GET(0),

#if DT_INST_NODE_HAS_PROP(0, int_gpios)
//	.int_gpios = {
//		DT_INST_GPIO_LABEL(0, int_gpios),
//		DT_INST_GPIO_PIN(0, int_gpios),
//		DT_INST_GPIO_FLAGS(0, int_gpios),
//	},
	.int_gpio = GPIO_DT_SPEC_INST_GET(0, int_gpios),
#endif
#if DT_INST_NODE_HAS_PROP(0, trickle_resistor_ohms)
	.trickle_resistor_ohms = DT_PROP(DT_DRV_INST(0),trickle_resistor_ohms),
#endif
//	.i2c_slave_addr = DT_INST_REG_ADDR(0),
};

static struct rv3028_data rv3028_0_data;

#if CONFIG_MICROCRYSTAL_RV3028_INIT_PRIORITY <= CONFIG_I2C_INIT_PRIORITY
#error CONFIG_MICROCRYSTAL_RV3028_INIT_PRIORITY must be greater than I2C_INIT_PRIORITY
#endif

DEVICE_DT_INST_DEFINE(0, rv3028_init, device_pm_control_nop, &rv3028_0_data,
		    &rv3028_0_config,
		    POST_KERNEL, CONFIG_MICROCRYSTAL_RV3028_INIT_PRIORITY,
		    &rv3028_api);
