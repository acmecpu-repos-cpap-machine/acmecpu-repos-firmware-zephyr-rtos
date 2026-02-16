/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 02-Oct-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"

#include "app_blower/app_blower.h"
#include "app_sensor/app_sensor.h"
#include "app_battery/app_battery.h"
#include "app_battery/bsp_battery.h"

static void test_work_handler(struct k_work *work);
static void test_tmr_handler(struct k_timer *timer);
static void test_tmr_stop_handler(struct k_timer *timer);

K_TIMER_DEFINE(m_test_timer, test_tmr_handler, test_tmr_stop_handler);
K_WORK_DEFINE(m_test_work, test_work_handler);
static sys_slist_t plist;
static sys_slist_t tlist;
static int m_dtype = 1;

static void sensor_get_all(int disp_type)
{
	const struct shell* shell = shell_backend_uart_get_ptr();
	int ret=0;

	/* print Pamb, Ptube */
#if CONFIG_APP_HAS_PRESSURE_SENSOR
	ret = app_sensor_pressure_get_all(&plist);
	if (!ret) {
		struct pressure_val *pv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Pressure:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&plist, pv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", ((double)pv->val*PRESS_KPA_TO_CMH2O_MUL));
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f cm H2O", pv->id, ((double)pv->val*PRESS_KPA_TO_CMH2O_MUL));
			}
		}
	}
	app_sensor_pressure_delete_list(&plist);
#endif	/* CONFIG_APP_HAS_PRESSURE_SENSOR */

	/* print Tamb, Ttube */
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
	ret = app_sensor_temp_c_get_all(&tlist);
	if (!ret) {
		struct temperature_val *tv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Temperature:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tlist, tv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", (double)tv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f C", tv->id, (double)tv->val);
			}
		}
	}
	app_sensor_temperature_delete_list(&tlist);
#endif	/* CONFIG_APP_HAS_TEMPERATURE_SENSOR */

	/* print Blower RPM, Blower voltage */
	struct app_blower_params blower;
	ret = app_blower_runtime_params_get(&blower);
	if (disp_type == 1 /* csv */) {
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%d,%d,", blower.acq_speed_rpm, blower.acq_volt_mv);
	} else {
		shell_warn(shell, "RPM: %d, Blower voltage: %d mv", blower.acq_speed_rpm, blower.acq_volt_mv);
	}

	/* print battery voltage, system current */
	int vsys, ibus, ibat, isys;
	int batt_stat = app_battery_check_enable_charging();
	if (batt_stat == BSP_BATT_CONNECTED)
		bsp_battery_vbat_get(&vsys);
	else if (batt_stat == BSP_BATT_DISCONNECTED)
		bsp_battery_vbus_get(&vsys);
	bsp_battery_ibus_get(&ibus);
	bsp_battery_ibat_get(&ibat);
	isys = abs(ibus - ibat);

/*
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	int chan = 0;
	int attr_vbat, attr_ibat, attr_ibus = 0;
	struct sensor_value val_vbat, val_ibat, val_ibus;
#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr_vbat = POWER_SUPPLY_PROP_VOLTAGE_VBAT_NOW;
	attr_ibat = POWER_SUPPLY_PROP_CURRENT_VBAT_NOW;
	attr_ibus = POWER_SUPPLY_PROP_CURRENT_VBUS_NOW;
#endif
	ret = sensor_attr_get(dev, chan, attr_vbat, &val_vbat);
	ret = sensor_attr_get(dev, chan, attr_ibat, &val_ibat);
	ret = sensor_attr_get(dev, chan, attr_ibus, &val_ibus);
	vbat = val_vbat.val1/1000;
	isys = (val_ibus.val1/1000) - (val_ibat.val1/1000);
	isys = abs(isys);
*/
	if (disp_type == 1 /* csv */) {
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%d,%d", vsys, isys);
	} else {
		shell_warn(shell, "Battery voltage: %d mv, System curr = %d ma", vsys, isys);
	}

	/* insert a new line character */
	if (disp_type == 1 /* csv */) {
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "\n");
	}
}

static void test_work_handler(struct k_work *work)
{
	sensor_get_all(m_dtype);
}

static void test_tmr_handler(struct k_timer *timer)
{
	k_work_submit(&m_test_work);
}

static void test_tmr_stop_handler(struct k_timer *timer)
{
	const struct shell* shell = shell_backend_uart_get_ptr();
	shell_print(shell, "test stopped");
}

static int ptrvc_run(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 3) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int ret = 0;
	/* apply the pid setvalue */
	float sv = strtof(argv[1], NULL);
	app_blower_pid_sv_change(sv);
	shell_print(shell, "P set to %0.2f cmH2O", (double)sv);

	/* check blower status and turn blower on */
	uint8_t blower_state=0;
	blower_state = app_blower_run_state_get();
	if (blower_state) {
		shell_print(shell, "!!! Blower is running. Stop blower and then run the test !!!");
	}
	else {
		ret = app_blower_settings_change_state(APP_BLOWER_START);
		if (!ret)	shell_print(shell, "Blower started ...");
		else {
			shell_print(shell, "Failed to start Blower, aborting test");
			return -1;
		}
	}

	/* enable battery discharge sensing */
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	} else {
		ret = bsp_battery_ibat_discharge_sensing_control(BSP_BATT_IBAT_DIS_SENSE_ENABLE);
		if (ret)
			shell_warn(shell, "could not enable battery discharge sensing");
	}

	/* check if interval is present */
	if (argc == 3) {
		uint32_t ms_interval = strtol(argv[2], NULL, 10);
		if (ms_interval < 25) {
			shell_print(shell, "interval must be greater than or equal to 25 ms");
			return -1;
		}

		/* print the csv columns headings */
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW,
				"Pamb(cmH2O),Ptube(cmH2O),Tamb(C),Ttube(C),RPM,Vmotor(mv),Vbat(mv),Isys(ma)");
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "\n");

		/* start timer and acquire sensor values */
		k_timer_start(&m_test_timer, K_MSEC(ms_interval), K_MSEC(ms_interval));
	} else {
		/* start single shot timer and acquire sensor values */
		k_timer_start(&m_test_timer, K_NO_WAIT, K_NO_WAIT);
	}

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int test_stop(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* stop blower */
	int ret = app_blower_settings_change_state(APP_BLOWER_STOP);
//	if (!ret)	shell_print(shell, "Blower started ...");

	/* disable battery discharge sensing */
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	} else {
		ret = bsp_battery_ibat_discharge_sensing_control(BSP_BATT_IBAT_DIS_SENSE_DISABLE);
		if (ret)
			shell_warn(shell, "could not disable battery discharge sensing");
	}

	k_timer_stop(&m_test_timer);
//	shell_print(shell, "test stopped");
	return 0;
}

/* test */
SHELL_STATIC_SUBCMD_SET_CREATE(test_subcmds,
		SHELL_CMD_ARG(ptrvc, NULL, "Measure P, T, RPM, V, C. Inputs (1) P in cmH2O, (2) measurement interval in ms",
				ptrvc_run, 2, 1),
		SHELL_CMD(stop, NULL, "Stop test", test_stop),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(test, &test_subcmds, "Commands used for running various tests", NULL);
