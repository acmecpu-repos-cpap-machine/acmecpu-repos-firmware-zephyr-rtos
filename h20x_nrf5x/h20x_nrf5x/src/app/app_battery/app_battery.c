/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 12-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_battery);

#include "h20x_modules.h"
#include "app_battery/app_battery.h"
#if (CONFIG_BQ25611D)
	#include "bq25611d.h"
#endif
#if (CONFIG_LTC294X)
	#include "ltc294x.h"
#endif
#include "app_utils/app_utils.h"

struct charger_data {
    struct k_work chgevt_worker;
    APP_BATTERY_CHARGER_EVENTS event;
};

/* static variable */
static struct charger_data m_chgdata;

int app_battery_FG_level_get(uint8_t *batt_level) {
	int ret = 0;
	float batt_capacity=0;
	
   	const struct device *dev = device_get_binding(SENSOR_FG_LABEL);
	if (dev == NULL) {
		LOG_ERR("device %s not found", SENSOR_FG_LABEL);
		return -ENXIO;
	}
	struct sensor_value val;
	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, &val);

	batt_capacity = sensor_value_to_double(&val);

	*batt_level = (uint8_t)((batt_capacity/CONFIG_APP_BATTERY_MAX_CAPACITY_MAH) * 100);
	LOG_INF("Battery Capacity = %.2f mAh", batt_capacity);
	LOG_INF("Battery Level = %d", *batt_level);
	return ret;
}

int app_battery_FG_charge_complete_set()
{
	int ret = 0;
	const struct device *dev = device_get_binding(SENSOR_FG_LABEL);
	struct sensor_value val;
	val.val1 = ((CONFIG_APP_BATTERY_MAX_CAPACITY_MAH+1)*1000);	/* 100% charge*/
	ret = sensor_attr_set(dev, SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, 
					SENSOR_ATTR_CHARGE_VALUE_NOW, &val);
	return ret;
}

int app_battery_FG_accumulated_charge_set(uint32_t charge_mah)
{
	int ret = 0;
	const struct device *dev = device_get_binding(SENSOR_FG_LABEL);
	struct sensor_value val;
	val.val1 = (charge_mah*1000);
	ret = sensor_attr_set(dev, SENSOR_CHAN_GAUGE_NOM_AVAIL_CAPACITY, 
					SENSOR_ATTR_CHARGE_VALUE_NOW, &val);
	return ret;
}

static void charger_event_handler(struct k_work *work) {
    struct charger_data *const chgdata = CONTAINER_OF(work, struct charger_data, chgevt_worker);
	int ret=0;

    switch (chgdata->event) {
	case APP_BATT_CHRG_EVENT_ADAPTER_ATTACHED:
    {
		LOG_INF("APP_BATT_CHRG_EVENT_ADAPTER_ATTACHED");
        /* select charger USB */
	    ret = app_utils_usb_channel_select(USB_DATA_CHANNEL_HOST);
	    if (ret < 0) {
		    LOG_ERR("app_utils_usb_channel_select failed");
		    return;
	    }
        /* get the type of input source detected */
        uint8_t charging_status=0, source_type=0;
    	uint32_t ichg=0, in_curr_lim=0;
        app_battery_charging_status_get(&charging_status, &ichg, &source_type, &in_curr_lim);
        LOG_INF("Charging status = %s, Source type = %d, Charge current = %d, Input current lim = %d", 
                (charging_status ? "CHARGING" : "NOT CHARGING"),
                source_type,
                ichg,
                in_curr_lim);

// #if (CONFIG_BQ25611D)
//         /* set charge & input current limits */
//         ret = app_battery_current_limits_set(
//             (BQ25611D_CHARGE_CURRENT_LIMIT_1920MA | BQ25611D_CHARGE_CURRENT_LIMIT_120MA),
//             (BQ25611D_INPUT_CURRENT_LIMIT_800MA | BQ25611D_INPUT_CURRENT_LIMIT_1600MA)
//             );
// #endif  /*CONFIG_BQ25611D*/

//         app_battery_charging_status_get(&charging_status, &ichg, &source_type, &in_curr_lim);
//         LOG_INF("Charging status = %s, Source type = %d, Charge current = %d, Input current lim = %d", 
//                 (charging_status ? "CHARGING" : "NOT CHARGING"),
//                 source_type,
//                 ichg,
//                 in_curr_lim);

        /* show led notification */
        /* update on display */
    }
		break;
	case APP_BATT_CHRG_EVENT_ADAPTER_REMOVED:
		LOG_INF("APP_BATT_CHRG_EVENT_ADAPTER_REMOVED");
        /* select charger USB */
	    ret = app_utils_usb_channel_select(USB_DATA_CHANNEL_CHARGER);
	    if (ret < 0) {
		    LOG_ERR("app_utils_usb_channel_select failed");
		    return;
	    }
        /* show led notification */
        /* update on display */
		break;
	case APP_BATT_CHRG_EVENT_CHARGE_COMPLETE:
	{
		LOG_INF("APP_BATT_CHRG_EVENT_CHARGE_COMPLETE");
        /* show led notification */
        /* update on display */
        
		/* inform fuel gauge driver */
		app_battery_FG_charge_complete_set();
	}
		break;
	case APP_BATT_CHRG_EVENT_FAULT:
		LOG_INF("APP_BATT_CHRG_EVENT_FAULT");
        /* show led notification */
        /* update on display */
		break;
	default:
		break;
	}
}

static void charger_intr_handler(const struct device *dev, bq25611d_status_t *chrg_stat)
{
	APP_BATTERY_CHARGER_EVENTS charger_event = 0;
#if (CONFIG_BQ25611D)
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
			charger_event = APP_BATT_CHRG_EVENT_ADAPTER_ATTACHED;
		}
	}

	/* Check for charge complete event */
//	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_GOOD) {
		if ((chrg_stat->chrg_status0.chrg_stat
				== BQ25611D_STATUS_CHARGE_TERMINATION)) {
			charger_event = APP_BATT_CHRG_EVENT_CHARGE_COMPLETE;
		}
//	}

	/* Check for charger removed event */
	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_NOT_GOOD) {
		if ((chrg_stat->chrg_status0.chrg_stat == BQ25611D_STATUS_NOT_CHARGING)) {
			charger_event = APP_BATT_CHRG_EVENT_ADAPTER_REMOVED;
		}
	}

	/* Check for faults event */
	if (chrg_stat->chrg_status2.vbus_gd == BQ25611D_STATUS_VBUS_GOOD) {
		if ((chrg_stat->chrg_status1.chrg_fault
				!= BQ25611D_STATUS_CHARGE_FAULT_NORMAL)
				|| (chrg_stat->chrg_status1.bat_fault
						!= BQ25611D_STATUS_BATT_FAULT_NORMAL)) {
			charger_event = APP_BATT_CHRG_EVENT_FAULT;
		}
	}
#endif /*CONFIG_BQ25611D*/
    /* handle the event */
    m_chgdata.event = charger_event;
    k_work_submit(&m_chgdata.chgevt_worker);
}

int app_battery_charging_status_get(uint8_t *charging_status,
                                    uint32_t *charge_curr_ma, 
                                    uint8_t *source_type,
                                    uint32_t *in_curr_lim)
{
	int ret = 0;

	const struct device *bat_dev = device_get_binding(BATT_CHRG_LABEL);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", BATT_CHRG_LABEL);
		return -ENXIO;
	}
#if (CONFIG_BQ25611D)
	const struct bq25611d_driver_api *bat_api = bat_dev->api;

	bq25611d_status_t bq25611d_status;
	ret = bat_api->status_get(bat_dev, &bq25611d_status);
    LOG_DBG("CHRG_STAT = %d", bq25611d_status.chrg_status0.chrg_stat);
	if ((bq25611d_status.chrg_status0.chrg_stat == BQ25611D_STATUS_FAST_CHARGING)
			|| (bq25611d_status.chrg_status0.chrg_stat
					== BQ25611D_STATUS_PRE_CHARGING)) {
		*charging_status = APP_BATT_CHARGING;
	} else {
		*charging_status = APP_BATT_NOT_CHARGING;
	}

    *source_type = bq25611d_status.chrg_status0.vbus_stat;

	ret = bat_api->chrg_curr_setting_get(bat_dev, charge_curr_ma);
    ret |= bat_api->in_curr_setting_get(bat_dev, in_curr_lim);
#endif
	return ret;
}

int app_battery_current_limits_set(uint32_t chg_cur_lim, uint32_t in_cur_lim)
{
    int ret = 0;
	const struct device *bat_dev = device_get_binding(BATT_CHRG_LABEL);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", BATT_CHRG_LABEL);
		return -ENXIO;
	}
#if (CONFIG_BQ25611D)
    const struct bq25611d_driver_api *bat_api = bat_dev->api;
	/* set charge current limit */
	ret = bat_api->chrg_curr_lim_set(bat_dev, chg_cur_lim);

    /* set input current limit */
    ret = bat_api->in_curr_setting_set(bat_dev, in_cur_lim);
#endif
    return ret;
}

int app_battery_init()
{
	int ret = 0;

	/* Prepare worker thread */
	k_work_init(&m_chgdata.chgevt_worker, charger_event_handler);

#if (CONFIG_BQ25611D)
	/* register interrupt handler callback with battery charger driver */
	const struct device *bat_dev = device_get_binding(BATT_CHRG_LABEL);
	if (bat_dev == NULL) {
		LOG_ERR("device %s not found", BATT_CHRG_LABEL);
		return -ENXIO;
	}
	const struct bq25611d_driver_api *bat_api = bat_dev->api;
	ret = bat_api->intr_handler_set(bat_dev, charger_intr_handler);

	/* set charge & input current limits */
    ret = app_battery_current_limits_set(APP_BATTERY_CHARGE_CURR_LIMIT_SETTINGS,
                                APP_BATTERY_CHARGER_INPUT_CURRENT_LIMIT_SETTINGS);

    uint8_t charging_status=0, source_type=0;
	uint32_t ichg=0, in_curr_lim=0;;
	ret = app_battery_charging_status_get(&charging_status, &ichg, &source_type, &in_curr_lim);
	if ((!ret) && (charging_status == APP_BATT_CHARGING)) {
        /* handle the event */
        m_chgdata.event = charging_status;
        k_work_submit(&m_chgdata.chgevt_worker);
	}
    LOG_INF("charging_status = %d, charge_curr = %d", charging_status, ichg);

#endif /* CONFIG_BQ25611D */

	return ret;
}
