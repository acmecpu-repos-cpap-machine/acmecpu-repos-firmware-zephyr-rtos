/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 15-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
// #include "app_settings/app_settings.h"
#include "app_sensor/app_sensor.h"
#if (CONFIG_APP_HAS_PPG_SENSOR)
#include "shmax30101.h"
#endif

static void timer_expired_handler(struct k_timer *timer);
static void sens_work_handler(struct k_work *work);
/* function pointer to be called from the timer handler to acquire the sensor data */
typedef void (*acquire_data_t)(int);

//static struct k_timer m_sens_timer;
static acquire_data_t m_sens_acq;
static int m_dtype;
K_TIMER_DEFINE(m_sens_timer, timer_expired_handler, NULL);
K_WORK_DEFINE(m_sens_work, sens_work_handler);

static void sens_work_handler(struct k_work *work) {
	if (m_sens_acq != NULL) {
		m_sens_acq(m_dtype);
	}
}

static void timer_expired_handler(struct k_timer *timer) {
	k_work_submit(&m_sens_work);
}

static int shellcmd_list(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret=0;

	/* get info of all sensors */

	int sensor_count = 0;
	int channel = -1;
	sys_slist_t *sens_list = app_sensor_info_get(&sensor_count);
	/* print */
	shell_print(shell, "Number of sensors = %d\n", sensor_count);
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(sens_list, sen, tmp, node)
	{
		channel = sen->chan;	// done to get rid of compiler warnings
		switch (channel) {
		case SENSOR_CHAN_PRESS:
			shell_print(shell, "Channel: %d, PRESSURE", sen->chan);
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			shell_print(shell, "Channel: %d, AMBIENT_TEMP", sen->chan);
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			shell_print(shell, "Channel: %d, ACCELEROMETER_XYZ", sen->chan);
			break;
		case SENSOR_CHAN_GYRO_XYZ:
			shell_print(shell, "Channel: %d, GYROSCOPE_XYZ", sen->chan);
			break;
#if CONFIG_APP_HAS_PPG_SENSOR
		case SENSOR_CHAN_HEART_RATE:
			shell_print(shell, "Channel: %d, HEART RATE", sen->chan);
			break;
		case SENSOR_CHAN_SPO2:
			shell_print(shell, "Channel: %d, SPO2", sen->chan);
			break;
		case SENSOR_CHAN_HR_AND_SPO2:
			shell_print(shell, "Channel: %d, HR and SPO2", sen->chan);
			break;
#endif	/*CONFIG_APP_HAS_PPG_SENSOR*/
		default:
			break;
		}
		shell_print(shell, "id: %d, name: %s, stat: %s", sen->id, sen->name, sen->status ? "OK" : "ERR");
		shell_print(shell, "");
	}

	return ret;
}

#if CONFIG_APP_HAS_PRESSURE_SENSOR
sys_slist_t plist;
static int shellcmd_press_get_all(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret=0;

	ret = app_sensor_pressure_get_all(&plist);

	if (!ret) {
		struct pressure_val *pv, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&plist, pv, tmp, node)
		{
			shell_print(shell, "id: %d, \t val: %0.2f kPa", pv->id, pv->val);
		}
	}

	app_sensor_pressure_delete_list(&plist);

	return ret;
}

static int shellcmd_press_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float pressure_kpa;
	int ret = app_sensor_pressure_kpa_get(id, &pressure_kpa);
	if (!ret)
		shell_print(shell, "%.4f kPa", pressure_kpa);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif	/* CONFIG_APP_HAS_PRESSURE_SENSOR */

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
sys_slist_t tlist;
static int shellcmd_temp_get_all(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret=0;

	ret = app_sensor_temp_c_get_all(&tlist);

	if (!ret) {
		struct pressure_val *tv, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tlist, tv, tmp, node)
		{
			shell_print(shell, "id: %d, \t val: %0.4f C", tv->id, tv->val);
		}
	}

	app_sensor_temperature_delete_list(&tlist);

	return ret;
}

static int shellcmd_temp_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float temp_c;
	int ret = app_sensor_temp_c_get(id, &temp_c);
	if (!ret)
		shell_print(shell, "%.4f C", temp_c);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif	/* CONFIG_APP_HAS_TEMPERATURE_SENSOR */

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
static int shellcmd_accel_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float x, y, z;
	int ret = app_sensor_3a_accel_get(id, &x, &y, &z);
	if (!ret) {
		shell_print(shell, "x %.2f, \t y %.2f, \t z %.2f m/s^2", x,y,z);
//		shell_print(shell, "%.1f,%.1f,%.1f", x,y,z);
	} else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif	/* CONFIG_APP_HAS_ACCELEROMETER_SENSOR */

#if CONFIG_APP_HAS_PPG_SENSOR
static int shellcmd_ppg_start(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	int ret = app_sensor_ppg_get_start(id);
	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}
static int shellcmd_ppg_stop(const struct shell *shell, size_t argc, char **argv)
{
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);
	int ret = app_sensor_ppg_get_stop(id);

	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}
#endif	/* CONFIG_APP_HAS_PPG_SENSOR */

#if CONFIG_APP_HAS_EKG_SENSOR
static int shellcmd_ekg_start(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = app_sensor_ekg_start();
	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}
static int shellcmd_ekg_stop(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret = app_sensor_ekg_stop();

	if (!ret)
		shell_print(shell, "OK");
	else
		shell_print(shell, "ERR");

	return ret;
}
#endif	/*CONFIG_APP_HAS_EKG_SENSOR*/

static void sensor_get_all(int disp_type) {
	const struct shell* shell = shell_backend_uart_get_ptr();
	int ret=0;
//	int rd=0;
//	char csv_buf[64] = {0x00};

#if CONFIG_APP_HAS_PRESSURE_SENSOR
	/* get all pressure sensor values */
	ret = app_sensor_pressure_get_all(&plist);
	if (!ret) {
		struct pressure_val *pv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Pressure:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&plist, pv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
//				rd += sprintf(csv_buf, "%.4f,", pv->val);
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", pv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f kPa", pv->id, pv->val);
			}
		}
	}
	app_sensor_pressure_delete_list(&plist);
#endif	/* CONFIG_APP_HAS_PRESSURE_SENSOR */

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
	/* get temperature sensor values */
//	float temp_c;
//	ret = app_sensor_temp_c_get(5, &temp_c);
	ret = app_sensor_temp_c_get_all(&tlist);
	if (!ret) {
		struct temperature_val *tv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Temperature:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tlist, tv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
//				rd += sprintf(csv_buf, "%.4f,", pv->val);
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", tv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f C", tv->id, tv->val);
			}
		}
	}
	app_sensor_temperature_delete_list(&tlist);

//	if (disp_type == 1 /* csv */) {
////		rd += sprintf(csv_buf+rd, "%0.4f,", temp_c);
//		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", temp_c);
//	} else {
//		shell_warn(shell, "Temperature: %0.4f", temp_c);
//	}

#endif	/* CONFIG_APP_HAS_TEMPERATURE_SENSOR */

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
	/* get accelerometer values */
	float x, y, z;
	ret = app_sensor_3a_accel_get(1, &x, &y, &z);
	if (disp_type == 1 /* csv */) {
//		rd += sprintf(csv_buf+rd, "%0.2f,%0.2f,%0.2f", x,y,z);
//		shell_print(shell, "%s", csv_buf);
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.2f,%0.2f,%0.2f", x,y,z);
	} else {
		shell_warn(shell, "Accelerometer: x %.2f, \t y %.2f, \t z %.2f", x,y,z);
		shell_print(shell, "--------");
	}
#endif	/* CONFIG_APP_HAS_ACCELEROMETER_SENSOR */

#if CONFIG_APP_HAS_IMU_SENSOR
	/* get accelerometer values */
	float ax, ay, az, gx, gy, gz;
	
	/* get IDs */
	uint8_t a_ids[10] = {0};
	uint8_t g_ids[10] = {0};
	int a_count=0, g_count=0;
	app_sensor_chan_to_id(SENSOR_CHAN_ACCEL_XYZ, a_ids, &a_count);
	app_sensor_chan_to_id(SENSOR_CHAN_GYRO_XYZ, g_ids, &g_count);
	
	if (a_count == g_count) {
		for (int i=0; i<a_count; i++) {
			ret = app_sensor_imu_get(a_ids[i], &ax, &ay, &az, g_ids[i], &gx, &gy, &gz);
			if (disp_type == 1 /* csv */) {
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f", ax, ay, az, gx, gy, gz);
			} else {
				shell_warn(shell, "a_id: %d", a_ids[i]);
				shell_warn(shell, "Acce: x %.4f, \t y %.4f, \t z %.4f", ax, ay, az);
				shell_warn(shell, "g_id: %d", g_ids[i]);
				shell_warn(shell, "Gyro: x %.4f, \t y %.4f, \t z %.4f", gx, gy, gz);
				shell_print(shell, "--------");
			}
		}
	} else {
		shell_print(shell, "ACCEL and GYRO channel count mismatch");
	}
#endif	/* CONFIG_APP_HAS_IMU_SENSOR */

	if (disp_type == 1 /* csv */) {
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "\n");
	}
}

static int shellcmd_sensor_all_get(const struct shell *shell, size_t argc, char **argv) {
	int ret=0;

	int dtype = 0; /* 0 - pretty, 1 - csv */
	if (!strcmp(argv[1], "csv")) {
		dtype = 1;
	}
	m_dtype = dtype;

	if (argc == 3) {
		uint32_t ms_interval = strtol(argv[2], NULL, 10);
		if (ms_interval < 10) {
			shell_print(shell, "interval must be greater than or equal to 10 ms");
			return -1;
		}

		/* start timer and acquire sensor values */
		m_sens_acq = sensor_get_all;
		k_timer_start(&m_sens_timer, K_MSEC(ms_interval), K_MSEC(ms_interval));
		shell_print(shell, "sensor allget started");
	} else {
		/* start single shot timer and acquire sensor values */
		m_sens_acq = sensor_get_all;
		k_timer_start(&m_sens_timer, K_NO_WAIT, K_NO_WAIT);
	}

	return ret;
}

static int shellcmd_sensor_stop(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	k_timer_stop(&m_sens_timer);
	shell_print(shell, "sensor allget stopped");
	return 0;
}

static int shellcmd_sensor_getone(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		shell_print(shell, "incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	/* check the channel */
	int chan=0;
	int ret = app_sensor_id_to_chan(id, &chan);
	if (	(chan == SENSOR_CHAN_ACCEL_XYZ)
			||  (chan == SENSOR_CHAN_GYRO_XYZ)
#if	CONFIG_APP_HAS_PPG_SENSOR
			|| (chan == SENSOR_CHAN_HR_AND_SPO2)
#endif	/*CONFIG_APP_HAS_PPG_SENSOR*/
		) {
		shell_print(shell, "not supported by this command, try sensor allget ...");
		return -1;
	}

	struct sensor_value val;
	ret = app_sensor_value_get(id, &val);
	if (!ret)
		shell_print(shell, "val1: %d\t val2: %d", val.val1, val.val2);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}

#if CONFIG_APP_HAS_PRESSURE_SENSOR
/* pressure sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(pressure_subcmds,
		SHELL_CMD(all, NULL, "Get pressure value of all sensors", shellcmd_press_get_all),
		SHELL_CMD_ARG(get, NULL, "usage: sensor pressure get <id>", shellcmd_press_get, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
/* temperature sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(temp_subcmds,
		SHELL_CMD(all, NULL, "Get temperature value of all sensors", shellcmd_temp_get_all),
		SHELL_CMD_ARG(get, NULL, "usage: sensor temp get <id>", shellcmd_temp_get, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
/* Accelerometer sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(accel_subcmds,
		SHELL_CMD_ARG(get, NULL, "usage: sensor accel get <id> (in m/s^2)", shellcmd_accel_get, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

#if CONFIG_APP_HAS_PPG_SENSOR
/* PPG sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(ppg_subcmds,
		SHELL_CMD_ARG(start, NULL, "usage: sensor ppg start <id>", shellcmd_ppg_start, 2, 0),
		SHELL_CMD(stop, NULL, "stop ppg sensor data acquisition", shellcmd_ppg_stop),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

#if CONFIG_APP_HAS_EKG_SENSOR
/* EKG sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(ekg_subcmds,
		SHELL_CMD(start, NULL, "start ekg sensor data acquisition", shellcmd_ekg_start),
		SHELL_CMD(stop, NULL, "stop ekg sensor data acquisition", shellcmd_ekg_stop),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif	/*CONFIG_APP_HAS_EKG_SENSOR*/

SHELL_STATIC_SUBCMD_SET_CREATE(sensor_subcmds,
		SHELL_CMD(list, NULL, "List all sensors", shellcmd_list),
#if CONFIG_APP_HAS_PRESSURE_SENSOR
		SHELL_CMD(pressure, &pressure_subcmds, "Pessure sensor cmds", NULL),
#endif
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
		SHELL_CMD(temp, &temp_subcmds, "Temperature sensor cmds", NULL),
#endif
#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
		SHELL_CMD(accel, &accel_subcmds, "Accelerometer cmds", NULL),
#endif
#if CONFIG_APP_HAS_PPG_SENSOR
		SHELL_CMD(ppg, &ppg_subcmds, "PPG cmds", NULL),
#endif
#if CONFIG_APP_HAS_EKG_SENSOR
		SHELL_CMD(ekg, &ekg_subcmds, "EKG cmds", NULL),
#endif
		SHELL_CMD_ARG(allget, NULL, "Get value of all sensors in a loop with interval. Usage: <sensor allget csv/pretty 100(ms optional)>",
				shellcmd_sensor_all_get, 2, 1),
		SHELL_CMD(stop, NULL, "Stop sensor allget loop", shellcmd_sensor_stop),
		SHELL_CMD_ARG(getone, NULL, "Get value of a sensor against its id. Usage: <sensor getval id>", shellcmd_sensor_getone, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(sensor, &sensor_subcmds, "Sensor commands", NULL);

