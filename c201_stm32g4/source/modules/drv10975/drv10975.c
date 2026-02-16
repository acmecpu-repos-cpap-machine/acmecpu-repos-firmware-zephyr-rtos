/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT ti_drv10975

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <drivers/pwm.h>
#include "drv10975.h"
#define LOG_LEVEL CONFIG_DRV10975_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(drv10975);

/* Register definitions */
#define REG_SPEED_CTRL1				0x00	/* Read Write */
#define REG_SPEED_CTRL2				0x01	/* Read Write */
#define REG_DEV_CTRL				0x02	/* Read Write */
#define REG_EE_CTRL					0x03	/* Read Write */
#define REG_STATUS					0x10	/* Read Only */
#define REG_MOTOR_SPEED1			0x11	/* Read Only */
#define REG_MOTOR_SPEED2			0x12	/* Read Only */
#define REG_MOTOR_PERIOD1			0x13	/* Read Only */
#define REG_MOTOR_PERIOD2			0x14	/* Read Only */
#define REG_MOTOR_KT1				0x15	/* Read Only */
#define REG_MOTOR_KT2				0x16	/* Read Only */
#define REG_MOTOR_CURRENT1			0x17	/* Read Only */
#define REG_MOTOR_CURRENT2			0x18	/* Read Only */
#define REG_IPD_POSITION			0x19	/* Read Only */
#define REG_SUPPLY_VOLTAGE			0x1A	/* Read Only */
#define REG_SPEED_CMD				0x1B	/* Read Only */
#define REG_SPEED_CMD_BUFFER		0x1C	/* Read Only */
#define REG_FAULT_CODE				0x1E	/* Read Only */
#define REG_MOTOR_PARAM1			0x20	/* EEPROM, Default Value - 0x4A */
#define REG_MOTOR_PARAM2			0x21	/* EEPROM, Default Value - 0x4E */
#define REG_MOTOR_PARAM3			0x22	/* EEPROM, Default Value - 0x2A */
#define REG_SYS_OPT1				0x23	/* EEPROM, Default Value - 0x00 */
#define REG_SYS_OPT2				0x24	/* EEPROM, Default Value - 0x98 */
#define REG_SYS_OPT3				0x25	/* EEPROM, Default Value - 0xE4 */
#define REG_SYS_OPT4				0x26	/* EEPROM, Default Value - 0x7A */
#define REG_SYS_OPT5				0x27	/* EEPROM, Default Value - 0xF4 */
#define REG_SYS_OPT6				0x28	/* EEPROM, Default Value - 0x69 */
#define REG_SYS_OPT7				0x29	/* EEPROM, Default Value - 0xB8 */
#define REG_SYS_OPT8				0x2A	/* EEPROM, Default Value - 0xAD */
#define REG_SYS_OPT9				0x2B	/* EEPROM, Default Value - 0x0C */

/* Motor and System parameters */
#define DRV10975_MOTOR_PARAMS1		CONFIG_DRV10975_MOTOR_PARAMS1
#define DRV10975_MOTOR_PARAMS2		CONFIG_DRV10975_MOTOR_PARAMS2
#define DRV10975_MOTOR_PARAMS3		CONFIG_DRV10975_MOTOR_PARAMS3
#define DRV10975_SYSTEM_OPTION1		CONFIG_DRV10975_SYSTEM_OPTION1
#define DRV10975_SYSTEM_OPTION2		CONFIG_DRV10975_SYSTEM_OPTION2
#define DRV10975_SYSTEM_OPTION3		CONFIG_DRV10975_SYSTEM_OPTION3
#define DRV10975_SYSTEM_OPTION4		CONFIG_DRV10975_SYSTEM_OPTION4
#define DRV10975_SYSTEM_OPTION5		CONFIG_DRV10975_SYSTEM_OPTION5
#define DRV10975_SYSTEM_OPTION6		CONFIG_DRV10975_SYSTEM_OPTION6
#define DRV10975_SYSTEM_OPTION7		CONFIG_DRV10975_SYSTEM_OPTION7
#define DRV10975_SYSTEM_OPTION8		CONFIG_DRV10975_SYSTEM_OPTION8
#define DRV10975_SYSTEM_OPTION9		CONFIG_DRV10975_SYSTEM_OPTION9

/** Configuration data */
struct drv10975_config {
	/** The master I2C device's name */
	const char * const i2c_master_dev_name;

	/** The slave address of the chip */
	uint16_t i2c_slave_addr;

	uint8_t capabilities;

	const char * speed_ctrl_dev_name;
	uint32_t speed_ctrl_pwm_pin;
	pwm_flags_t speed_ctrl_pwm_flags;

	const char * speed_fb_dev_name;

	/* Direction pin definition */
	const char *dir_gpio_port;
	gpio_pin_t dir_gpio_pin;
	gpio_flags_t dir_gpio_flags;
};

struct drv10975_data {
	/** Master I2C device */
	const struct device *i2c_master;

	struct k_sem lock;

	const struct device *speed_ctrl_dev;
	const struct device *speed_fb_dev;
};

/**
 * @brief Read a register of the DRV10975
 *
 * @param dev Device struct of the DRV10975
 * @param reg Register to read
 * @param buf Buffer to read data into
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_regs_i2c(const struct device *dev, const uint8_t reg, uint8_t *buf) {
	const struct drv10975_config *const config = dev->config;
	struct drv10975_data *const drv_data = (struct drv10975_data* const ) dev->data;
	const struct device *i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	int ret;

	ret = i2c_burst_read(i2c_master, i2c_addr, reg, (uint8_t*) buf, 1);
	if (ret != 0) {
		LOG_ERR("DRV10975[0x%X]: error reading register 0x%X (%d)", i2c_addr, reg, ret);
		return ret;
	}

	LOG_DBG("DRV10975[0x%X]: Read: REG[0x%X] = 0x%X", i2c_addr, reg, *buf);

	return 0;
}

/**
 * @brief Write to a register of the DRV10975
 *
 * @param dev Device struct of the DRV10975
 * @param reg Register to write into
 * @param value New value to set
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_regs_i2c(const struct device *dev, const uint8_t reg, const uint8_t value) {
	const struct drv10975_config *const config = dev->config;
	struct drv10975_data *const drv_data = (struct drv10975_data* const ) dev->data;
	const struct device *i2c_master = drv_data->i2c_master;
	uint16_t i2c_addr = config->i2c_slave_addr;
	int ret;

	LOG_DBG("DRV10975[0x%X]: Write: REG[0x%X] = 0x%X", i2c_addr, reg, value);

	ret = i2c_burst_write(i2c_master, i2c_addr, reg, (uint8_t*) &value, sizeof(value));
	if (ret != 0) {
		LOG_ERR("DRV10975[0x%X]: error writing to register 0x%X (%d)", i2c_addr, reg, ret);
	}

	return ret;
}

static int write_eeprom(const struct device *dev) {
	return 0;
}

static int drv10975_init(const struct device *dev)
{
	const struct drv10975_config *config = dev->config;
	struct drv10975_data *drv_data = dev->data;

	const struct device *i2c_master;

	/* Find out the device struct of the I2C master */
	i2c_master = device_get_binding((char *)config->i2c_master_dev_name);
	if (!i2c_master) {
		return -EINVAL;
	}
	drv_data->i2c_master = i2c_master;

	k_sem_init(&drv_data->lock, 1, 1);

	/* Configure GPIO direction pin */
	const struct device *dir_gpio_dev;
	dir_gpio_dev = device_get_binding(config->dir_gpio_port);
	if (dir_gpio_dev == NULL) {
		LOG_ERR("DRV10975[0x%X]: error getting direction GPIO device (%s)", config->i2c_slave_addr,
				config->dir_gpio_port);
		return -ENODEV;
	}
	int ret = gpio_pin_configure(dir_gpio_dev, config->dir_gpio_pin, (config->dir_gpio_flags | GPIO_OUTPUT));
	if (ret != 0) {
		LOG_ERR("DRV10975[0x%X]: failed to configure interrupt pin %d (%d)", config->i2c_slave_addr,
				config->dir_gpio_pin, ret);
		return ret;
	}
	return 0;
}

static int drv10975_configure(const struct device *dev) {
	int ret = -1;

	/* Configure the DRV10975 with motor parameters */
	ret = write_regs_i2c(dev, REG_EE_CTRL, 0xC0);
#if 0
	/* Motor Params */
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM1, DRV10975_MOTOR_PARAMS1);
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM2, DRV10975_MOTOR_PARAMS2);
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM3, DRV10975_MOTOR_PARAMS3);

	/* System Params */
	ret = write_regs_i2c(dev, REG_SYS_OPT1, DRV10975_SYSTEM_OPTION1);
	ret = write_regs_i2c(dev, REG_SYS_OPT2, DRV10975_SYSTEM_OPTION2);
	ret = write_regs_i2c(dev, REG_SYS_OPT3, DRV10975_SYSTEM_OPTION3);
	ret = write_regs_i2c(dev, REG_SYS_OPT4, DRV10975_SYSTEM_OPTION4);
	ret = write_regs_i2c(dev, REG_SYS_OPT5, DRV10975_SYSTEM_OPTION5);
	ret = write_regs_i2c(dev, REG_SYS_OPT6, DRV10975_SYSTEM_OPTION6);
	ret = write_regs_i2c(dev, REG_SYS_OPT7, DRV10975_SYSTEM_OPTION7);
	ret = write_regs_i2c(dev, REG_SYS_OPT8, DRV10975_SYSTEM_OPTION8);
	ret = write_regs_i2c(dev, REG_SYS_OPT9, DRV10975_SYSTEM_OPTION9);
#else
	/* Motor Params */
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM1, 0xAC);
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM2, 0x8A);
	ret = write_regs_i2c(dev, REG_MOTOR_PARAM3, 0xAA);

	/* System Params */
	ret = write_regs_i2c(dev, REG_SYS_OPT1, 0x00);
	ret = write_regs_i2c(dev, REG_SYS_OPT2, 0x98);
	ret = write_regs_i2c(dev, REG_SYS_OPT3, 0xFC);
	ret = write_regs_i2c(dev, REG_SYS_OPT4, 0x8B);
	ret = write_regs_i2c(dev, REG_SYS_OPT5, 0x00);
	ret = write_regs_i2c(dev, REG_SYS_OPT6, 0x0E);
	ret = write_regs_i2c(dev, REG_SYS_OPT7, 0x78);
	ret = write_regs_i2c(dev, REG_SYS_OPT8, 0x00);
	ret = write_regs_i2c(dev, REG_SYS_OPT9, 0x0F);
#endif
	/* Set speed control */
	ret = write_regs_i2c(dev, REG_SPEED_CTRL2, 0x80);
	ret = write_regs_i2c(dev, REG_SPEED_CTRL1, 0x50);

	if (ret != 0) {
		LOG_ERR("write_regs_i2c [0x%x, 0x2C)] failed", REG_SPEED_CTRL1);
		return -EIO;
	}

	return ret;
}

static int drv10975_enter_closed_loop(const struct device *dev) {
	int ret = -1;
	uint8_t buf=0x00;

	ret = read_regs_i2c(dev, REG_SYS_OPT9, &buf);
	ret = write_regs_i2c(dev, REG_SYS_OPT9, (buf & 0xFE));
	if (ret != 0) {
		LOG_ERR("drv10975_enter_closed_loop failed, %d", ret);
		return -EIO;
	}

	return ret;
}

static int drv10975_speed_ctrl_ana(const struct device *dev, const uint16_t speed_val) {
	return 0;
}

static int drv10975_speed_ctrl_pwm(const struct device *dev, const uint16_t speed_percent) {
	const struct drv10975_config *config = dev->config;

	const struct device *speed_pwm_dev = device_get_binding(config->speed_ctrl_dev_name);
	if (speed_pwm_dev == NULL) {
		LOG_ERR("DRV10975[0x%X]: error getting speed control pwm device (%s)", config->i2c_slave_addr,
				config->speed_ctrl_dev_name);
		return -ENODEV;
	}
	uint32_t pwm_period = (1 * 1000 * 1000) / 100;	// 100 Hz, period in usecs
	float pwm_pulse_mul = (speed_percent / 100);

	int ret = pwm_pin_set_usec(speed_pwm_dev, config->speed_ctrl_pwm_pin, pwm_period, (pwm_period * pwm_pulse_mul),
			config->speed_ctrl_pwm_flags);
	if (ret != 0) {
		LOG_ERR("DRV10975[0x%X]: error setting PWM period (%s)", config->i2c_slave_addr, config->dir_gpio_port);
		return -EIO;
	}

	return 0;
}

static int drv10975_speed_ctrl_i2c(const struct device *dev, const uint16_t speed_val, const uint8_t up_down) {
//	uint8_t speed_ctrl1 = (speed_val & 0x00FF);
//	uint8_t speed_ctrl2 = ((speed_val >> 8) | DRV10975_BIT_OVERRIDE);

	int ret = -1;

	uint8_t buf=0x00;
	uint16_t speed_curr=0;
	ret = read_regs_i2c(dev, REG_SPEED_CTRL2, &buf);
	speed_curr = buf;
	speed_curr = speed_curr << 8;
	ret = read_regs_i2c(dev, REG_SPEED_CTRL1, &buf);
	speed_curr |= buf;

	if (up_down == 1) { /* Increment */
		speed_curr += speed_val;
//		if (speed_curr > 0x1FF) {
//			speed_curr = 0x1FF;
//		}
	} else if (up_down == 2) { /* Decrement */
		speed_curr -= speed_val;
//		if (speed_curr < 0) {
//			speed_curr = 0;
//		}
	}

	uint8_t speed_ctrl1 = (speed_curr & 0x00FF);
	uint8_t speed_ctrl2 = ((speed_curr >> 8) | DRV10975_BIT_OVERRIDE);

	LOG_INF("speed_ctrl1 = %d\n", speed_ctrl1);
	LOG_INF("speed_ctrl2 = %d\n", speed_ctrl2);

	ret = write_regs_i2c(dev, REG_SPEED_CTRL2, speed_ctrl2);
	ret = write_regs_i2c(dev, REG_SPEED_CTRL1, speed_ctrl1);

	return ret;
}

static int drv10975_dir_ctrl(const struct device *dev, const uint8_t dir) {
	const struct drv10975_config *config = dev->config;
//	struct drv10975_data *drv_data = dev->data;

	const struct device *dir_gpio_dev;
	dir_gpio_dev = device_get_binding(config->dir_gpio_port);
	if (dir_gpio_dev == NULL) {
		LOG_ERR("DRV10975[0x%X]: error getting direction GPIO device (%s)", config->i2c_slave_addr,
				config->dir_gpio_port);
		return -ENODEV;
	}

	/* Set the direction pin */
	int ret = gpio_pin_set(dir_gpio_dev, config->dir_gpio_pin, dir);
	if (ret != 0) {
		LOG_ERR("DRV10975[0x%X]: error setting direction GPIO (%s)", config->i2c_slave_addr, config->dir_gpio_port);
		return -EIO;
	}

	return 0;
}

static int drv10975_status_get(const struct device *dev) {
	uint8_t status = 0xFF;
	int ret = read_regs_i2c(dev, REG_STATUS, &status);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_STATUS);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_STATUS);
	}

	return status;
}

static int drv10975_speed_get(const struct device *dev, float *speed_hz) {
	uint8_t motor_speed1 = 0xFF;
	uint8_t motor_speed2 = 0xFF;
	int speed = 0;
	int ret = -1;

	ret = read_regs_i2c(dev, REG_MOTOR_SPEED1, &motor_speed1);
	ret = read_regs_i2c(dev, REG_MOTOR_SPEED2, &motor_speed2);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_MOTOR_SPEED2);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_MOTOR_SPEED2);
	}

	speed = motor_speed1;
	speed = speed << 8;
	speed |= motor_speed2;

	*speed_hz = (speed/10);

	return ret;
}

static int drv10975_motor_period_get(const struct device *dev, float *period_us) {
	uint8_t motor_period1 = 0xFF;
	uint8_t motor_period2 = 0xFF;
	int period = 0;
	int ret = -1;

	ret = read_regs_i2c(dev, REG_MOTOR_PERIOD1, &motor_period1);
	ret = read_regs_i2c(dev, REG_MOTOR_PERIOD2, &motor_period2);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_MOTOR_PERIOD1);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_MOTOR_PERIOD2);
	}

	period = motor_period1;
	period = period << 8;
	period |= motor_period2;

	*period_us = (float) period * 10;

	return ret;
}

static int drv10975_motor_kt_get(const struct device *dev, float *kt) {
	uint8_t motor_kt1 = 0xFF;
	uint8_t motor_kt2 = 0xFF;
	int motor_kt = 0;
	int ret = -1;

	ret = read_regs_i2c(dev, REG_MOTOR_KT1, &motor_kt1);
	ret = read_regs_i2c(dev, REG_MOTOR_KT2, &motor_kt2);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_MOTOR_KT1);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_MOTOR_KT2);
	}

	motor_kt = motor_kt1;
	motor_kt = motor_kt << 8;
	motor_kt |= motor_kt2;

	*kt = (float) (motor_kt / 2);
	*kt = (float) (*kt / 1442);

	return ret;
}

static int drv10975_motor_current_get(const struct device *dev, float *p_mtr_curr) {
	uint8_t motor_current1 = 0xFF;
	uint8_t motor_current2 = 0xFF;
	int current = 0;
	int ret = -1;

	ret = read_regs_i2c(dev, REG_MOTOR_CURRENT1, &motor_current1);
	ret = read_regs_i2c(dev, REG_MOTOR_CURRENT2, &motor_current2);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_MOTOR_CURRENT1);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_MOTOR_CURRENT2);
	}

	motor_current1 = motor_current1 & 0x07;	/* bit 2:0 has value */

	current = motor_current1;
	current = current << 8;
	current |= motor_current2;

	if (current >= 1023) {
		current = (current - 1023);
		*p_mtr_curr = (float) (current / 512);
		*p_mtr_curr = (float) (3 * *p_mtr_curr);
	} else {
		*p_mtr_curr = (float) (current / 512);
		*p_mtr_curr = (float) (3 * *p_mtr_curr);
	}

	return 0;
}

static int drv10975_initial_position_get(const struct device *dev) {
	uint8_t ipd_position = 0xFF;
	int ret = read_regs_i2c(dev, REG_IPD_POSITION, &ipd_position);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_IPD_POSITION);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_IPD_POSITION);
	}

	return ipd_position;
}

static int drv10975_supply_voltage_get(const struct device *dev, float *supply_volts) {
	uint8_t volt = 0xFF;
	int ret = read_regs_i2c(dev, REG_SUPPLY_VOLTAGE, &volt);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_SUPPLY_VOLTAGE);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_SUPPLY_VOLTAGE);
	}

	*supply_volts = (float) (volt * 22.8 / 256);

	return ret;
}

static int drv10975_speed_cmd_get(const struct device *dev) {
	uint8_t speed_cmd = 0xFF;
	int ret = read_regs_i2c(dev, REG_SPEED_CMD, &speed_cmd);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_SPEED_CMD);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_SPEED_CMD);
	}

	return speed_cmd;
}

static int drv10975_speed_cmd_buffer_get(const struct device *dev) {
	uint8_t speed_cmd_buf = 0xFF;
	int ret = read_regs_i2c(dev, REG_SPEED_CMD_BUFFER, &speed_cmd_buf);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_SPEED_CMD_BUFFER);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_SPEED_CMD_BUFFER);
	}

	return speed_cmd_buf;
}

static int drv10975_fault_code_get(const struct device *dev) {
	uint8_t fault_code = 0xFF;
	int ret = read_regs_i2c(dev, REG_FAULT_CODE, &fault_code);
	if (ret != 0) {
		LOG_ERR("read_regs_i2c [0x%x] failed", REG_FAULT_CODE);
		return ret;
	} else {
		LOG_DBG("read_regs_i2c [0x%x] success", REG_FAULT_CODE);
	}

	return fault_code;
}

static int drv10975_eeprom_val_get(const struct device *dev, uint8_t *buf) {
	if (buf == NULL) {
		return -EINVAL;
	}

#define DRV10975_EEPROM_SZ	12
	uint8_t eeprom[DRV10975_EEPROM_SZ] = {0x00};
	uint8_t eeprom_start_addr = REG_MOTOR_PARAM1;
	for (int i=0; i<DRV10975_EEPROM_SZ; i++) {
		int ret = read_regs_i2c(dev, eeprom_start_addr, eeprom+i);
		if (ret != 0) {
			LOG_ERR("read_regs_i2c [0x10)] failed");
		} else {
			LOG_DBG("read_regs_i2c [0x10)] success");
		}
		eeprom_start_addr++;
	}

	memcpy(buf, eeprom, DRV10975_EEPROM_SZ);

	return 0;
}

static const struct drv10975_driver_api drv10975_drv_api_funcs = {
	.configure = drv10975_configure,
	.enter_closed_loop = drv10975_enter_closed_loop,

	.speed_ctrl_ana = drv10975_speed_ctrl_ana,
	.speed_ctrl_pwm = drv10975_speed_ctrl_pwm,
	.speed_ctrl_i2c = drv10975_speed_ctrl_i2c,
	.dir_ctrl = drv10975_dir_ctrl,

	.status_get = drv10975_status_get,
	.speed_get = drv10975_speed_get,
	.motor_period_get = drv10975_motor_period_get,
	.motor_kt_get = drv10975_motor_kt_get,
	.motor_current_get = drv10975_motor_current_get,
	.initial_position_get = drv10975_initial_position_get,
	.supply_voltage_get = drv10975_supply_voltage_get,
	.speed_cmd_get = drv10975_speed_cmd_get,
	.speed_cmd_buffer_get = drv10975_speed_cmd_buffer_get,
	.fault_code_get = drv10975_fault_code_get,
	.eeprom_val_get = drv10975_eeprom_val_get,
};

#define DEVICE_INSTANCE(inst)													\
																				\
const static struct drv10975_config drv10975_##inst##_cfg = {					\
	.i2c_master_dev_name = DT_INST_BUS_LABEL(inst),								\
	.i2c_slave_addr = DT_INST_REG_ADDR(inst),									\
																				\
	.speed_ctrl_dev_name = DT_PWMS_LABEL_BY_NAME(DT_DRV_INST(inst),speed),		\
	.speed_ctrl_pwm_pin = DT_INST_PWMS_CHANNEL_BY_NAME(inst,speed),				\
	.speed_ctrl_pwm_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst,speed),				\
																				\
	.speed_fb_dev_name = DT_PWMS_LABEL_BY_NAME(DT_DRV_INST(inst),tacho),		\
																				\
	.dir_gpio_port = DT_INST_GPIO_LABEL(inst, direction_gpios),					\
	.dir_gpio_pin = DT_INST_GPIO_PIN(inst, direction_gpios),					\
	.dir_gpio_flags = DT_INST_GPIO_FLAGS(inst, direction_gpios),				\
};																				\
																				\
static struct drv10975_data drv10975_##inst##_drvdata;							\
																				\
DEVICE_DT_INST_DEFINE(inst,														\
		drv10975_init,															\
		device_pm_control_nop,													\
		&drv10975_##inst##_drvdata,												\
		&drv10975_##inst##_cfg,													\
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,							\
		&drv10975_drv_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
