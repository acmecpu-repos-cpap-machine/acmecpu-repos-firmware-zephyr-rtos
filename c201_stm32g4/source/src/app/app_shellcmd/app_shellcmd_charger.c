/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 16-Sep-2022
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

#if CONFIG_BQ25792
#include "bq25792.h"
#endif

static int charger_status_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_STATUS;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "status = %d", val.val1);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_charge_type_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CHARGE_TYPE;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "charge type = %d", val.val1);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_isonline_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_ONLINE;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "isonline = %d", val.val1);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_usb_type_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_USB_TYPE;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "usb type = %d", val.val1);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_health_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_HEALTH;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "health = %d", val.val1);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vbus_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_VOLTAGE_VBUS_NOW;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VBUS = %d uv, %d mv", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ibus_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CURRENT_VBUS_NOW;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "IBUS = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vlim_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VLIM = %d uv, %d mv", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ilim_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "ILIM = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vbat_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_VOLTAGE_VBAT_NOW;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VBAT = %d uv, %d mv", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ibat_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CURRENT_VBAT_NOW;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "IBAT = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ichrg_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "ICHRG = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ichrg_max_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "ICHRG MAX = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vchrg_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VCHRG = %d uv, %d mv", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vchrg_max_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "VCHRG MAX = %d uv, %d mv", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_iprechrg_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_PRECHARGE_CURRENT;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "PRECHRG = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_iterm_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		shell_print(shell, "TERM CHRG = %d ua, %d ma", val.val1, (val.val1/1000));
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ilim_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vlim_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_vchrg_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_ichrg_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_iprechrg_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_PRECHARGE_CURRENT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_iterm_set(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_chrg_ctrl(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	int32_t value = strtol(argv[1], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(charger));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": charger device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {value,0};

#if CONFIG_BQ25792
	chan = POWER_SUPPLY_CHAN_CHARGER;
	attr = POWER_SUPPLY_PROP_CHARGE_CONTROL;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int charger_allget(const struct shell *shell, size_t argc, char **argv)
{
	charger_status_get(shell, argc, argv);
	charger_charge_type_get(shell, argc, argv);
	charger_isonline_get(shell, argc, argv);
	charger_usb_type_get(shell, argc, argv);
	charger_health_get(shell, argc, argv);
	charger_vbus_get(shell, argc, argv);
	charger_ibus_get(shell, argc, argv);
	charger_vlim_get(shell, argc, argv);
	charger_ilim_get(shell, argc, argv);
	charger_vbat_get(shell, argc, argv);
	charger_ibat_get(shell, argc, argv);
	charger_ichrg_get(shell, argc, argv);
	charger_ichrg_max_get(shell, argc, argv);
	charger_vchrg_get(shell, argc, argv);
	charger_vchrg_max_get(shell, argc, argv);
	charger_iprechrg_get(shell, argc, argv);
	charger_iterm_get(shell, argc, argv);

	return 0;
}

/* charger */
SHELL_STATIC_SUBCMD_SET_CREATE(charger_subcmds,
		SHELL_CMD(status, NULL, "Get the charging status", charger_status_get),
		SHELL_CMD(charge_type, NULL, "Get the charging type", charger_charge_type_get),
		SHELL_CMD(isonline, NULL, "Check if charger is online", charger_isonline_get),
		SHELL_CMD(usb_type, NULL, "Get the detected USB type", charger_usb_type_get),
		SHELL_CMD(health, NULL, "Get the charger health", charger_health_get),
		SHELL_CMD(vbus, NULL, "Get the charger VBUS voltage", charger_vbus_get),
		SHELL_CMD(ibus, NULL, "Get the charger VBUS current", charger_ibus_get),
		SHELL_CMD(vlim, NULL, "Get the charger input voltage limit", charger_vlim_get),
		SHELL_CMD(ilim, NULL, "Get the charger input current limit", charger_ilim_get),
		SHELL_CMD(vbat, NULL, "Get the battery voltage measured by the charger", charger_vbat_get),
		SHELL_CMD(ibat, NULL, "Get the battery current measured by the charger", charger_ibat_get),
		SHELL_CMD(ichrg, NULL, "Get the charging current", charger_ichrg_get),
		SHELL_CMD(ichrg_max, NULL, "Get the max charging current", charger_ichrg_max_get),
		SHELL_CMD(vchrg, NULL, "Get the charging voltage", charger_vchrg_get),
		SHELL_CMD(vchrg_max, NULL, "Get the max charging voltage", charger_vchrg_max_get),
		SHELL_CMD(iprechrg, NULL, "Get the precharge current", charger_iprechrg_get),
		SHELL_CMD(iterm, NULL, "Get the termination current", charger_iterm_get),
		SHELL_CMD_ARG(ilim_set, NULL, "Set the charger input current limit in uA", charger_ilim_set, 2, 0),
		SHELL_CMD_ARG(vlim_set, NULL, "Set the charger input voltage limit in uV", charger_vlim_set, 2, 0),
		SHELL_CMD_ARG(vchrg_set, NULL, "Set the charging voltage in uV", charger_vchrg_set, 2, 0),
		SHELL_CMD_ARG(ichrg_set, NULL, "Set the charging current in uA", charger_ichrg_set, 2, 0),
		SHELL_CMD_ARG(iprechrg_set, NULL, "Set the precharge current in uA", charger_iprechrg_set, 2, 0),
		SHELL_CMD_ARG(iterm_set, NULL, "Set the termination current in uA", charger_iterm_set, 2, 0),
		SHELL_CMD_ARG(chrg_ctrl, NULL, "Enable or disable the battery charging", charger_chrg_ctrl, 2, 0),
		SHELL_CMD(allget, NULL, "Command to fire all the get commands", charger_allget),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(charger, &charger_subcmds, "Charger commands", NULL);

