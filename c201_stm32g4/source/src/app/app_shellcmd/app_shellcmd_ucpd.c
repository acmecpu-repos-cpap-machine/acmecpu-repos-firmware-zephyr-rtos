/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 28-Sept-2023
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

#if (CONFIG_STUSB4500)
	#include "stusb4500.h"
#endif

static int ucpd_contr_init(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_INIT;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int ucpd_contr_softreset(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SOFT_RESET;
#endif
	ret = sensor_attr_set(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int ucpd_contr_rdo_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val[3];

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_RDO;
#endif
	ret = sensor_attr_get(dev, chan, attr, val);
	if (!ret) {
		shell_print(shell, MSG_PASS);
		LOG_INF("Object Pos: %d", val[0].val1);
		LOG_INF("Voltage: %d mv", val[1].val1);
		LOG_INF("Max Current: %d", val[2].val1);
		LOG_INF("Operating Current: %d", val[2].val2);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int ucpd_contr_src_pdo_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SRCPDO;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int ucpd_contr_typec_stat_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val = {0,0};

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_CC_STATUS;
#endif
	ret = sensor_attr_get(dev, chan, attr, &val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int ucpd_contr_pdo_update(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 4) {
		shell_print(shell, MSG_FAIL": incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t pdo_num = strtol(argv[1], NULL, 10);
	int mvolts = strtol(argv[2], NULL, 10);
	int mamps = strtol(argv[3], NULL, 10);

	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(ucpd));
	if (!device_is_ready(dev)) {
		shell_print(shell, MSG_FAIL": ucpd device is not ready");
		return -1;
	}

	int ret = 0;
	int chan = 0;
	int attr = 0;
	struct sensor_value val[2];
	val[0].val1 = pdo_num;
	val[1].val1 = mvolts;
	val[1].val2 = mamps;

#if CONFIG_STUSB4500
	chan = USBCPD_CHAN_UCPD_CONTR;
	attr = USBCPD_ATTR_SNKPDO;
#endif
	ret = sensor_attr_set(dev, chan, attr, val);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

/* ucpd */
SHELL_STATIC_SUBCMD_SET_CREATE(ucpd_subcmds,
		SHELL_CMD(init, NULL, "initialize the USB C PD controller", ucpd_contr_init),
		SHELL_CMD(softreset, NULL, "send a soft reset message to the peer", ucpd_contr_softreset),
		SHELL_CMD(rdo, NULL, "Get the RDO", ucpd_contr_rdo_get),
		SHELL_CMD(src_pdo, NULL, "Get source PDO", ucpd_contr_src_pdo_get),
		SHELL_CMD(typec_stat, NULL, "Get Type C connection status", ucpd_contr_typec_stat_get),
		SHELL_CMD_ARG(update_pdo, NULL, "Set new PDO value to a PDO number. Usage: update_pdo 2 9000 2000", ucpd_contr_pdo_update, 4, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(ucpd, &ucpd_subcmds, "UCPD commands for external controller like stusb4500", NULL);


