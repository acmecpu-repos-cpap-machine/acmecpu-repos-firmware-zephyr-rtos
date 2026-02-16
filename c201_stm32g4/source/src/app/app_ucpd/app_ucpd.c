/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 19-Jan-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb_c/usbc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_ucpd, LOG_LEVEL_INF);

#define PORT1_NODE DT_NODELABEL(port1)
#define PORT1_POWER_ROLE		DT_ENUM_IDX(DT_NODELABEL(port1), power_role)

#if (PORT1_POWER_ROLE != TC_ROLE_SINK)
#error "Unsupported board: Only Sink device supported"
#endif

#define SINK_PDO(node_id, prop, idx)	(DT_PROP_BY_IDX(node_id, prop, idx)),

#define APP_MVOLT_MAX	CONFIG_APP_MVOLT_MAX //(15000)

/* usbc.rst port data object start */
/**
 * @brief A structure that encapsulates Port data.
 */
static struct port1_data_t {
	/** Sink Capabilities */
	uint32_t snk_caps[DT_PROP_LEN(DT_NODELABEL(port1), sink_pdos)];
	/** Number of Sink Capabilities */
	int snk_cap_cnt;
	/** Source Capabilities */
	uint32_t src_caps[PDO_MAX_DATA_OBJECTS];
	/** Number of Source Capabilities */
	int src_cap_cnt;
	/* Power Supply Ready flag */
	atomic_t ps_ready;
} port1_data = {
	.snk_caps = {DT_FOREACH_PROP_ELEM(DT_NODELABEL(port1), sink_pdos, SINK_PDO)},
	.snk_cap_cnt = DT_PROP_LEN(DT_NODELABEL(port1), sink_pdos),
	.src_caps = {0},
	.src_cap_cnt = 0,
	.ps_ready = 0
};

const struct device *usbc_port1;

/* usbc.rst port data object end */


static void get_src_max_specs(const struct port1_data_t *dpm_data, uint32_t *max_mv, uint32_t *max_ma)
{
	uint32_t src_mv_max=5000, tmp_mv=0;
	uint32_t src_ma_max=100, tmp_ma=0;

	for (int i = 0; i < dpm_data->src_cap_cnt; i++) {
		const uint32_t pdo_value = dpm_data->src_caps[i];
		union pd_fixed_supply_pdo_source pdo;

		/* Default to fixed supply pdo source until type is detected */
		pdo.raw_value = pdo_value;

		switch (pdo.type) {
		case PDO_FIXED:
			tmp_mv = PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(pdo.voltage);
			tmp_ma = PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(pdo.max_current);
			break;
		case PDO_BATTERY:
#if 0	// not supported now
		{
			union pd_battery_supply_pdo_source pdo;
			pdo.raw_value = pdo_value;
			tmp_mv = PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(pdo.max_voltage);
			tmp_ma = PD_CONVERT_BATTERY_PDO_POWER_TO_MW(pdo.max_power)/tmp_mv;
		}
#endif
			break;
		case PDO_VARIABLE:
#if 0	// not supported now
		{
			union pd_variable_supply_pdo_source pdo;
			pdo.raw_value = pdo_value;
			tmp_mv = PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(pdo.max_voltage);
			tmp_ma = PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(pdo.max_current);
		}
#endif
			break;
		case PDO_AUGMENTED:
#if 0	// not supported now
			union pd_augmented_supply_pdo_source pdo;
			pdo.raw_value = pdo_value;
			tmp_mv = PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(pdo.max_voltage);
			tmp_ma = PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(pdo.max_current);
#endif
			break;
		}

		/* save the max voltage */
		if (tmp_mv > src_mv_max) {
			src_mv_max = tmp_mv;
		}
//		if (tmp_ma > src_ma_max) {
//			src_ma_max = tmp_ma;
//		}
	}
	/* save the current wrt to the max voltage */
	src_ma_max = tmp_ma;

	/* copy to out variables */
	*max_mv = src_mv_max;
	*max_ma = src_ma_max;
}
/**
 * @brief Builds a Request Data Object (RDO) with the following properties:
 *		- Maximum operating current 100mA
 *		- Operating current is 100mA
 *		- Unchunked Extended Messages Not Supported
 *		- No USB Suspend
 *		- Not USB Communications Capable
 *		- No capability mismatch
 *		- Does not Giveback
 *		- Select object position 1 (5V Power Data Object (PDO))
 *
 * @note Generally a sink application would build an RDO from the
 *	 Source Capabilities stored in the dpm_data object
 */
static uint32_t build_rdo(const struct port1_data_t *dpm_data)
{
	/**/
	uint32_t src_mv_max=5000,src_ma_max=100;
	uint32_t snk_mv=5000, snk_ma=100;

	get_src_max_specs(dpm_data, &src_mv_max, &src_ma_max);

	union pd_fixed_supply_pdo_sink pdo;
	int pos/*, match=-1*/;
	for (pos=0; pos<dpm_data->snk_cap_cnt; pos++) {
		const uint32_t pdo_value = dpm_data->snk_caps[pos];
		pdo.raw_value = pdo_value;

		snk_mv = PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(pdo.voltage);
		snk_ma = PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(pdo.operational_current);

		if (src_mv_max == 0) {
			LOG_ERR("could not determine source caps!");
			break;
		}
		if ((src_mv_max <= APP_MVOLT_MAX) && (snk_mv >= src_mv_max)) {
			snk_mv = src_mv_max;
//			match++;
			if (snk_ma <= src_ma_max) {
				snk_ma = src_ma_max;
//				match++;
			} else {
				LOG_WRN("current not matching, snk_ma = %d, src_ma_max = %d", snk_ma, src_ma_max);
//				match--;
				/* use the max available current from source */
				snk_ma = src_ma_max;
				/* notify user */
				// TODO: fire and event to show on UI
			}
			pos++;	// this is used as pdo object position which starts from 1, hence incrementing
			break;
		}
	}

	/* check if we got a match */
//	if (match < 0) {
//		LOG_WRN("incapable source! src_mv_max = %d, src_ma_max = %d", src_mv_max, src_ma_max);
//		snk_mv = src_mv_max;
//		snk_ma = src_ma_max;
//	}

	LOG_INF("snk_mv = %d, snk_ma = %d, pos = %d", snk_mv, snk_ma, pos);

	union pd_rdo rdo;

	/* Maximum operating current 100mA (GIVEBACK = 0) */
	rdo.fixed.min_or_max_operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(snk_ma);
	/* Operating current 100mA */
	rdo.fixed.operating_current = PD_CONVERT_MA_TO_FIXED_PDO_CURRENT(snk_ma);
	/* Unchunked Extended Messages Not Supported */
	rdo.fixed.unchunked_ext_msg_supported = 0;
	/* No USB Suspend */
	rdo.fixed.no_usb_suspend = 1;
	/* Not USB Communications Capable */
	rdo.fixed.usb_comm_capable = 0;
	/* No capability mismatch */
	rdo.fixed.cap_mismatch = 0;
	/* Don't giveback */
	rdo.fixed.giveback = 0;
	/* Object position 1 (5V PDO) */
	rdo.fixed.object_pos = pos;

	return rdo.raw_value;
}

static void display_pdo(const int idx,
			const uint32_t pdo_value)
{
	union pd_fixed_supply_pdo_source pdo;

	/* Default to fixed supply pdo source until type is detected */
	pdo.raw_value = pdo_value;

	LOG_INF("PDO %d:", idx);
	switch (pdo.type) {
	case PDO_FIXED: {
		LOG_INF("\tType:              FIXED");
		LOG_INF("\tCurrent:           %d",
		       PD_CONVERT_FIXED_PDO_CURRENT_TO_MA(pdo.max_current));
		LOG_INF("\tVoltage:           %d",
		       PD_CONVERT_FIXED_PDO_VOLTAGE_TO_MV(pdo.voltage));
		LOG_INF("\tPeak Current:      %d", pdo.peak_current);
		LOG_INF("\tUchunked Support:  %d",
		       pdo.unchunked_ext_msg_supported);
		LOG_INF("\tDual Role Data:    %d",
		       pdo.dual_role_data);
		LOG_INF("\tUSB Comms:         %d",
		       pdo.usb_comms_capable);
		LOG_INF("\tUnconstrained Pwr: %d",
		       pdo.unconstrained_power);
		LOG_INF("\tUSB Suspend:       %d",
		       pdo.usb_suspend_supported);
		LOG_INF("\tDual Role Power:   %d",
		       pdo.dual_role_power);
	}
	break;
	case PDO_BATTERY: {
		union pd_battery_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:              BATTERY");
		LOG_INF("\tMin Voltage: %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
			PD_CONVERT_BATTERY_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Power:   %d",
			PD_CONVERT_BATTERY_PDO_POWER_TO_MW(pdo.max_power));
	}
	break;
	case PDO_VARIABLE: {
		union pd_variable_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:        VARIABLE");
		LOG_INF("\tMin Voltage: %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage: %d",
			PD_CONVERT_VARIABLE_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Current: %d",
			PD_CONVERT_VARIABLE_PDO_CURRENT_TO_MA(pdo.max_current));
	}
	break;
	case PDO_AUGMENTED: {
		union pd_augmented_supply_pdo_source pdo;

		pdo.raw_value = pdo_value;
		LOG_INF("\tType:              AUGMENTED");
		LOG_INF("\tMin Voltage:       %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(pdo.min_voltage));
		LOG_INF("\tMax Voltage:       %d",
			PD_CONVERT_AUGMENTED_PDO_VOLTAGE_TO_MV(pdo.max_voltage));
		LOG_INF("\tMax Current:       %d",
			PD_CONVERT_AUGMENTED_PDO_CURRENT_TO_MA(pdo.max_current));
		LOG_INF("\tPPS Power Limited: %d", pdo.pps_power_limited);
	}
	break;
	}
}

static void display_source_caps(const struct device *dev)
{
	struct port1_data_t *dpm_data = usbc_get_dpm_data(dev);

	LOG_INF("Source Caps:");
	for (int i = 0; i < dpm_data->src_cap_cnt; i++) {
		display_pdo(i, dpm_data->src_caps[i]);
		k_msleep(50);
	}
}

/* usbc.rst callbacks start */
static int port1_policy_cb_get_snk_cap(const struct device *dev,
					    uint32_t **pdos,
					    int *num_pdos)
{
	struct port1_data_t *dpm_data = usbc_get_dpm_data(dev);

	*pdos = dpm_data->snk_caps;
	num_pdos = &dpm_data->snk_cap_cnt;
//	LOG_INF("port1_policy_cb_get_snk_cap, num = %d", dpm_data->snk_cap_cnt);
	return 0;
}

static void port1_policy_cb_set_src_cap(const struct device *dev,
					     const uint32_t *pdos,
					     const int num_pdos)
{
	struct port1_data_t *dpm_data;
	int num;
	int i;

	dpm_data = usbc_get_dpm_data(dev);

	num = num_pdos;
	if (num > PDO_MAX_DATA_OBJECTS) {
		num = PDO_MAX_DATA_OBJECTS;
	}
//	LOG_INF("port1_policy_cb_set_src_cap, num = %d", num);

	for (i = 0; i < num; i++) {
		dpm_data->src_caps[i] = *(pdos + i);
	}

	dpm_data->src_cap_cnt = num;
}

static uint32_t port1_policy_cb_get_rdo(const struct device *dev)
{
	struct port1_data_t *dpm_data = usbc_get_dpm_data(dev);
//	LOG_INF("port1_policy_cb_get_rdo");
	return build_rdo(dpm_data);
}
/* usbc.rst callbacks end */

/* usbc.rst notify start */
static void port1_notify(const struct device *dev,
			      const enum usbc_policy_notify_t policy_notify)
{
	struct port1_data_t *dpm_data = usbc_get_dpm_data(dev);

	switch (policy_notify) {
	case PROTOCOL_ERROR:
		LOG_DBG("PROTOCOL_ERROR");
		break;
	case MSG_DISCARDED:
		LOG_DBG("MSG_DISCARDED");
		break;
	case MSG_ACCEPT_RECEIVED:
		LOG_DBG("MSG_ACCEPT_RECEIVED");
		break;
	case MSG_REJECTED_RECEIVED:
		LOG_DBG("MSG_REJECTED_RECEIVED");
		break;
	case MSG_NOT_SUPPORTED_RECEIVED:
		LOG_DBG("MSG_NOT_SUPPORTED_RECEIVED");
		break;
	case TRANSITION_PS:
		LOG_DBG("TRANSITION_PS");
		atomic_set_bit(&dpm_data->ps_ready, 0);
		break;
	case PD_CONNECTED:
		LOG_DBG("PD_CONNECTED");
		break;
	case NOT_PD_CONNECTED:
		LOG_DBG("NOT_PD_CONNECTED");
		break;
	case POWER_CHANGE_0A0:
		LOG_INF("PWR 0A");
		break;
	case POWER_CHANGE_DEF:
		LOG_INF("PWR DEF");
		break;
	case POWER_CHANGE_1A5:
		LOG_INF("PWR 1A5");
		break;
	case POWER_CHANGE_3A0:
		LOG_INF("PWR 3A0");
		break;
	case DATA_ROLE_IS_UFP:
		LOG_DBG("DATA_ROLE_IS_UFP");
		break;
	case DATA_ROLE_IS_DFP:
		LOG_DBG("DATA_ROLE_IS_DFP");
		break;
	case PORT_PARTNER_NOT_RESPONSIVE:
		LOG_DBG("Port Partner not PD Capable");
		break;
	case SNK_TRANSITION_TO_DEFAULT:
		LOG_DBG("SNK_TRANSITION_TO_DEFAULT");
		break;
	case HARD_RESET_RECEIVED:
		LOG_DBG("HARD_RESET_RECEIVED");
		break;
	case SENDER_RESPONSE_TIMEOUT:
		LOG_DBG("SENDER_RESPONSE_TIMEOUT");
		break;
	case SOURCE_CAPABILITIES_RECEIVED:
		LOG_DBG("SOURCE_CAPABILITIES_RECEIVED");
		break;
	}
}
/* usbc.rst notify end */

/* usbc.rst check start */
bool port1_policy_check(const struct device *dev,
			const enum usbc_policy_check_t policy_check)
{
	switch (policy_check) {
	case CHECK_POWER_ROLE_SWAP:
		/* Reject power role swaps */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_DFP:
		/* Reject data role swap to DFP */
		return false;
	case CHECK_DATA_ROLE_SWAP_TO_UFP:
		/* Accept data role swap to UFP */
		return true;
	case CHECK_SNK_AT_DEFAULT_LEVEL:
		/* This device is always at the default power level */
		return true;
	default:
		/* Reject all other policy checks */
		return false;

	}
}
/* usbc.rst check end */

int app_ucpd_check_ps()
{
	if (atomic_test_and_clear_bit(&port1_data.ps_ready, 0)) {
		/* Display the Source Capabilities */
		display_source_caps(usbc_port1);
		return 1;
	}
	return 0;
}

int app_ucpd_init(void)
{
	/* Get the device for this port */
	usbc_port1 = DEVICE_DT_GET(PORT1_NODE);
	if (!device_is_ready(usbc_port1)) {
		LOG_ERR("PORT1 device not ready");
		return -1;
	}

	/* usbc.rst register start */
	/* Register USB-C Callbacks */

	/* Register Policy Check callback */
	usbc_set_policy_cb_check(usbc_port1, port1_policy_check);
	/* Register Policy Notify callback */
	usbc_set_policy_cb_notify(usbc_port1, port1_notify);
	/* Register Policy Get Sink Capabilities callback */
	usbc_set_policy_cb_get_snk_cap(usbc_port1, port1_policy_cb_get_snk_cap);
	/* Register Policy Set Source Capabilities callback */
	usbc_set_policy_cb_set_src_cap(usbc_port1, port1_policy_cb_set_src_cap);
	/* Register Policy Get Request Data Object callback */
	usbc_set_policy_cb_get_rdo(usbc_port1, port1_policy_cb_get_rdo);
	/* usbc.rst register end */

	/* usbc.rst user data start */
	/* Set Application port data object. This object is passed to the policy callbacks */
	port1_data.ps_ready = ATOMIC_INIT(0);
	usbc_set_dpm_data(usbc_port1, &port1_data);
	/* usbc.rst user data end */

	/* usbc.rst usbc start */
	/* Start the USB-C Subsystem */
	usbc_start(usbc_port1);
	/* usbc.rst usbc end */

//	while (1) {
//		/* Perform Application Specific functions */
//		if (atomic_test_and_clear_bit(&port1_data.ps_ready, 0)) {
//			/* Display the Source Capabilities */
//			display_source_caps(usbc_port1);
//		}
//
//		/* Arbitrary delay */
//		k_msleep(1000);
//	}
	return 0;
}



