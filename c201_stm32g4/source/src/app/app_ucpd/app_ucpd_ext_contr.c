/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 05-Oct-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <sys/errno.h>
#include <stdint.h>
#include <stdbool.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_ucpd_ext_contr, LOG_LEVEL_INF);

#include "lib_events/lib_events.h"
#include "app_battery/app_battery.h"
//#include "app_battery/bsp_battery.h"
#include "app_utils/app_utils.h"

#if (CONFIG_STUSB4500)
	#include "stusb4500.h"
#endif

#define APP_MVOLT_MAX	CONFIG_APP_MVOLT_MAX //(15000)
#define PDO_MAX_DATA_OBJECTS 7
#define MAX_RETRY	20

#if (CONFIG_STUSB4500)
	#define PDO_POSITION_TO_UPDATE	2
#else
	#define PDO_POSITION_TO_UPDATE	2
#endif

typedef enum {
	UCPD_CONTR_STATE_NONE = 0,
	UCPD_CONTR_STATE_INIT,
	UCPD_CONTR_STATE_SOFTRESET,
	UCPD_CONTR_STATE_SRCPDO_GET,
	UCPD_CONTR_STATE_PDO_UPDATE,
	UCPD_CONTR_STATE_RDO_GET,
	UCPD_CONTR_STATE_NEGO_CHECK,
	UCPD_CONTR_STATE_NEGO_DONE,
	UCPD_CONTR_STATE_IDLE,

	UCPD_CONTR_STATE_MAX
} UCPD_CONTR_STATES;

struct ucpd_rdo {
	uint8_t object_pos;
	uint32_t nego_mvolts;
	uint32_t max_curr_ma;
	uint32_t oper_curr_ma;
};

struct ucpd_va {
	uint32_t mv;
	uint32_t ma;
};

struct ucpd_contr {
	UCPD_CONTR_STATES state;
	UCPD_CONTR_STATES prev_state;
	struct k_work worker;
	int src_pdo_num;
	struct ucpd_va src_caps[PDO_MAX_DATA_OBJECTS];
	struct ucpd_va snk_va;
	struct ucpd_rdo rdo;
	int retry;
};

static struct lib_events_callback m_evnt_cb_charger_attached;
static struct lib_events_callback m_evnt_cb_charger_removed;
static struct ucpd_contr m_ucpd;

static void ucpd_data_reset();

static void ucpd_device_intr_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	k_work_submit(&m_ucpd.worker);
}

static int app_ucpd_device_init()
{
	int ret = 0;
#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}

	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_INIT;
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set failed");
	}
#endif
	return ret;
}

static int app_ucpd_device_trigger_set()
{
	int ret = 0;
#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}

	/* set trigger for handling interrupts */
	struct sensor_trigger trig;
	trig.type = USBCPD_TRIG_UCPD_INTR;
	trig.chan = USBCPD_CHAN_UCPD_CONTR;
	ret = sensor_trigger_set(dev, &trig, ucpd_device_intr_handler);
#endif
	return ret;
}

static int app_ucpd_device_softreset()
{
	int ret = 0;
#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}

	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SOFT_RESET;
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set failed");
	}
#endif
	return ret;
}

static int app_ucpd_device_src_pdo_get(int *num_pdo, struct ucpd_va *src_caps)
{
	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val[PDO_MAX_DATA_OBJECTS];

#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}

	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SRCPDO;
	ret = sensor_attr_get(dev, chan, attr, val);
	if (ret) {
		LOG_ERR("sensor_attr_get failed");
	}
#endif
	if (ret == 0) {
		*num_pdo = val[0].val1;
		for (int i = 0; i < val[0].val1; i++) {
			(&src_caps[i])->mv = val[i + 1].val1;
			(&src_caps[i])->ma = val[i + 1].val2;
		}
	}

	return ret;
}

static int app_ucpd_device_pdo_update(uint8_t pdo_pos, struct ucpd_va *snk)
{
	int ret = 0;
	int chan = 0;
	int attr = 0;
	uint8_t pdo_num = pdo_pos;
	int mvolts = snk->mv;
	int mamps = snk->ma;

#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}

	struct sensor_value val[2];
	val[0].val1 = pdo_num;
	val[1].val1 = mvolts;
	val[1].val2 = mamps;

	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SNKPDO;
	ret = sensor_attr_set(dev, chan, attr, val);
	if (ret) {
		LOG_ERR("sensor_attr_set failed");
	}
#endif

	return ret;
}

static int app_ucpd_device_rdo_get(struct ucpd_rdo *rdo)
{
	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val[3];

#if CONFIG_STUSB4500
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		LOG_ERR("ucpd device is not ready");
		return -1;
	}
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_RDO;
#endif

	ret = sensor_attr_get(dev, chan, attr, val);
	if (!ret) {
		rdo->object_pos = val[0].val1;
		rdo->nego_mvolts = val[1].val1;
		rdo->max_curr_ma = val[2].val1;
		rdo->oper_curr_ma = val[2].val2;
	}

	return ret;
}

static int snk_va_compute(int num_pdo, struct ucpd_va *src_caps, struct ucpd_va *snk)
{
	int ret = -1;
	uint32_t snk_mv = 5000, snk_ma=100;

//	if ((snk->mv != 0) && (snk->ma != 0)) {
//		snk_mv = snk->mv;
//		snk_ma = snk->ma;
//	}

	int i;
	for (i=0; i < num_pdo; i++) {
		if ((&src_caps[i])->mv <= APP_MVOLT_MAX) {
			snk_mv = (&src_caps[i])->mv;
			snk_ma = (&src_caps[i])->ma;
			ret = 0;
		}
	}
	snk->mv = snk_mv;
	snk->ma = snk_ma;

	return ret;
}

static void ucpd_work_handler(struct k_work *work)
{
	int ret = 0;
	struct ucpd_contr *ucpd = CONTAINER_OF(work, struct ucpd_contr, worker);

	switch(ucpd->state) {
	case UCPD_CONTR_STATE_INIT:
	{
//		int32_t vbus_mv = 0;
//		ret = bsp_battery_vbus_get(&vbus_mv);
//		if (ret) {
//			LOG_ERR("could not read vbus voltage, %d", ret);
//			return;
//		}
//		if ((vbus_mv * 1000) >= BSP_BATTERY_VBUS_MIN_UV) {
		if (app_battery_usb_attached_check()) {
			app_utils_ucpd_i2c_mux_control(APP_UTILS_DEVICE_ENABLE);
			k_sleep(K_MSEC(10));

			/* 2. initialize ucpd controller */
			ret = app_ucpd_device_init();
			if (ret == 0) {
				LOG_INF("UCPD controller init successful");
				ucpd->prev_state = UCPD_CONTR_STATE_INIT;
				ucpd->state = UCPD_CONTR_STATE_SOFTRESET;
			}
		}
	}
		break;
	case UCPD_CONTR_STATE_SOFTRESET:
	{
		ret = app_ucpd_device_softreset();
		if (ret == 0) {
			LOG_INF("UCPD controller softreset successful");
			if (ucpd->prev_state == UCPD_CONTR_STATE_INIT) {
				ucpd->state = UCPD_CONTR_STATE_SRCPDO_GET;
			} else if (ucpd->prev_state == UCPD_CONTR_STATE_PDO_UPDATE) {
				/* read RDO and check whether the new PDO was negotiated */
				ucpd->state = UCPD_CONTR_STATE_RDO_GET;
//				k_work_submit(&ucpd->worker);
			}
		}
	}
		break;
	case UCPD_CONTR_STATE_SRCPDO_GET:
	{
		ret = app_ucpd_device_src_pdo_get(&ucpd->src_pdo_num, ucpd->src_caps);
		if (ret < 0) {	// retry
			if (++ucpd->retry >= MAX_RETRY) {
				LOG_INF("max retry = %d reached, abort", ucpd->retry);
				ucpd->state = UCPD_CONTR_STATE_RDO_GET;
				k_work_submit(&ucpd->worker);
				return;
			}
			LOG_INF("could not read src pdo, retry = %d", ucpd->retry);
			ucpd->prev_state = UCPD_CONTR_STATE_INIT;
			ucpd->state = UCPD_CONTR_STATE_SOFTRESET;
		} else {
			ucpd->retry = 0;
			LOG_INF("------------------------");
			LOG_INF("Successful read %d SRC PDOs", ucpd->src_pdo_num);
			for (int i=0; i<ucpd->src_pdo_num; i++) {
				LOG_INF("[%d] :: %d mv, %d ma", i, ucpd->src_caps[i].mv, ucpd->src_caps[i].ma);
			}
			LOG_INF("------------------------");

			/* compute rdo from src pdo */
			ret = snk_va_compute(ucpd->src_pdo_num, ucpd->src_caps, &ucpd->snk_va);
			if (ret < 0)	return;

			LOG_INF("Updating PDO to %d mv and %d ma", ucpd->snk_va.mv, ucpd->snk_va.ma);
			ucpd->prev_state = UCPD_CONTR_STATE_SRCPDO_GET;
			ucpd->state = UCPD_CONTR_STATE_PDO_UPDATE;
		}
		k_work_submit(&ucpd->worker);
	}
		break;
	case UCPD_CONTR_STATE_PDO_UPDATE:
	{
		ucpd->prev_state = UCPD_CONTR_STATE_PDO_UPDATE;
		ucpd->state = UCPD_CONTR_STATE_SOFTRESET;

		ret = app_ucpd_device_pdo_update(PDO_POSITION_TO_UPDATE, &ucpd->snk_va);
		if (ret < 0) {
			LOG_ERR("could not update PDO");
			ucpd->state = UCPD_CONTR_STATE_RDO_GET;
		}
		k_sleep(K_MSEC(1000));
		k_work_submit(&ucpd->worker);
	}
		break;
	case UCPD_CONTR_STATE_RDO_GET:
	{
		k_sleep(K_MSEC(3000));
		ucpd->prev_state = UCPD_CONTR_STATE_RDO_GET;
		ucpd->state = UCPD_CONTR_STATE_NEGO_CHECK;

		ret = app_ucpd_device_rdo_get(&ucpd->rdo);
		if (ret < 0) {
			if (++ucpd->retry >= MAX_RETRY) {
				LOG_INF("could not read RDO, max retry = %d reached, ABORT!", ucpd->retry);
				// todo check if ucpd_data_reset() should be called here
				lib_events_report_event(LIB_EVENT_UCPD_SNK_NEGO_FAILED);
				return;
			}
			// retry getting RDO
			LOG_ERR("could not read RDO, retry = %d", ucpd->retry);
			ucpd->prev_state = UCPD_CONTR_STATE_PDO_UPDATE;
			ucpd->state = UCPD_CONTR_STATE_RDO_GET;
			k_work_submit(&ucpd->worker);
			return;
		}
		LOG_INF("------------------------");
		LOG_INF("RDO");
		LOG_INF("Obj Pos = %d", ucpd->rdo.object_pos);
		LOG_INF("Voltage = %d mv", ucpd->rdo.nego_mvolts);
		LOG_INF("Max Curr = %d ma", ucpd->rdo.max_curr_ma);
		LOG_INF("Oper Curr = %d ma", ucpd->rdo.oper_curr_ma);
		LOG_INF("------------------------");
		k_work_submit(&ucpd->worker);
	}
		break;
	case UCPD_CONTR_STATE_NEGO_CHECK:
	{
		if (ucpd->rdo.nego_mvolts == ucpd->snk_va.mv) {
		} else {
			ucpd->snk_va.mv = ucpd->rdo.nego_mvolts;
			ucpd->snk_va.ma = ucpd->rdo.oper_curr_ma;
		}
		ucpd->prev_state = UCPD_CONTR_STATE_NEGO_CHECK;
		ucpd->state = UCPD_CONTR_STATE_NEGO_DONE;
		k_work_submit(&ucpd->worker);
	}
		break;
	case UCPD_CONTR_STATE_NEGO_DONE:
	{
		LOG_INF("UCPD negotiation done: Voltage = %d mv, Current = %d ma",
				ucpd->snk_va.mv, ucpd->snk_va.ma);
		ucpd->prev_state = UCPD_CONTR_STATE_NEGO_DONE;
		ucpd->state = UCPD_CONTR_STATE_IDLE;

		app_utils_ucpd_i2c_mux_control(APP_UTILS_DEVICE_DISABLE);
		k_sleep(K_MSEC(10));

		lib_events_report_event(LIB_EVENT_UCPD_SNK_NEGO_DONE);
	}
		break;
	case UCPD_CONTR_STATE_IDLE:
	{
		break;
	}
	default:
		break;
	}
}

static void app_event_handler(struct lib_events_callback *cb, LIB_EVENT_TYPE event)
{
	switch (event) {
	case LIB_EVENT_CHARGER_ATTACHED:
		if ((m_ucpd.state == UCPD_CONTR_STATE_NONE) && (m_ucpd.prev_state == UCPD_CONTR_STATE_NONE)) {
			m_ucpd.state = UCPD_CONTR_STATE_INIT;
			k_work_submit(&m_ucpd.worker);
		}
		break;
	case LIB_EVENT_CHARGER_REMOVED:
		if (m_ucpd.state != UCPD_CONTR_STATE_NONE) {
			ucpd_data_reset();
		}
		break;
	default:
		break;
	}
}

static void ucpd_data_reset()
{
	m_ucpd.state = UCPD_CONTR_STATE_NONE;
	m_ucpd.prev_state = UCPD_CONTR_STATE_NONE;
	m_ucpd.src_pdo_num = 0;
	memset(m_ucpd.src_caps, 0x00, sizeof(m_ucpd.src_caps));
	memset(&m_ucpd.snk_va, 0x00, sizeof(m_ucpd.snk_va));
	memset(&m_ucpd.rdo, 0x00, sizeof(m_ucpd.rdo));
	m_ucpd.retry = 0;
}

int app_ucpd_ext_contr_nego_power_get(uint32_t *mvolts, uint32_t *max_curr_ma, uint32_t *oper_curr_ma)
{
	if ((m_ucpd.prev_state == UCPD_CONTR_STATE_NEGO_DONE) && (m_ucpd.state == UCPD_CONTR_STATE_IDLE)) {
		*mvolts = m_ucpd.rdo.nego_mvolts;
		*max_curr_ma = m_ucpd.rdo.max_curr_ma;
		*oper_curr_ma = m_ucpd.rdo.oper_curr_ma;
		return 0;
	} else if (	(m_ucpd.state == UCPD_CONTR_STATE_PDO_UPDATE)	||
				(m_ucpd.state == UCPD_CONTR_STATE_RDO_GET)		||
				(m_ucpd.state == UCPD_CONTR_STATE_NEGO_CHECK)	) {
		return -EBUSY;
	} else {
		return -ENOTSUP;
	}
}

int app_ucpd_ext_contr_init()
{
	int ret = 0;

	k_work_init(&m_ucpd.worker, ucpd_work_handler);
	ucpd_data_reset();
	app_ucpd_device_trigger_set();

	ret = lib_events_callback_add(&m_evnt_cb_charger_attached, app_event_handler, LIB_EVENT_CHARGER_ATTACHED);
	ret = lib_events_callback_add(&m_evnt_cb_charger_removed, app_event_handler, LIB_EVENT_CHARGER_REMOVED);

	return ret;
}
