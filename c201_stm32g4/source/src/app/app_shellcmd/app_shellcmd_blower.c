/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 20-Jan-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#include "app_settings/app_settings.h"
#include "app_blower/app_blower.h"

#define OLD_BLOWER_CMDS	1

static int shellcmd_blower_start(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower start");

	/* Start the blower */
	int ret = app_blower_settings_change_state(APP_BLOWER_START);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_stop(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower stop");

	/* Stop the blower */
	int ret = app_blower_settings_change_state(APP_BLOWER_STOP);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

#if (OLD_BLOWER_CMDS)
static int shellcmd_blower_set_voltage(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("blower set_voltage: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower %s %s", (argv[0]), (argv[1]));

	uint32_t voltage_mv = strtol(argv[1], NULL, 10);

	/* set the blower voltage */
	int ret = app_blower_voltage_mv_change(voltage_mv);
	if (!ret)	shell_print(shell, MSG_PASS);
	else if (ret == -ENOTSUP)	{shell_print(shell, MSG_FAIL); shell_print(shell, "only works in TEST mode");}
	else if (ret == -EINVAL)	{shell_print(shell, MSG_FAIL); shell_print(shell, "value out of range");}
	else						{shell_print(shell, MSG_FAIL); shell_print(shell, "could not set voltage");}

	return ret;
}

static int shellcmd_blower_set_duty(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("blower set_duty: incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower %s %s", (argv[0]), (argv[1]));

	uint8_t duty_percent = strtol(argv[1], NULL, 10);

	/* set the blower voltage */
	int ret = app_blower_duty_percent_change(duty_percent);
	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_get_volts_mv(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower get_volts_mv");

	/* set the blower voltage */
	struct app_blower_params blower;
	int ret = app_blower_runtime_params_get(&blower);
	uint32_t volts_mv = blower.acq_volt_mv;

	if (!ret)	shell_print(shell, "%d", volts_mv);
	else if (ret == -ENOTSUP)	{shell_print(shell, MSG_FAIL); shell_print(shell, "only works in TEST mode");}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_get_speed_hz(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower get_speed_hz");

	/* get the blower params */
	struct app_blower_params blower;
	int ret = app_blower_runtime_params_get(&blower);
	uint32_t speed_hz = blower.acq_speed_hz;

	if (!ret)	shell_print(shell, "%d", speed_hz);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_params_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower get_speed_rpm");

	/* get the blower params */
	struct app_blower_params blower;
	int ret = app_blower_runtime_params_get(&blower);
//	int32_t speed_rpm = blower.acq_speed_rpm;
//	int ret = app_blower_speed_rpm_get(&speed_rpm);

	if (!ret) {
		shell_print(shell, "volts = %d mv", blower.acq_volt_mv);
		shell_print(shell, "rpm = %d", blower.acq_speed_rpm);
		shell_print(shell, "faults = 0x%x", blower.faults);
	}
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_set_speed_rpm(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower rpm_set");

	/* set the blower speed */

	int32_t speed_rpm = strtol(argv[1], NULL, 10);
	LOG_INF("speed_rpm = %d", speed_rpm);
	int ret = app_blower_speed_rpm_change(speed_rpm);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}
#endif /* #if (OLD_BLOWER_CMDS) */
static int shellcmd_blower_status_get(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower status_get");

	/* get the blower state */
	uint8_t blower_state=0;
	int ret = 0;
//	ret = app_blower_state_get(&blower_state);
	blower_state = app_blower_run_state_get();

	if (!ret) {
		if (!blower_state)	shell_print(shell, "STOPPED");
		else				shell_print(shell, "RUNNING");
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
static int shellcmd_blower_fault_ack(const struct shell *shell, size_t argc, char **argv) {

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower faulta");

	/* ack blower faults */
	int ret = app_blower_fault_ack();

	if (!ret) {
		shell_print(shell, MSG_PASS);
	}
	else {
		shell_print(shell, MSG_FAIL);
	}

	return ret;
}
#endif

static int shellcmd_blower_set_oper_mode(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower mode_set");

	/* set the blower speed */

	uint8_t mode = strtol(argv[1], NULL, 10);
	LOG_INF("mode = %d", mode);
	int ret = app_blower_oper_mode_set(mode);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}

static int shellcmd_blower_oper_mode_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower mode_get");

	uint8_t mode = app_blower_oper_mode_get();
	shell_print(shell, "operating mode = %d", mode);

	return 0;
}

static int shellcmd_blower_set_ramp(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower ramp_set");

	/* set the blower speed */

	uint32_t ramp = strtol(argv[1], NULL, 10);
	LOG_INF("ramp = %d ms", ramp);
	int ret = app_blower_ramp_ms_set(ramp);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL); shell_print(shell, "only works in TEST mode");}

	return ret;
}

static int shellcmd_blower_get_ramp(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower ramp_get");

	uint32_t ramp = app_blower_ramp_ms_get();
	shell_print(shell, "ramp = %d ms", ramp);

	return 0;
}

#if (CONFIG_APP_BLOWER_VOLTAGE_SLOPE_CONTROL)
static int shellcmd_blower_set_vslope_intv(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower vslope_intv_set");

	/* set the blower speed */

	uint32_t vslope_intv = strtol(argv[1], NULL, 10);
	LOG_INF("vslope_intv = %d ms", vslope_intv);
	int ret = app_blower_vslope_intv_set(vslope_intv);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL); shell_print(shell, "only works in PID mode");}

	return ret;
}

static int shellcmd_blower_get_vslope_intv(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower vslope_intv_get");

	uint32_t vslope_intv = app_blower_vslope_intv_get();
	shell_print(shell, "vslope_intv = %d ms", vslope_intv);

	return 0;
}

static int shellcmd_blower_set_vslope_thres(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower vslope_thres_set");

	/* set the blower speed */

	float vslope_thres = strtof(argv[1], NULL);
	LOG_INF("vslope_thres = %f ms", vslope_thres);
	int ret = app_blower_vslope_thres_set(vslope_thres);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL); shell_print(shell, "only works in PID mode");}

	return ret;
}

static int shellcmd_blower_get_vslope_thres(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower vslope_thres_get");

	float vslope_thres = app_blower_vslope_thres_get();
	shell_print(shell, "vslope_thres = %f", vslope_thres);

	return 0;
}
#endif

static int shellcmd_blower_power_get(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower power_get");

	uint16_t power_avg_w = 0;
	uint16_t power_inst_w = 0;
	int ret = -1;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = app_blower_power_get(&power_avg_w, &power_inst_w);
#endif
	if (!ret) {
		shell_print(shell, "power avg = %d W", power_avg_w);
		shell_print(shell, "power inst = %d W", power_inst_w);
	}
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

#if (BLOWER_POWER_CONSUMP)
static int shellcmd_blower_power_vget(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower power_get");

	uint16_t thresh = 0;
	uint32_t count = 0;
	uint32_t intv = 0;
	int ret = app_blower_power_settings_get(&thresh, &count, &intv);
	if (!ret) {
		shell_print(shell, "power threshold = %d W, recovery count = %d, measurement interval = %d ms", thresh, count, intv);
	}
	else
		shell_print(shell, MSG_FAIL);

	return 0;
}

static int shellcmd_blower_power_vset(const struct shell *shell, size_t argc, char **argv)
{
	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower power_vset");

	int ret = 0;
	uint16_t thres = strtol(argv[1], NULL, 10);
	LOG_INF("power thres = %d W", thres);
	ret = app_blower_power_threshold_set(thres);

	if (argc >= 3) {
		uint32_t count = strtol(argv[2], NULL, 10);
		LOG_INF("recovery count = %d", count);
		ret |= app_blower_power_recovery_count_set(count);
	}

	if (argc >= 4) {
		uint32_t intv = strtol(argv[3], NULL, 10);
		LOG_INF("measure interval = %d ms", intv);
		ret |= app_blower_power_interval_set(intv);
	}

	if (!ret)	shell_print(shell, MSG_PASS);
	else		shell_print(shell, MSG_FAIL);

	return ret;
}
#endif /* BLOWER_POWER_CONSUMP */

static int shellcmd_blower_set_kp(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower pid kp_set");

	/* set the blower pid kp */

	float kp = strtof(argv[1], NULL);
	LOG_INF("KP = %0.2f",(double)kp);
	int ret = app_blower_kp_set(kp);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL);}

	return ret;
}

static int shellcmd_blower_set_ki(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower pid ki_set");

	/* set the blower pid ki */

	float ki = strtof(argv[1], NULL);
	LOG_INF("KI = %0.2f",(double)ki);
	int ret = app_blower_ki_set(ki);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL);}

	return ret;
}

static int shellcmd_blower_set_kd(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower pid kd_set");

	/* set the blower pid kd */

	float kd = strtof(argv[1], NULL);
	LOG_INF("KD = %0.2f",(double)kd);
	int ret = app_blower_kd_set(kd);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL);}

	return ret;
}

static int shellcmd_blower_set_sample_time(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower pid sample_time_set");

	/* set the blower pid kd */

	float sample_time = strtof(argv[1], NULL);
	LOG_INF("Sample time = %0.2f",(double)sample_time);
	int ret = app_blower_sample_time_set(sample_time);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL);}

	return ret;
}

static int shellcmd_blower_set_ramp_div_const(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: blower pid ramp_div_set");

	/* set the blower pid kd */

	int ramp_div = strtof(argv[1], NULL);
	LOG_INF("Ramp_div = %d",ramp_div);
	int ret = app_blower_ramp_div_set(ramp_div);

	if (!ret)	shell_print(shell, MSG_PASS);
	else		{shell_print(shell, MSG_FAIL);}

	return ret;
}

/* blower */
SHELL_STATIC_SUBCMD_SET_CREATE(blower_subcmds,
		SHELL_CMD(on, NULL, "blower on", shellcmd_blower_start),
		SHELL_CMD(off, NULL, "blower off", shellcmd_blower_stop),
#if (OLD_BLOWER_CMDS)
		SHELL_CMD_ARG(mv_set, NULL, "milli-volts set", shellcmd_blower_set_voltage, 2, 0),
		SHELL_CMD_ARG(duty_set, NULL, "duty set", shellcmd_blower_set_duty, 2, 0),
		SHELL_CMD(mv_get, NULL, "current milli-volts get", shellcmd_blower_get_volts_mv),
		SHELL_CMD(hz_get, NULL, "speed in Hz get", shellcmd_blower_get_speed_hz),
		SHELL_CMD(paramsget, NULL, "blower params get", shellcmd_blower_params_get),
		SHELL_CMD_ARG(rpm_set, NULL, "speed in RPM set", shellcmd_blower_set_speed_rpm, 2, 0),
		SHELL_CMD_ARG(kp_set, NULL, "PID KP set", shellcmd_blower_set_kp, 2, 0),
		SHELL_CMD_ARG(ki_set, NULL, "PID KI set", shellcmd_blower_set_ki, 2, 0),
		SHELL_CMD_ARG(kd_set, NULL, "PID KD set", shellcmd_blower_set_kd, 2, 0),
		SHELL_CMD_ARG(sample_time_set, NULL, "PID sample time ms set", shellcmd_blower_set_sample_time, 2, 0),
		SHELL_CMD_ARG(ramp_div_set, NULL, "PID ramp_div_const set", shellcmd_blower_set_ramp_div_const, 2, 0),
#endif
		SHELL_CMD(stat_get, NULL, "running status get", shellcmd_blower_status_get),
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
		SHELL_CMD(faulta, NULL, "fault ack", shellcmd_blower_fault_ack),
#endif
		SHELL_CMD_ARG(mode_set, NULL, "operating mode set (0 = PID, 1 = TEST)", shellcmd_blower_set_oper_mode, 2, 0),
		SHELL_CMD(mode_get, NULL, "operating mode get", shellcmd_blower_oper_mode_get),
		SHELL_CMD_ARG(ramp_set, NULL, "ramp duration set in ms", shellcmd_blower_set_ramp, 2, 0),
		SHELL_CMD(ramp_get, NULL, "ramp duration get in ms", shellcmd_blower_get_ramp),
#if (CONFIG_APP_BLOWER_VOLTAGE_SLOPE_CONTROL)
		SHELL_CMD_ARG(vslope_intv_set, NULL, "vslope data acquisition interval set in ms", shellcmd_blower_set_vslope_intv, 2, 0),
		SHELL_CMD(vslope_intv_get, NULL, "vslope data acquisition interval get in ms", shellcmd_blower_get_vslope_intv),
		SHELL_CMD_ARG(vslope_thres_set, NULL, "vslope threshold set (float)", shellcmd_blower_set_vslope_thres, 2, 0),
		SHELL_CMD(vslope_thres_get, NULL, "vslope threshold set", shellcmd_blower_get_vslope_thres),
#endif
		SHELL_CMD(power_get, NULL, "power get in watts", shellcmd_blower_power_get),
#if (BLOWER_POWER_CONSUMP)
		SHELL_CMD(power_vget, NULL, "get power masurement settings", shellcmd_blower_power_vget),
		SHELL_CMD_ARG(power_vset, NULL, "power_vset threshold recovery_count measure_interval", shellcmd_blower_power_vset, 2, 2),
#endif /* BLOWER_POWER_CONSUMP */
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(blower, &blower_subcmds, "Blower control commands", NULL);

