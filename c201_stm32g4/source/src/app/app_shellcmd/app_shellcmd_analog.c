/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 26-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */


#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#if (CONFIG_APP_ANALOG)
	#include "app_analog/app_analog.h"
#endif

static int adc_volt_vbat_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	int32_t val_mv = 0;
	ret = app_analog_measure_en(APP_ANALOG_VBAT);
	k_sleep(K_MSEC(100));
	ret |= app_analog_vbat_mv_get(&val_mv);
//	k_sleep(K_MSEC(10));
	ret |= app_analog_measure_dis(APP_ANALOG_VBAT);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VBAT mvolts = %d", val_mv);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int adc_volt_vbus_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	int32_t val_mv = 0;
	ret = app_analog_measure_en(APP_ANALOG_VBUS);
	k_sleep(K_MSEC(100));
	ret |= app_analog_vbus_mv_get(&val_mv);
//	k_sleep(K_MSEC(10));
	ret |= app_analog_measure_dis(APP_ANALOG_VBUS);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VBUS mvolts = %d", val_mv);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int adc_volt_dcjack_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = 0;
	int32_t val_mv = 0;
	ret = app_analog_measure_en(APP_ANALOG_PWRJACK);
	k_sleep(K_MSEC(100));
	ret |= app_analog_pwrjack_mv_get(&val_mv);
//	k_sleep(K_MSEC(10));
	ret |= app_analog_measure_dis(APP_ANALOG_PWRJACK);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "DCJACK mvolts = %d", val_mv);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int adc_volt_allget(const struct shell *shell, size_t argc, char **argv)
{
	adc_volt_vbat_get(shell, argc, argv);
	adc_volt_vbus_get(shell, argc, argv);
	adc_volt_dcjack_get(shell, argc, argv);

	return 0;
}

/* analog */
SHELL_STATIC_SUBCMD_SET_CREATE(analog_subcmds,
		SHELL_CMD(vbat, NULL, "Print the VBAT voltage", adc_volt_vbat_get),
		SHELL_CMD(vbus, NULL, "Print the VBUS voltage", adc_volt_vbus_get),
		SHELL_CMD(dcjack, NULL, "Print the DC jack input voltage", adc_volt_dcjack_get),
		SHELL_CMD(allget, NULL, "Command to fire all the get commands", adc_volt_allget),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(analog, &analog_subcmds, "Analog voltage commands", NULL);
