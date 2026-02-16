/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_battery);

#include "acpu_c201_modules.h"

#if (CONFIG_BQ25611D) && (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
	#include "bq25611d.h"
#elif (CONFIG_BQ25890) && (CONFIG_BOARD_C204_CORE)
	#include "bq25890.h"
#endif
#if CONFIG_BQ25792
	#include "bq25792.h"
#endif
#if CONFIG_LTC294X
	#include "ltc294x.h"
#endif
#include "app_battery/bsp_battery.h"

/* static variables */
static bsp_battery_cb_handler_t m_handler = NULL;

/**/
#if (CONFIG_BQ25611D) && (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
static void charger_intr_handler(const struct device *dev,
		bq25611d_status_t *chrg_stat) {
	BSP_BATTERY_CHARGER_EVENTS charger_event = BSP_BATT_CHRG_EVENT_NONE;

/*	LOG_INF("vbus status = %d", chrg_stat->chrg_status0.vbus_stat);
	LOG_INF("charging status = %d", chrg_stat->chrg_status0.chrg_stat);
	LOG_INF("thermal regulation status = %d",
			chrg_stat->chrg_status0.therm_stat);
	LOG_INF("Vsys min regulation status = %d",
			chrg_stat->chrg_status0.vsys_stat);
	LOG_INF("watchdog fault status = %d",
			chrg_stat->chrg_status1.watchdog_fault);
	LOG_INF("boost fault status = %d", chrg_stat->chrg_status1.boost_fault);
	LOG_INF("charging fault status = %d", chrg_stat->chrg_status1.chrg_fault);
	LOG_INF("battery fault status = %d", chrg_stat->chrg_status1.bat_fault);
	LOG_INF("NTC fault status = %d", chrg_stat->chrg_status1.ntc_fault);
	LOG_INF("VBUS good status = %d", chrg_stat->chrg_status2.vbus_gd);
	LOG_INF("VINDPM status = %d", chrg_stat->chrg_status2.vindpm_stat);
	LOG_INF("IINDPM status = %d", chrg_stat->chrg_status2.iindpm_stat);
	LOG_INF("BATSNS pin status = %d", chrg_stat->chrg_status2.batsns_stat);
	LOG_INF("Top off timer status = %d", chrg_stat->chrg_status2.topoff_active);
	LOG_INF("ACOV status = %d", chrg_stat->chrg_status2.acov_stat);
	LOG_INF("VINDPN INT mask status = %d",
			chrg_stat->chrg_status2.vindpm_int_mask);
	LOG_INF("IINDPN INT mask status = %d",
			chrg_stat->chrg_status2.iindpm_int_mask);*/

	/* Check for adapter connected event */
	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_GOOD) {
		if ((chrg_stat->chrg_status0.chrg_stat == BQ25611D_STATUS_FAST_CHARGING)
				|| (chrg_stat->chrg_status0.chrg_stat
						== BQ25611D_STATUS_PRE_CHARGING)) {
			charger_event = BSP_BATT_CHRG_EVENT_ADAPTER_ATTACHED;
		}
	}

	/* Check for charge complete event */
//	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_GOOD) {
		if ((chrg_stat->chrg_status0.chrg_stat
				== BQ25611D_STATUS_CHARGE_TERMINATION)) {
			charger_event = BSP_BATT_CHRG_EVENT_CHARGE_COMPLETE;
		}
//	}

	/* Check for charger removed event */
	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_NOT_GOOD) {
		if ((chrg_stat->chrg_status0.chrg_stat == BQ25611D_STATUS_NOT_CHARGING)) {
			charger_event = BSP_BATT_CHRG_EVENT_ADAPTER_REMOVED;
		}
	}

	/* Check for faults event */
	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_GOOD) {
		if ((chrg_stat->chrg_status1.chrg_fault
				!= BQ25611D_STATUS_CHARGE_FAULT_NORMAL)
				|| (chrg_stat->chrg_status1.bat_fault
						!= BQ25611D_STATUS_BATT_FAULT_NORMAL)) {
			charger_event = BSP_BATT_CHRG_EVENT_FAULT;
		}
	}
	if (m_handler != NULL) {
		m_handler(charger_event);
	}
}
#elif (CONFIG_BQ25890) && (CONFIG_BOARD_C204_CORE)

#elif (CONFIG_BQ25792)
static void charger_intr_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	int ret = 0;
	BSP_BATTERY_CHARGER_EVENTS charger_event = BSP_BATT_CHRG_EVENT_NONE;

	struct sensor_value stat = {0,0};
	struct sensor_value type = {0,0};
	struct sensor_value health = {0,0};

	int chan = POWER_SUPPLY_CHAN_CHARGER;
	int attr = POWER_SUPPLY_PROP_STATUS;

	ret = sensor_attr_get(dev, chan, attr, &stat);
	if (ret) {
		LOG_ERR("unable to get charger attribute POWER_SUPPLY_PROP_STATUS");
		return;
	}

	attr = POWER_SUPPLY_PROP_CHARGE_TYPE;
	ret |= sensor_attr_get(dev, chan, attr, &type);
	if (ret) {
		LOG_ERR("unable to get charger attribute POWER_SUPPLY_PROP_CHARGE_TYPE");
		return;
	}

	attr = POWER_SUPPLY_PROP_HEALTH;
	ret |= sensor_attr_get(dev, chan, attr, &health);
	if (ret) {
		LOG_ERR("unable to get charger attribute POWER_SUPPLY_PROP_HEALTH");
		return;
	}

	if ((stat.val1 == POWER_SUPPLY_STATUS_CHARGING) &&
			(	(type.val1 == POWER_SUPPLY_CHARGE_TYPE_TRICKLE) ||
				(type.val1 == POWER_SUPPLY_CHARGE_TYPE_FAST) ||
				(type.val1 == POWER_SUPPLY_CHARGE_TYPE_STANDARD)
			) && (health.val1 == POWER_SUPPLY_HEALTH_GOOD))
	{
		charger_event = BSP_BATT_CHRG_EVENT_ADAPTER_ATTACHED;
	}

	if (((stat.val1 == POWER_SUPPLY_STATUS_DISCHARGING) ||
			(stat.val1 == POWER_SUPPLY_STATUS_NOT_CHARGING)) &&
			(type.val1 == POWER_SUPPLY_CHARGE_TYPE_NONE)
			&& (health.val1 == POWER_SUPPLY_HEALTH_GOOD))
	{
		charger_event = BSP_BATT_CHRG_EVENT_ADAPTER_REMOVED;
	}

	if ((stat.val1 == POWER_SUPPLY_STATUS_FULL) && (health.val1 == POWER_SUPPLY_HEALTH_GOOD))
	{
		charger_event = BSP_BATT_CHRG_EVENT_CHARGE_COMPLETE;
	}

	if (health.val1 != POWER_SUPPLY_HEALTH_GOOD)
	{
		LOG_ERR("charger health = %d", health.val1);
		charger_event = BSP_BATT_CHRG_EVENT_FAULT;
	}

	if ((charger_event != BSP_BATT_CHRG_EVENT_NONE) && (m_handler != NULL)) {
		m_handler(charger_event);
	}
}
#endif	/* #if (CONFIG_BQ25611D) && (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201) */

int bsp_battery_register_cb(bsp_battery_cb_handler_t handler) {
	m_handler = handler;
	return 0;
}

int bsp_battery_charging_status_get(uint8_t *charging_status, uint32_t *charge_curr_ma) {
	int ret = 0;

#if (CONFIG_BQ25611D)
	const struct device *bat_dev = device_get_binding(ACPU_C201_MOD_NAME_BATTERY_CHRG);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_BATTERY_CHRG);
		return -ENXIO;
	}
	const struct bq25611d_driver_api *bat_api = bat_dev->api;

	bq25611d_status_t bq25611d_status;
	ret = bat_api->status_get(bat_dev, &bq25611d_status);

	if ((bq25611d_status.chrg_status0.chrg_stat == BQ25611D_STATUS_FAST_CHARGING)
			|| (bq25611d_status.chrg_status0.chrg_stat
					== BQ25611D_STATUS_PRE_CHARGING)) {
		*charging_status = BSP_BATT_CHARGING;
	} else {
		*charging_status = BSP_BATT_NOT_CHARGING;
	}

	ret = bat_api->chrg_curr_setting_get(bat_dev, charge_curr_ma);
#elif (CONFIG_BQ25890)
	// TODO: implement in driver and here
	*charging_status = BSP_BATT_UNKNOWN;
	*charge_curr_ma = 0;
#elif (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}

	struct sensor_value val = {0,0};
	int chan = POWER_SUPPLY_CHAN_CHARGER;
	int attr = POWER_SUPPLY_PROP_STATUS;

	ret = sensor_attr_get(dev, chan, attr, &val);
	if (ret) {
		LOG_ERR("unable to get charger attr = %d", attr);
		return ret;
	}
	switch (val.val1) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		*charging_status = BSP_BATT_UNKNOWN;
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		*charging_status = BSP_BATT_CHARGING;
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		*charging_status = BSP_BATT_DISCHARGING;
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		*charging_status = BSP_BATT_NOT_CHARGING;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		*charging_status = BSP_BATT_FULL;
		break;
	}

	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CURRENT_VBAT_NOW;
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (ret) {
		LOG_ERR("unable to get charger attr = %d", attr);
		return ret;
	}
	*charge_curr_ma = (val.val1 / 1000); // ibat in ma
#endif
	return ret;
}

int bsp_battery_schedule_system_off(uint32_t *ms_to_off) {
#if (CONFIG_BQ25611D)
	const struct device *bat_dev = device_get_binding(ACPU_C201_MOD_NAME_BATTERY_CHRG);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_BATTERY_CHRG);
		return -ENXIO;
	}
	const struct bq25611d_driver_api *bat_api = bat_dev->api;
	return bat_api->enter_ship_mode(bat_dev, false, ms_to_off);
#elif (CONFIG_BQ25890)
	// TODO: implement in driver and here
#endif
	return -1;
}

int bsp_battery_mvolts_get(uint32_t *batt_mvolts) {
	int ret = 0;

#if 0 //(CONFIG_BQ25611D)
	const struct device *bat_dev = device_get_binding(
	ACPU_C201_MOD_NAME_BATTERY_CHRG);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_BATTERY_CHRG);
		return -ENXIO;
	}

	const struct bq25611d_driver_api *bat_api = bat_dev->api;
	uint32_t temp_mvolts = 0;

	ret = bat_api->batt_mvolts_get(bat_dev, &temp_mvolts);
	if (ret != 0) {
		LOG_ERR("failed to acquire battery mili volts");
		*batt_mvolts = 0;
	} else {
		LOG_DBG("acquired battery mili volts = %d", temp_mvolts);
		*batt_mvolts = temp_mvolts;
	}
#endif

#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_FUEL_GAUGE);
	if (dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_FUEL_GAUGE);
		return -ENXIO;
	}
	struct sensor_value val;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE, &val);

	*batt_mvolts = val.val1;
#endif /* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206) */

	return ret;
}

int bsp_battery_available_capacity_get(float *batt_capacity_mah)
{
	int ret = 0;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_FUEL_GAUGE);
	if (dev == NULL) {
		LOG_DBG("device %s not found", ACPU_C201_MOD_NAME_FUEL_GAUGE);
		return -ENXIO;
	}
	struct sensor_value val;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, &val);

	*batt_capacity_mah = sensor_value_to_double(&val);
#endif /* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206) */
	return ret;
}

int bsp_battery_capacity_value_set(float batt_capacity_mah)
{
	int ret = 0;
#if (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_FUEL_GAUGE);
	if (dev == NULL) {
		LOG_DBG("device %s not found", ACPU_C201_MOD_NAME_FUEL_GAUGE);
		return -ENXIO;
	}
	struct sensor_value val;
	val.val1 = (batt_capacity_mah * 1000);	// capacity in uAh
	sensor_attr_set(dev, SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, SENSOR_ATTR_CHARGE_VALUE_NOW, &val);
#endif /* (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_C205 || CONFIG_BOARD_E206) */
	return ret;
}

int bsp_battery_charging_control(BSP_BATTERY_CHARGING_CONTROL en_dis)
{
	int ret = 0;
#if (CONFIG_BQ25611D)
	// TODO
#elif (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CHARGE_CONTROL;
	if (en_dis == BSP_BATT_CHARGING_DISABLE)
		val.val1 = 0;
	else if (en_dis == BSP_BATT_CHARGING_ENABLE)
		val.val1 = 1;
	ret = sensor_attr_set(dev, chan, attr, &val);
#endif /* CONFIG_BQ25611D */
	return ret;
}

int bsp_battery_vbus_get(int32_t *mv)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_VOLTAGE_VBUS_NOW;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*mv = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_ibus_get(int32_t *ma)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CURRENT_VBUS_NOW;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*ma = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_vbat_get(int32_t *mv)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_VOLTAGE_VBAT_NOW;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*mv = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_ibat_discharge_sensing_control(BSP_BATTERY_IBAT_DISCHARGE_SENSING_CONTROL en_dis)
{
	int ret = 0;
#if (CONFIG_BQ25611D)
	// TODO
#elif (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_IBATT_DISCHARGE_SENSING;
	if (en_dis == BSP_BATT_IBAT_DIS_SENSE_DISABLE)
		val.val1 = 0;
	else if (en_dis == BSP_BATT_IBAT_DIS_SENSE_ENABLE)
		val.val1 = 1;
	ret = sensor_attr_set(dev, chan, attr, &val);
#endif /* CONFIG_BQ25611D */
	return ret;
}

int bsp_battery_ibat_get(int32_t *ma)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CURRENT_VBAT_NOW;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*ma = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_vchrg_get(int32_t *mv)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*mv = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_ichrg_get(int32_t *ma)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT;
	ret = sensor_attr_get(dev, chan, attr, &val);
	*ma = (val.val1 / 1000);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_ichrg_set(int32_t ma)
{
	int ret = 0;
#if (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};
	val.val1 = (ma * 1000);	// charge current ua
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT;
	ret = sensor_attr_set(dev, chan, attr, &val);
#endif /* CONFIG_BQ25792 */
	return ret;
}

int bsp_battery_init()
{
	int ret = 0;

#if (CONFIG_BQ25611D)
	/* register interrupt handler callback with battery charger driver */
	const struct device *bat_dev = device_get_binding(
	ACPU_C201_MOD_NAME_BATTERY_CHRG);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_BATTERY_CHRG);
		return -ENXIO;
	}
	const struct bq25611d_driver_api *bat_api = bat_dev->api;
	ret = bat_api->intr_handler_set(bat_dev, charger_intr_handler);

	/* set charge current limit */
	ret = bat_api->chrg_curr_lim_set(bat_dev,
			(BSP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS));
#elif (CONFIG_BQ25792)
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		LOG_ERR("charger device is not ready");
		return -1;
	}
	/* set trigger for handling interrupts */
	struct sensor_trigger trig;
	trig.type = POWER_SUPPLY_TRIG_CHARGER_INTR;
	trig.chan = POWER_SUPPLY_CHAN_CHARGER;
	ret = sensor_trigger_set(dev, &trig, charger_intr_handler);

	/* enable battery discharge current sensing */
	ret |= bsp_battery_ibat_discharge_sensing_control(BSP_BATT_IBAT_DIS_SENSE_ENABLE);

#endif /* CONFIG_BQ25611D */

	return ret;
}
