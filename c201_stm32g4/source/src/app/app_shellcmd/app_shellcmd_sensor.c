/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 20-Jan-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <version.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);

#include "app_shellcmd/app_shellcmd.h"
#include "app_settings/app_settings.h"
#include "app_sensor/app_sensor.h"
#include "app_blower/app_blower.h"
#include "app_fluid_level/app_fluid_level.h"

static void timer_expired_handler(struct k_timer *timer);
static void sens_work_handler(struct k_work *work);
/* function pointer to be called from the timer handler to acquire the sensor data */
typedef void (*acquire_data_t)(int);

//static struct k_timer m_sens_timer;
static acquire_data_t m_sens_acq;
static int m_dtype;
K_TIMER_DEFINE(m_sens_timer, timer_expired_handler, NULL);
K_WORK_DEFINE(m_sens_work, sens_work_handler);

static uint8_t m_accel_id = 0;
static uint8_t m_gyro_id = 0;

static void sens_work_handler(struct k_work *work) {
	if (m_sens_acq != NULL) {
		m_sens_acq(m_dtype);
	}
}

static void timer_expired_handler(struct k_timer *timer) {
	k_work_submit(&m_sens_work);
}

static int sensor_list(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int ret=0;

	/* get info of all sensors */

	int sensor_count = 0;
	sys_slist_t *sens_list = app_sensor_info_get(&sensor_count);
	shell_print(shell, "Number of sensors = %d\n", sensor_count);
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(sens_list, sen, tmp, node)
	{
		switch (sen->chan) {
		case SENSOR_CHAN_PRESS:
			shell_print(shell, "Channel: %d, PRESSURE", sen->chan);
			//m_press_id = sen->id;
			break;
		case SENSOR_CHAN_AMBIENT_TEMP:
			shell_print(shell, "Channel: %d, AMBIENT_TEMP", sen->chan);
			//m_temp_id = sen->id;
			break;
		case SENSOR_CHAN_ACCEL_XYZ:
			shell_print(shell, "Channel: %d, ACCELEROMETER_XYZ", sen->chan);
			m_accel_id = sen->id;
			break;
		case SENSOR_CHAN_GYRO_XYZ:
			shell_print(shell, "Channel: %d, GYROSCOPE_XYZ", sen->chan);
			m_gyro_id = sen->id;
			break;
		case SENSOR_CHAN_HUMIDITY:
			shell_print(shell, "Channel: %d, HUMIDITY", sen->chan);
			break;
		case SENSOR_CHAN_DISTANCE:
					shell_print(shell, "Channel: %d, DISTANCE", sen->chan);
					break;
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
			shell_print(shell, "id: %d, \t val: %0.2f kPa", pv->id, (double)pv->val);
		}
	}

	app_sensor_pressure_delete_list(&plist);

	return ret;
}

static int shellcmd_press_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float pressure_kpa;
	int ret = app_sensor_pressure_kpa_get(id, &pressure_kpa);
	if (!ret)
		shell_print(shell, "%.4f kPa", (double)pressure_kpa);
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
		struct temperature_val *tv, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tlist, tv, tmp, node)
		{
			shell_print(shell, "id: %d, \t val: %0.4f C", tv->id, (double)tv->val);
		}
	}

	app_sensor_temperature_delete_list(&tlist);

	return ret;
}

static int shellcmd_temp_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float temp_c;
	int ret = app_sensor_temp_c_get(id, &temp_c);
	if (!ret)
		shell_print(shell, "%.4f C", (double)temp_c);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif	/* CONFIG_APP_HAS_TEMPERATURE_SENSOR */

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
sys_slist_t hlist;
static int shellcmd_humid_get_all(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret=0;

	ret = app_sensor_humid_get_all(&hlist);

	if (!ret) {
		struct humidity_val *hv, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&hlist, hv, tmp, node)
		{
			shell_print(shell, "id: %d, \t val: %0.2f percent", hv->id, (double)hv->val);
		}
	}

	app_sensor_humidity_delete_list(&hlist);

	return ret;
}

static int shellcmd_humid_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float humid_per;
	int ret = app_sensor_humid_percent_get(id, &humid_per);
	if (!ret)
		shell_print(shell, "%.4f percent", (double)humid_per);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
sys_slist_t dlist;
static int shellcmd_distance_get_all(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	int ret=0;

	ret = app_sensor_distance_get_all(&dlist);

	if (!ret) {
		struct distance_val *dv, *tmp;
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlist, dv, tmp, node)
		{
			shell_print(shell, "id: %d, \t val: %0.2f mm", dv->id, (double)dv->val);
		}
	}

	app_sensor_distance_delete_list(&dlist);

	return ret;
}

static int shellcmd_distance_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float distance_mm;
	int ret = app_sensor_distance_mm_get(id, &distance_mm);
	if (!ret)
		shell_print(shell, "%.4f mm", (double)distance_mm);
	else
		shell_print(shell, "sensor not found!");

	return ret;
}

static int shellcmd_waterlevel_get_mm(const struct shell *shell, size_t argc, char **argv) {

	int raw_val_mm, liquidlevel_mm;
	int ret = app_fluid_level_mm(&raw_val_mm, &liquidlevel_mm);
	if (!ret) {
		shell_print(shell, "Raw distance - %d mm", raw_val_mm);
		shell_print(shell, "Liquid level - %d mm", liquidlevel_mm);
	} else
		shell_print(shell, "Err");

	return ret;
}

#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
static int shellcmd_accel_get(const struct shell *shell, size_t argc, char **argv) {
	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	uint8_t id = strtol(argv[1], NULL, 10);

	float x, y, z;
	int ret = app_sensor_3a_accel_get(id, &x, &y, &z);
	if (!ret) {
		shell_print(shell, "x %.2f, \t y %.2f, \t z %.2f m/s^2", (double)x,(double)y,(double)z);
//		shell_print(shell, "%.1f,%.1f,%.1f", x,y,z);
	} else
		shell_print(shell, "sensor not found!");

	return ret;
}
#endif	/* CONFIG_APP_HAS_ACCELEROMETER_SENSOR */

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
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", ((double)pv->val*PRESS_KPA_TO_CMH2O_MUL));
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f cm H2O", pv->id, ((double)pv->val*PRESS_KPA_TO_CMH2O_MUL));
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
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", (double)tv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f C", tv->id, (double)tv->val);
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

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
	ret = app_sensor_humid_get_all(&hlist);
	if (!ret) {
		struct humidity_val *hv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Humidity:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&hlist, hv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", (double)hv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f per", hv->id, (double)hv->val);
			}
		}
	}
	app_sensor_humidity_delete_list(&hlist);
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
	ret = app_sensor_distance_get_all(&dlist);
	if (!ret) {
		struct distance_val *dv, *tmp;
		if (disp_type != 1 /* not csv */) {
			shell_warn(shell, "Distance:");
		}
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlist, dv, tmp, node)
		{
			if (disp_type == 1 /* csv */) {
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,", (double)dv->val);
			} else {
				shell_warn(shell, "id: %d, \t val: %0.4f mm", dv->id, (double)dv->val);
			}
		}
	}
	app_sensor_distance_delete_list(&dlist);
#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
	/* get accelerometer values */
	float x, y, z;
	ret = app_sensor_3a_accel_get(m_accel_id, &x, &y, &z);
	if (disp_type == 1 /* csv */) {
//		rd += sprintf(csv_buf+rd, "%0.2f,%0.2f,%0.2f", x,y,z);
//		shell_print(shell, "%s", csv_buf);
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.2f,%0.2f,%0.2f", (double)x,(double)y,(double)z);
	} else {
		shell_warn(shell, "Accelerometer: x %.2f, \t y %.2f, \t z %.2f", (double)x,(double)y,(double)z);
		shell_print(shell, "--------");
	}
#endif	/* CONFIG_APP_HAS_ACCELEROMETER_SENSOR */

#if CONFIG_APP_HAS_IMU_SENSOR
	/* get accelerometer values */
	float ax, ay, az, gx, gy, gz;
	/* TODO: remove hardcoded IDs */
	ret = app_sensor_imu_get(m_accel_id, &ax, &ay, &az, m_gyro_id, &gx, &gy, &gz);

	if (disp_type == 1 /* csv */) {
//		rd += sprintf(csv_buf+rd, "%0.2f,%0.2f,%0.2f", x,y,z);
//		shell_print(shell, "%s", csv_buf);
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,%0.4f,%0.4f,%0.4f,%0.4f,%0.4f", (double)ax, (double)ay, (double)az, (double)gx, (double)gy, (double)gz);
	} else {
		shell_warn(shell, "Accelerometer: x %.4f, \t y %.4f, \t z %.4f", (double)ax, (double)ay, (double)az);
		shell_warn(shell, "Gyroscope: x %.4f, \t y %.4f, \t z %.4f", (double)gx, (double)gy, (double)gz);
		shell_print(shell, "--------");
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

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
		if(m_accel_id == 0) {
			shell_print(shell, "sensor list must be called once before this command");
			return -1;
		}
#endif
#if CONFIG_APP_HAS_IMU_SENSOR
		if(m_gyro_id == 0) {
			shell_print(shell, "sensor list must be called once before this command");
			return -1;
		}
#endif

		/* print the csv columns headings */
	if (dtype == 1 /* csv */) {
#if CONFIG_APP_HAS_PRESSURE_SENSOR
		ret = app_sensor_pressure_get_all(&plist);
		if (!ret) {
			struct pressure_val *pv, *tmp;
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&plist, pv, tmp, node)
			{
					shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "press[%d](cmH2O),", pv->id);
			}
		}
		app_sensor_pressure_delete_list(&plist);
#endif
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
		ret = app_sensor_temp_c_get_all(&tlist);
		if (!ret) {
			struct temperature_val *tv, *tmp;
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&tlist, tv, tmp, node)
			{
					shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "temp[%d](C),", tv->id);
			}
		}
		app_sensor_temperature_delete_list(&tlist);
#endif
#if CONFIG_APP_HAS_HUMIDITY_SENSOR
		ret = app_sensor_humid_get_all(&hlist);
		if (!ret) {
			struct humidity_val *hv, *tmp;
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&hlist, hv, tmp, node)
			{
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "humid[%d](per),", hv->id);
			}
		}
		app_sensor_humidity_delete_list(&hlist);
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
		ret = app_sensor_distance_get_all(&dlist);
		if (!ret) {
			struct distance_val *dv, *tmp;
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&dlist, dv, tmp, node)
			{
				shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "distance[%d](mm),", dv->id);
			}
		}
		app_sensor_distance_delete_list(&dlist);
#endif
#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "a[%d]X,a[%d]Y,a[%d]Z",
				m_accel_id, m_accel_id, m_accel_id);
#endif
#if CONFIG_APP_HAS_IMU_SENSOR
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "a[%d]X,a[%d]Y,a[%d]Z,g[%d]X,g[%d]Y,g[%d]Z",
				m_accel_id, m_accel_id, m_accel_id, m_gyro_id, m_gyro_id, m_gyro_id);
#endif
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "\n");
	}

	if (argc == 3) {
		uint32_t ms_interval = strtol(argv[2], NULL, 10);
		if (ms_interval < 25) {
			shell_print(shell, "interval must be greater than or equal to 25 ms");
			return -1;
		}
		/* start timer and acquire sensor values */
		m_sens_acq = sensor_get_all;
		k_timer_start(&m_sens_timer, K_MSEC(ms_interval), K_MSEC(ms_interval));
//		shell_print(shell, "sensor allget started");
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

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
/* humid sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(humid_subcmds,
		SHELL_CMD(all, NULL, "Get humidity value of all sensors", shellcmd_humid_get_all),
		SHELL_CMD_ARG(get, NULL, "usage: sensor temp get <id>", shellcmd_humid_get, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
/* fluid_level sensor */
SHELL_STATIC_SUBCMD_SET_CREATE(fluid_level_subcmds,
		SHELL_CMD(all, NULL, "Get distance value of all sensors", shellcmd_distance_get_all),
		SHELL_CMD(liquidlevel_get, NULL, "Get liquid level calculated by the sensor", shellcmd_waterlevel_get_mm),
		SHELL_CMD_ARG(get, NULL, "usage: sensor distance get <id>", shellcmd_distance_get, 2, 0),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
#endif

SHELL_STATIC_SUBCMD_SET_CREATE(sensor_subcmds,
		SHELL_CMD(list, NULL, "List all sensors", sensor_list),
#if CONFIG_APP_HAS_PRESSURE_SENSOR
		SHELL_CMD(pressure, &pressure_subcmds, "Pessure sensor cmds", NULL),
#endif
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
		SHELL_CMD(temp, &temp_subcmds, "Temperature sensor cmds", NULL),
#endif
#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
		SHELL_CMD(accel, &accel_subcmds, "Accelerometer cmds", NULL),
#endif
		SHELL_CMD_ARG(allget, NULL, "Get value of all sensors in a loop with interval. Usage: <sensor allget csv/pretty 100(ms optional)>",
				shellcmd_sensor_all_get, 2, 1),
		SHELL_CMD(stop, NULL, "Stop sensor allget loop", shellcmd_sensor_stop),
#if CONFIG_APP_HAS_HUMIDITY_SENSOR
		SHELL_CMD(humid, &humid_subcmds, "Humidity sensor cmds", NULL),
#endif
#if CONFIG_APP_HAS_FLUID_LEVEL
		SHELL_CMD(fluid_level, &fluid_level_subcmds, "fluid_level sensor cmds", NULL),
#endif
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(sensor, &sensor_subcmds, "Sensor commands", NULL);

