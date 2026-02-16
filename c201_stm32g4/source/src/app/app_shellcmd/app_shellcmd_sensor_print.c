/*
 * Copyright (c) 2024 Acme CPU
 *
 *  Created on: 2-Jul-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 *      		Rohan Dey (rohan@acmecpu.com)
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
#include "app_sensor/app_sensor.h"
#include "app_blower/app_blower.h"
#include "app_thread_configs.h"

/*Macros for sensor list thread*/
#define HIGH 1
#define LOW 0
#define SAMPLE_TIME_CSV_MS 100

/*Sensor list thread static variables*/
K_THREAD_STACK_DEFINE(m_p_sensor_list_stack, APP_THREAD_STACK_SIZE_SENSOR_LIST);
static struct k_thread m_p_sensor_list_data;
static k_tid_t m_p_sensor_list_tid;

/*Static global variable to kill the thread*/
static int m_exit_sensor_list_thread = LOW;

/*Global variable to store the sample time for sensor list*/
int m_csv_sample_time = SAMPLE_TIME_CSV_MS;

/*Global variable to store the print time in ms*/
int64_t m_print_time_ms =0;
//int64_t g_print_log =0;


/*Thread to fetch the pressure readings*/
static void sensor_allget_p_thread(void *p1, void *p2, void *p3)
{
	struct press_data *pd = NULL;
	struct k_fifo *fifo = app_blower_press_fifo_get();
	const struct shell *shell = shell_backend_uart_get_ptr();
	int64_t s_time =0;

	while (1) {
		/* dequeue a rx data packet from the fifo */
		pd = k_fifo_get(fifo, K_FOREVER);
		if (pd == NULL) {
			LOG_ERR("k_fifo_get failed");
			continue;
		}

		s_time = m_print_time_ms;

		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%lld,",k_uptime_delta(&s_time));

		for (int i=0; i<4; i++) {
			shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "%0.4f,",
									((double) pd->data[i] * PRESS_KPA_TO_CMH2O_MUL));
		}
		shell_fprintf(shell, SHELL_VT100_COLOR_YELLOW, "\n");

		//LOG_INF("Fetch time: %lldms",k_uptime_delta(&m_print_time_ms));
		free(pd);
		//LOG_INF("Free -----");

		if (m_exit_sensor_list_thread == LOW) {
			LOG_INF("sensor list stop");
			return;
		}

//		k_sleep(K_MSEC(m_csv_sample_time));
	}
}

/*Function to enable the sensor list thread*/
static int shellcmd_sensor_pressure_all_get (const struct shell *shell, size_t argc, char **argv) {

	m_exit_sensor_list_thread = HIGH;
	g_press_csv = HIGH;
	LOG_INF("OK");

	/*Start time of print log in ms*/
	m_print_time_ms = k_uptime_get();

	m_p_sensor_list_tid = k_thread_create(&m_p_sensor_list_data,
			m_p_sensor_list_stack, K_THREAD_STACK_SIZEOF(m_p_sensor_list_stack),
			sensor_allget_p_thread, NULL, NULL, NULL,
			APP_THREAD_PRIO_SENSOR_LIST, 0, K_NO_WAIT);

	k_thread_name_set(m_p_sensor_list_tid, APP_THREAD_NAME_SENSOR_LIST_P);

	return 0;
}

/*Function to stop the sensor list thread*/
static int shellcmd_sensor_stop (const struct shell *shell, size_t argc, char **argv) {

	m_exit_sensor_list_thread = LOW;
	g_press_csv = LOW;
	return 0;
}

/*Function to set the interval for sensor list thread*/
static int shellcmd_sensor_list_interval (const struct shell *shell, size_t argc, char **argv) {

	int ms_interval = strtol(argv[2], NULL, 10);
	LOG_INF("LOG: Sensor list interval : %d",ms_interval);
	m_csv_sample_time = ms_interval;
	return 0;
}


SHELL_STATIC_SUBCMD_SET_CREATE(senprint_subcmds,
		SHELL_CMD(press_csv, NULL, "Pressure sensor allget thread", shellcmd_sensor_pressure_all_get),
		SHELL_CMD(press_stop, NULL, "Stop pressure sensor allget thread", shellcmd_sensor_stop),
		SHELL_CMD(press_allget_interval, NULL, "Pressure sensor allget thread interval set", shellcmd_sensor_list_interval),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(senprint, &senprint_subcmds, "Sensor printing commands", NULL);
