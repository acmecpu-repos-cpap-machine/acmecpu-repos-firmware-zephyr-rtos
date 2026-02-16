/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 11-Sep-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 *
 *  References:
 *  	https://git.ti.com/gitweb?p=ti-analog-linux-kernel/dmurphy-analog.git;a=commit;h=75997c21e9dfc0d54b7f774bfb37e6af796ff293
 */

#define DT_DRV_COMPAT ti_bq25792

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_BQ25792_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bq25792);

#include "bq25792.h"

#define HIGH	1
#define LOW		0
#define BQ25790_NUM_WD_VAL	8

/** Configuration data */
struct bq25790_init_data {
	uint32_t ichg;	/* charge current		*/
	uint32_t ilim;	/* input current		*/
	uint32_t vreg;	/* regulation voltage		*/
	uint32_t iterm;	/* termination current		*/
	uint32_t iprechg;	/* precharge current		*/
	uint32_t vlim;	/* minimum system voltage limit */
	uint32_t max_ichg;
	uint32_t max_vreg;
	uint32_t constant_charge_current_max_ua;
	uint32_t constant_charge_voltage_max_uv;
	uint32_t precharge_current_ua;
	uint32_t charge_term_current_ua;
};

struct bq25790_state {
	bool online;
	uint8_t chrg_status;
	uint8_t chrg_type;
	uint8_t health;
	uint8_t chrg_fault;
	uint8_t vbus_status;
	uint8_t fault_0;
	uint8_t fault_1;
	uint32_t vbat_adc;
	uint32_t vbus_adc;
	uint32_t ibus_adc;
	int32_t ibat_adc;
};

struct bq25792_config {
	struct i2c_dt_spec i2c_bus;
#ifdef CONFIG_BQ25792_INTERRUPT
    struct gpio_dt_spec int_gpio;		/* Interrupt pin */
#endif
    struct gpio_dt_spec stat_gpio;		/* Status pin */
    struct gpio_dt_spec ce_gpio;		/* CE pin */
};

struct bq25792_data {
	struct k_sem lock;
	struct bq25790_init_data init_data;
	struct bq25790_state state;
	uint32_t watchdog_timer;

#ifdef CONFIG_BQ25792_INTERRUPT
	const struct device *instance;
	struct gpio_callback gpio_callback;
	struct k_work int_worker;
#endif
	const struct sensor_trigger *bq_trigger;
	sensor_trigger_handler_t bq_trig_handler;
};

static int bq25790_watchdog_time[BQ25790_NUM_WD_VAL] = {0, 500, 1000, 2000,
														20000, 40000, 80000,
														160000};

static int bq25792_read_regs(const struct bq25792_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	return i2c_burst_read_dt(&config->i2c_bus, reg, data, length);
}

static int bq25792_write_regs(const struct bq25792_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	return i2c_burst_write_dt(&config->i2c_bus, reg, data, length);
}

static int bq25792_update_bits(const struct bq25792_config *config, uint8_t reg, uint8_t mask, uint8_t val)
{
    int ret=0;
    uint8_t rw_val=0;

    /* check bits */
    ret = i2c_burst_read_dt(&config->i2c_bus, reg, &rw_val, 1);
    if (ret == 0) {
        if ((rw_val & mask) == val) {
            // no need to update
            ret = 0;
        } else {
            // update bits
            rw_val = (rw_val & ~mask) | (val & mask);
            ret = i2c_burst_write_dt(&config->i2c_bus, reg, &rw_val, 1);
            if (ret == 0) {
                LOG_DBG("write [0x%x] 0x%x", reg, rw_val);
            }
        }
    }
    return ret;
}

static int bq25790_get_vbat_adc(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t vbat_adc_lsb, vbat_adc_msb;
	int vbat_adc;

	ret = bq25792_update_bits(config, BQ25790_ADC_CTRL, BQ25790_ADC_EN, BQ25790_ADC_EN);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_VBAT_MSB, &vbat_adc_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_VBAT_LSB, &vbat_adc_lsb, 1);
	if (ret)
		return ret;

	vbat_adc = (vbat_adc_msb << 8) | vbat_adc_lsb;

	return vbat_adc * BQ25790_ADC_VOLT_STEP_uV;
}

/* enable/disable the input current limit */
static int bq25790_input_curr_en_dis(const struct bq25792_config *config, struct bq25792_data *bq, int en_dis)
{
	int ret;
	uint8_t val = BQ25790_IINDPM_EN;

	LOG_INF("en_dis = %d", en_dis);

	if (en_dis == 1) { // enable input current limit
		ret = bq25792_update_bits(config, BQ25790_CHRG_CTRL_5, BQ25790_IINDPM_EN, val);
		LOG_INF("Input current limit enabled\n");
	} else if (en_dis == 0) { // disable input current limit
	    val = (~val);
	    ret = bq25792_update_bits(config, BQ25790_CHRG_CTRL_5, BQ25790_IINDPM_EN, val);
	    LOG_INF("Input current limit disabled\n");
	} else	{
		return 0;
	}
	if (ret != 0) {
		LOG_ERR("Failed to set Input current limit bit of resistor");
		return ret;
	}

	return ret ;
}

static int bq25790_get_vbus_adc(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t vbus_adc_lsb, vbus_adc_msb;
	int vbus_adc;

	ret = bq25792_update_bits(config, BQ25790_ADC_CTRL, BQ25790_ADC_EN, BQ25790_ADC_EN);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_VBUS_MSB, &vbus_adc_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_VBUS_LSB, &vbus_adc_lsb, 1);
	if (ret)
		return ret;

	vbus_adc = (vbus_adc_msb << 8) | vbus_adc_lsb;

	return vbus_adc * BQ25790_ADC_VOLT_STEP_uV;
}

static int bq25790_get_ibus_adc(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t ibus_adc_lsb, ibus_adc_msb;
	int ibus_adc;

	ret = bq25792_update_bits(config, BQ25790_ADC_CTRL, BQ25790_ADC_EN, BQ25790_ADC_EN);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_IBUS_MSB, &ibus_adc_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_IBUS_LSB, &ibus_adc_lsb, 1);
	if (ret)
		return ret;

	ibus_adc = (ibus_adc_msb << 8) | ibus_adc_lsb;

	return ibus_adc * BQ25790_ADC_CURR_STEP_uA;
}


static int bq25790_get_ibat_adc(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	int8_t ibat_adc_lsb, ibat_adc_msb;
	int ibat_adc;

	ret = bq25792_update_bits(config, BQ25790_ADC_CTRL, BQ25790_ADC_EN, BQ25790_ADC_EN);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_IBAT_MSB, &ibat_adc_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_ADC_IBAT_LSB, &ibat_adc_lsb, 1);
	if (ret)
		return ret;

	ibat_adc = (ibat_adc_msb << 8) | ibat_adc_lsb;

	return ibat_adc * BQ25790_ADC_CURR_STEP_uA;
}

static int bq25790_get_term_curr(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t reg_val;

	ret = bq25792_read_regs(config, BQ25790_TERM_CTRL, &reg_val, 1);
	if (ret)
		return ret;

	reg_val &= BQ25790_TERMCHRG_CUR_MASK;

	return reg_val * BQ25790_TERMCHRG_CURRENT_STEP_uA;
}

static int bq25790_get_prechrg_curr(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t reg_val;

	ret = bq25792_read_regs(config, BQ25790_PRECHRG_CTRL, &reg_val, 1);
	if (ret)
		return ret;

	reg_val &= BQ25790_PRECHRG_CUR_MASK;

	return reg_val * BQ25790_PRECHRG_CURRENT_STEP_uA;
}

static int bq25790_get_ichg_curr(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t ichg_lsb, ichg_msb;
	int ichg;

	ret = bq25792_read_regs(config, BQ25790_CHRG_I_LIM_LSB, &ichg_lsb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_CHRG_I_LIM_MSB, &ichg_msb, 1);
	if (ret)
		return ret;

	ichg = (ichg_msb << 8) | ichg_lsb;

	return ichg * BQ25790_ICHRG_CURRENT_STEP_uA;
}

static int bq25790_set_term_curr(const struct bq25792_config *config, struct bq25792_data *bq, int term_current)
{
	uint8_t reg_val;

	if (term_current < BQ25790_TERMCHRG_I_MIN_uA)
		term_current = BQ25790_TERMCHRG_I_MIN_uA;
	else if (term_current > BQ25790_TERMCHRG_I_MAX_uA)
		term_current = BQ25790_TERMCHRG_I_MAX_uA;

	reg_val = term_current / BQ25790_TERMCHRG_CURRENT_STEP_uA;

	return bq25792_update_bits(config, BQ25790_TERM_CTRL, BQ25790_TERMCHRG_CUR_MASK, reg_val);
}

static int bq25790_control_charging(const struct bq25792_config *config, struct bq25792_data *bq, int en_dis)
{
	int ret = 0;
	uint8_t val = BQ25790_CE_CHG_EN;

	if (!device_is_ready(config->ce_gpio.port)) {
		LOG_ERR("CE device is not ready");
    } else {
    	if (en_dis == 1) { // enable charging
    		ret = gpio_pin_set_dt(&config->ce_gpio, LOW);
    		bq25792_update_bits(config, BQ25790_CHRG_CTRL_0, BQ25790_CE_CHG_EN, val);
    	} else if (en_dis == 0) { // disable charging
    		ret = gpio_pin_set_dt(&config->ce_gpio, HIGH);
    		val = (~val);
    		bq25792_update_bits(config, BQ25790_CHRG_CTRL_0, BQ25790_CE_CHG_EN, val);
    	}
		if (ret != 0) {
			LOG_ERR("Failed to set %s pin %d (%d)", config->ce_gpio.port->name,
					config->ce_gpio.pin, ret);
			return ret;
		}
    }

	return ret;
}

static int bq25790_control_ibat_en(const struct bq25792_config *config, struct bq25792_data *bq, int en_dis)
{
	int ret;
	uint8_t val = BQ25790_IBAT_EN;
	if (en_dis == 1) { // enable ibat discharge current sensing

	} else if (en_dis == 0) { // disable ibat discharge current sensing
		val = (~val);
	}

	ret = bq25792_update_bits(config, BQ25790_CHRG_CTRL_5, BQ25790_IBAT_EN, val);
	return ret;
}

static int bq25790_set_prechrg_curr(const struct bq25792_config *config, struct bq25792_data *bq, int pre_current)
{
	uint8_t reg_val;

	if (pre_current < BQ25790_PRECHRG_I_MIN_uA)
		pre_current = BQ25790_PRECHRG_I_MIN_uA;
	else if (pre_current > BQ25790_PRECHRG_I_MAX_uA)
		pre_current = BQ25790_PRECHRG_I_MAX_uA;

	reg_val = pre_current / BQ25790_PRECHRG_CURRENT_STEP_uA;

	return bq25792_update_bits(config, BQ25790_PRECHRG_CTRL, BQ25790_PRECHRG_CUR_MASK, reg_val);
}

static int bq25790_set_ichrg_curr(const struct bq25792_config *config, struct bq25792_data *bq, int chrg_curr)
{
	int ret;
	uint8_t ichg_msb, ichg_lsb;
	int ichg;

	if (chrg_curr < BQ25790_ICHRG_I_MIN_uA)
		chrg_curr = BQ25790_ICHRG_I_MIN_uA;
	else if ( chrg_curr > bq->init_data.max_ichg)
		chrg_curr = bq->init_data.max_ichg;

	ichg = chrg_curr / BQ25790_ICHRG_CURRENT_STEP_uA;
	ichg_msb = (ichg >> 8) & 0xff;
	ret = bq25792_write_regs(config, BQ25790_CHRG_I_LIM_MSB, &ichg_msb, 1);
	if (ret)
		return ret;

	ichg_lsb = ichg & 0xff;

	return bq25792_write_regs(config, BQ25790_CHRG_I_LIM_LSB, &ichg_lsb, 1);
}

static int bq25790_set_chrg_volt(const struct bq25792_config *config, struct bq25792_data *bq, int chrg_volt)
{
	uint8_t vlim_lsb, vlim_msb;
	int vlim;
	int ret;

	if (chrg_volt < BQ25790_VREG_V_MIN_uV)
		chrg_volt = BQ25790_VREG_V_MIN_uV;
	else if (chrg_volt > bq->init_data.max_vreg)
		chrg_volt = bq->init_data.max_vreg;

	vlim = chrg_volt / BQ25790_VREG_V_STEP_uV;
	vlim_msb = (vlim >> 8) & 0xff;
	ret = bq25792_write_regs(config, BQ25790_CHRG_V_LIM_MSB, &vlim_msb, 1);
	if (ret)
		return ret;

	vlim_lsb = vlim & 0xff;

	return bq25792_write_regs(config, BQ25790_CHRG_V_LIM_LSB, &vlim_lsb, 1);;
}

static int bq25790_get_chrg_volt(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t vlim_lsb, vlim_msb;
	int chrg_volt;

	ret = bq25792_read_regs(config, BQ25790_CHRG_V_LIM_MSB, &vlim_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_CHRG_V_LIM_LSB, &vlim_lsb, 1);
	if (ret)
		return ret;

	chrg_volt = (vlim_msb << 8) | vlim_lsb;

	return chrg_volt * BQ25790_VREG_V_STEP_uV;
}

static int bq25790_set_input_volt_lim(const struct bq25792_config *config, struct bq25792_data *bq, int vindpm)
{
	int ret;
	uint8_t vlim_lsb, vlim_msb;
	int vlim;

	if (vindpm < BQ25790_VINDPM_V_MIN_uV ||
	    vindpm > BQ25790_VINDPM_V_MAX_uV)
 		return -EINVAL;

	vlim = vindpm / BQ25790_VINDPM_STEP_uV;

	vlim_msb = (vlim >> 8) & 0xff;

	ret = bq25792_write_regs(config, BQ25790_INPUT_V_LIM, &vlim_msb, 1);
	if (ret)
		return ret;

	vlim_lsb = vlim & 0xff;

	return bq25792_write_regs(config, BQ25790_INPUT_V_LIM, &vlim_lsb, 1);
}

static int bq25790_get_input_volt_lim(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t vlim;

	ret = bq25792_read_regs(config, BQ25790_INPUT_V_LIM, &vlim, 1);
	if (ret)
		return ret;

	return vlim * BQ25790_VINDPM_STEP_uV;
}

static int bq25790_set_input_curr_lim(const struct bq25792_config *config, struct bq25792_data *bq, int iindpm)
{
	int ret;
	uint8_t ilim_lsb, ilim_msb;
	int ilim;

	if (iindpm < BQ25790_IINDPM_I_MIN_uA ||
	    iindpm > BQ25790_IINDPM_I_MAX_uA)
		return -EINVAL;

	ilim = iindpm / BQ25790_IINDPM_STEP_uA;
	ilim_msb = (ilim >> 8) & 0xff;

	ret = bq25792_write_regs(config, BQ25790_INPUT_I_LIM_MSB, &ilim_msb, 1);
	if (ret)
		return ret;

	ilim_lsb = ilim & 0xff;

	return bq25792_write_regs(config, BQ25790_INPUT_I_LIM_LSB, &ilim_lsb, 1);
}

static int bq25790_get_input_curr_lim(const struct bq25792_config *config, struct bq25792_data *bq)
{
	int ret;
	uint8_t ilim_msb, ilim_lsb;
	int ilim;

	ret = bq25792_read_regs(config, BQ25790_INPUT_I_LIM_MSB, &ilim_msb, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_INPUT_I_LIM_LSB, &ilim_lsb, 1);
	if (ret)
		return ret;

	ilim = (ilim_msb << 8) | ilim_lsb;

	return ilim * BQ25790_IINDPM_STEP_uA;
}

static int bq25790_get_state(const struct bq25792_config *config, struct bq25792_data *bq,
			     struct bq25790_state *state)
{
	uint8_t chrg_stat_0, chrg_stat_1, chrg_stat_3, chrg_stat_4;
	uint8_t chrg_ctrl_0, fault_0, fault_1;
	int ret;

	ret = bq25792_read_regs(config, BQ25790_CHRG_STAT_0, &chrg_stat_0, 1);
	if (ret)
		return ret;

	state->vbus_status = chrg_stat_0 & BQ25790_VBUS_PRESENT;
	state->online = chrg_stat_0 & BQ25790_PG_STAT;

	ret = bq25792_read_regs(config, BQ25790_CHRG_STAT_1, &chrg_stat_1, 1);
	if (ret)
		return ret;

	ret = bq25792_read_regs(config, BQ25790_CHRG_CTRL_0, &chrg_ctrl_0, 1);
	if (ret)
		return ret;

	if (chrg_ctrl_0 & BQ25790_CHRG_EN)
		state->chrg_status = chrg_stat_1 & BQ25790_CHG_STAT_MSK;
	else
		state->chrg_status = BQ25790_NOT_CHRGING;

	state->chrg_type = chrg_stat_1 & BQ25790_VBUS_STAT_MSK;

	ret = bq25792_read_regs(config, BQ25790_CHRG_STAT_4, &chrg_stat_4, 1);
	if (ret)
		return ret;

	state->health = chrg_stat_4 & BQ25790_TEMP_MASK;

	ret = bq25792_read_regs(config, BQ25790_FAULT_STAT_0, &fault_0, 1);
	if (ret)
		return ret;

	state->fault_0 = fault_0;

	ret = bq25792_read_regs(config, BQ25790_FAULT_STAT_1, &fault_1, 1);
	if (ret)
		return ret;

	state->fault_1 = fault_1;

	ret = bq25792_read_regs(config, BQ25790_CHRG_STAT_3, &chrg_stat_3, 1);
	if (ret)
		return ret;

	state->vbat_adc = bq25790_get_vbat_adc(config, bq);
	state->vbus_adc = bq25790_get_vbus_adc(config, bq);
	state->ibat_adc = bq25790_get_ibat_adc(config, bq);
	state->ibus_adc = bq25790_get_ibus_adc(config, bq);

	return 0;
}

static int bq25792_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;
	return ret;
}

static int bq25792_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *valp)
{
	int ret = 0;
	return ret;
}

static int bq25792_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct bq25792_config *config = dev->config;
	struct bq25792_data *bq = dev->data;
	int ret = 0;
	int prop = (int) attr;

	switch (prop) {
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = bq25790_set_input_curr_lim(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		break;
	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = bq25790_set_input_volt_lim(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = bq25790_set_chrg_volt(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = bq25790_set_ichrg_curr(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
		ret = bq25790_set_prechrg_curr(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		ret = bq25790_set_term_curr(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_CHARGE_CONTROL:
		ret = bq25790_control_charging(config, bq, val->val1);
		break;
	case POWER_SUPPLY_PROP_IBATT_DISCHARGE_SENSING:
		ret = bq25790_control_ibat_en(config, bq, val->val1);
		break;
	case POWER_SUPPLY_INPUT_CURRENT_LIMIT_CONTROL:
				ret = bq25790_input_curr_en_dis(config, bq, val->val1);
				break;
	default:
		return -EINVAL;
	}

	return ret;
}

static int bq25792_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	const struct bq25792_config *config = dev->config;
	struct bq25792_data *bq = dev->data;
	struct bq25790_state state;
	int ret = 0;
	int prop = (int) attr;

	k_sem_take(&bq->lock, K_FOREVER);
	ret = bq25790_get_state(config, bq, &state);
	k_sem_give(&bq->lock);
	if (ret) {
		LOG_ERR("bq25790_get_state failed");
		return ret;
	}

	ret = bq25792_update_bits(config, BQ25790_ADC_CTRL, BQ25790_ADC_EN, BQ25790_ADC_EN);
	if (ret) {
		LOG_ERR("bq25792_update_bits failed");
		return ret;
	}

	switch (prop) {
	case POWER_SUPPLY_PROP_STATUS:
		if (!state.chrg_type || (state.chrg_type == BQ25790_OTG_MODE))
			val->val1 = POWER_SUPPLY_STATUS_DISCHARGING;
		else if (!state.chrg_status)
			val->val1 = POWER_SUPPLY_STATUS_NOT_CHARGING;
		else if (state.chrg_status == BQ25790_TERM_CHRG)
			val->val1 = POWER_SUPPLY_STATUS_FULL;
		else
			val->val1 = POWER_SUPPLY_STATUS_CHARGING;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		switch (state.chrg_status) {
		case BQ25790_TRICKLE_CHRG:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case BQ25790_PRECHRG:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case BQ25790_FAST_CHRG:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_FAST;
			break;
		case BQ25790_TAPER_CHRG:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_STANDARD;
			break;
		case BQ25790_TOP_OFF_CHRG:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
			break;
		case BQ25790_NOT_CHRGING:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_NONE;
			break;
		default:
			val->val1 = POWER_SUPPLY_CHARGE_TYPE_UNKNOWN;
		}
		break;
	case POWER_SUPPLY_PROP_MANUFACTURER:
//		val->strval = BQ25790_MANUFACTURER;
		break;

	case POWER_SUPPLY_PROP_MODEL_NAME:
//		val->strval = BQ25790_NAME;
		break;

	case POWER_SUPPLY_PROP_ONLINE:
		val->val1 = state.online;
		break;
	case POWER_SUPPLY_PROP_USB_TYPE:
		if (!state.chrg_type) {
			val->val1 = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
		}
		switch (state.chrg_type) {
		case BQ25790_USB_SDP:
			val->val1 = POWER_SUPPLY_USB_TYPE_SDP;
			break;
		case BQ25790_USB_CDP:
			val->val1 = POWER_SUPPLY_USB_TYPE_CDP;
			break;
		case BQ25790_USB_DCP:
			val->val1 = POWER_SUPPLY_USB_TYPE_DCP;
			break;
		case BQ25790_OTG_MODE:
			val->val1 = POWER_SUPPLY_USB_TYPE_ACA;
			break;

		default:
			val->val1 = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		if (state.fault_1 && (BQ25790_OTG_OVP | BQ25790_VSYS_OVP))
			val->val1 = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else
			val->val1 = POWER_SUPPLY_HEALTH_GOOD;

		switch (state.health) {
		case BQ25790_TEMP_HOT:
			val->val1 = POWER_SUPPLY_HEALTH_HOT;
			break;
		case BQ25790_TEMP_WARM:
			val->val1 = POWER_SUPPLY_HEALTH_WARM;
			break;
		case BQ25790_TEMP_COOL:
			val->val1 = POWER_SUPPLY_HEALTH_COOL;
			break;
		case BQ25790_TEMP_COLD:
			val->val1 = POWER_SUPPLY_HEALTH_COLD;
			break;
		}
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_VBUS_NOW:
		val->val1 = state.vbus_adc;
		break;

	case POWER_SUPPLY_PROP_CURRENT_VBUS_NOW:
		val->val1 = state.ibus_adc;
		break;

	case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		ret = bq25790_get_input_volt_lim(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		ret = bq25790_get_input_curr_lim(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;
////////////////////////////////////////////////////////////////////////////
 	case POWER_SUPPLY_PROP_VOLTAGE_VBAT_NOW:
		val->val1 = state.vbat_adc;
		break;
	case POWER_SUPPLY_PROP_CURRENT_VBAT_NOW:
		val->val1 = state.ibat_adc;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		ret = bq25790_get_ichg_curr(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->val1 = bq->init_data.max_ichg;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		ret = bq25790_get_chrg_volt(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		val->val1 = bq->init_data.max_vreg;
		break;

	case POWER_SUPPLY_PROP_PRECHARGE_CURRENT:
		ret = bq25790_get_prechrg_curr(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		ret = bq25790_get_term_curr(config, bq);
		if (ret < 0)
			return ret;

		val->val1 = ret;
		ret = 0;
		break;

	default:
		return -EINVAL;
	}
	return ret;
}

static bool bq25790_state_changed(struct bq25792_data *bq,
				  struct bq25790_state *new_state)
{
	struct bq25790_state old_state;

	k_sem_take(&bq->lock, K_FOREVER);
	old_state = bq->state;
	k_sem_give(&bq->lock);

	return (old_state.chrg_status != new_state->chrg_status ||
		old_state.chrg_fault != new_state->chrg_fault	||
		old_state.online != new_state->online		||
		old_state.health != new_state->health	||
		old_state.fault_0 != new_state->fault_0 ||
		old_state.fault_1 != new_state->fault_1 ||
		old_state.chrg_type != new_state->chrg_type ||
		old_state.vbat_adc != new_state->vbat_adc ||
		old_state.vbus_adc != new_state->vbus_adc ||
		old_state.ibat_adc != new_state->ibat_adc);
}

#ifdef CONFIG_BQ25792_INTERRUPT
static void bq25792_interrupt_worker(struct k_work *work)
{
	struct bq25792_data *bq = CONTAINER_OF(work, struct bq25792_data, int_worker);
	const struct bq25792_config *config = bq->instance->config;
	struct bq25790_state state;

	int ret = bq25790_get_state(config, bq, &state);
	if (ret < 0)
		goto irq_out;

	if (!bq25790_state_changed(bq, &state))
		goto irq_out;

	k_sem_take(&bq->lock, K_FOREVER);
	bq->state = state;
	k_sem_give(&bq->lock);

	if (bq->bq_trig_handler != NULL) {
		bq->bq_trig_handler(bq->instance, bq->bq_trigger);
	}

irq_out:
		return;
}

static void bq25792_interrupt_callback(const struct device *dev, struct gpio_callback *cb,
											gpio_port_pins_t pins)
{
	struct bq25792_data *const data = CONTAINER_OF(cb, struct bq25792_data, gpio_callback);

	ARG_UNUSED(pins);

	/* Cannot read registers from ISR context, queue worker */
	k_work_submit(&data->int_worker);
}

static int bq25792_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				    				sensor_trigger_handler_t handler)
{
//	const struct bq25792_config *config = dev->config;
	struct bq25792_data *bq = dev->data;

	if (trig->type == POWER_SUPPLY_TRIG_CHARGER_INTR) {
		bq->bq_trig_handler = handler;
		if (handler == NULL) {
			return 0;
		}
		bq->bq_trigger = trig;
	}
	return 0;
}
#endif /* CONFIG_BQ25792_INTERRUPT */


static int bq25790_hw_init(const struct device *dev)
{
	const struct bq25792_config *config = dev->config;
	struct bq25792_data *bq = dev->data;
	int ret = 0;
	uint8_t i;
	uint8_t wd_reg_val = BQ25790_WATCHDOG_DIS;

	if (bq->watchdog_timer) {
		for (i = 0; i < BQ25790_NUM_WD_VAL; i++) {
			if (bq->watchdog_timer > bq25790_watchdog_time[i] &&
			    bq->watchdog_timer < bq25790_watchdog_time[i + 1])
				wd_reg_val = i;
		}
	}

	ret = bq25792_write_regs(config, BQ25790_CHRG_CTRL_1, &wd_reg_val, 1);

	/* TODO get the battery properties dynamically */
	bq->init_data.constant_charge_current_max_ua = BQ25790_ICHRG_I_DEF_uA;
	bq->init_data.constant_charge_voltage_max_uv = BQ25790_VREG_V_DEF_uV;
	bq->init_data.precharge_current_ua = BQ25790_PRECHRG_I_DEF_uA;
	bq->init_data.charge_term_current_ua = BQ25790_TERMCHRG_I_DEF_uA;
	bq->init_data.max_ichg = BQ25790_ICHRG_I_MAX_uA;
	bq->init_data.max_vreg = BQ25790_VREG_V_MAX_uV;

	ret = bq25790_set_ichrg_curr(config, bq, bq->init_data.constant_charge_current_max_ua);
	if (ret)
		goto err_out;

	ret = bq25790_set_prechrg_curr(config, bq, bq->init_data.precharge_current_ua);
	if (ret)
		goto err_out;

	ret = bq25790_set_chrg_volt(config, bq, bq->init_data.constant_charge_voltage_max_uv);
	if (ret)
		goto err_out;

	ret = bq25790_set_term_curr(config, bq, bq->init_data.charge_term_current_ua);
	if (ret)
		goto err_out;

//	ret = bq25790_set_input_volt_lim(config, bq, bq->init_data.vlim);
//	if (ret)
//		goto err_out;

	ret = bq25790_set_input_curr_lim(config, bq, bq->init_data.ilim);
	if (ret)
		goto err_out;

	return 0;

err_out:
	return ret;
}

static int bq25792_init(const struct device *dev)
{
	const struct bq25792_config *config = dev->config;
	struct bq25792_data *data = dev->data;
	int ret = 0;

   	if (!device_is_ready(config->i2c_bus.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	/* interrupt configuration */
#ifdef CONFIG_BQ25792_INTERRUPT
	data->instance = dev;
	k_work_init(&data->int_worker, bq25792_interrupt_worker);

	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("INT device is not ready");
//		return -ENODEV;
	} else {
		ret = gpio_pin_configure(config->int_gpio.port, config->int_gpio.pin,
				(GPIO_INPUT | config->int_gpio.dt_flags));
		ret |= gpio_pin_interrupt_configure(config->int_gpio.port,
				config->int_gpio.pin, (GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure %s pin %d (%d)",
					config->int_gpio.port->name, config->int_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&data->gpio_callback, bq25792_interrupt_callback,
				BIT(config->int_gpio.pin));
		gpio_add_callback(config->int_gpio.port, &data->gpio_callback);
	}
#endif

	/* TODO: status gpio configuration */

	/* charge enable gpio configuration */
	if (!device_is_ready(config->ce_gpio.port)) {
		LOG_ERR("CE device is not ready");
    } else {
		ret = gpio_pin_configure(config->ce_gpio.port, config->ce_gpio.pin,
                                (GPIO_OUTPUT | config->ce_gpio.dt_flags));
		ret |= gpio_pin_set_dt(&config->ce_gpio, HIGH);	// keep charging disabled initially
		if (ret != 0) {
			LOG_ERR("Failed to set %s pin %d (%d)", config->ce_gpio.port->name,
					config->ce_gpio.pin, ret);
			return ret;
		}
    }

	ret = bq25790_hw_init(dev);
	if (ret != 0) {
		LOG_ERR("cannot initialize the chip, %d", ret);
		return ret;
	}

	return ret;
}

static const struct sensor_driver_api bq25792_driver_api = {
	.sample_fetch = bq25792_sample_fetch,
	.channel_get = bq25792_channel_get,
	.attr_set = bq25792_attr_set,
	.attr_get = bq25792_attr_get,
#ifdef CONFIG_BQ25792_INTERRUPT
	.trigger_set = bq25792_trigger_set,
#endif
};


#define DEVICE_INSTANCE(inst) \
\
const static struct bq25792_config bq25792_##inst##_cfg = { \
	.i2c_bus = I2C_DT_SPEC_INST_GET(inst),		       \
	IF_ENABLED(CONFIG_BQ25792_INTERRUPT, (			\
	    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios), (	\
            .int_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios), \
	))))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, status_gpios), (	\
        .stat_gpio = GPIO_DT_SPEC_INST_GET(inst, status_gpios), \
    ))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, charge_enable_gpios), (	\
        .ce_gpio = GPIO_DT_SPEC_INST_GET(inst, charge_enable_gpios), \
    ))								\
};\
static struct bq25792_data bq25792_##inst##_drvdata = { \
		.watchdog_timer = DT_PROP(DT_DRV_INST(inst),watchdog_timer), \
		.init_data.vlim = DT_PROP(DT_DRV_INST(inst),input_voltage_limit_microvolt), \
		.init_data.ilim = DT_PROP(DT_DRV_INST(inst),input_current_limit_microamp), \
}; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		bq25792_init,									\
		device_pm_control_nop,							\
		&bq25792_##inst##_drvdata,						\
		&bq25792_##inst##_cfg,							\
		APPLICATION, CONFIG_BQ25792_INIT_PRIORITY,		\
		&bq25792_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
