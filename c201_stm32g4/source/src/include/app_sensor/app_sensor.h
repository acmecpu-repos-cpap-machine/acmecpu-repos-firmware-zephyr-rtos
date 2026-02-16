/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 14-Dec-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SENSOR_APP_SENSOR_H_
#define SRC_INCLUDE_APP_SENSOR_APP_SENSOR_H_

#include <zephyr/drivers/sensor.h>
#include "app_sensor/bsp_sensor.h"

#if CONFIG_APP_HAS_PRESSURE_SENSOR
struct pressure_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float val;
};
#endif

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
struct temperature_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float val;
};
#endif

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
struct humidity_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float val;
};
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
struct distance_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float val;
};
#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
struct accel_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float x,y,z;
};
#endif

#if CONFIG_APP_HAS_IMU_SENSOR
struct imu_val {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	uint8_t id;
	float x,y,z;
	float r,p,w;
};
#endif

/**
 * Returns a list of all sensors which are active in the system
 */
sys_slist_t * app_sensor_info_get(int *sens_count);

/**
 * Deletes the list previously created by calling app_sensor_info_get()
 */
//void app_sensor_info_delete(sys_slist_t *sinfo_list);

/* Pressure sensor */
int app_sensor_pressure_kpa_get(uint8_t sens_id, float *press);
int app_sensor_pressure_get_all(sys_slist_t *press_list);
int app_sensor_pressure_get_count(sys_slist_t *press_list);
void app_sensor_pressure_delete_list(sys_slist_t *list);

/* Humidity sensor */
int app_sensor_humid_percent_get(uint8_t sens_id, float *humid_per);
int app_sensor_humid_get_all(sys_slist_t *humid_list);
void app_sensor_humidity_delete_list(sys_slist_t *list);


/**
 * @brief Fetches the distance from the TOF sensor
 * @param distance_mm[out]
 * @return 	0 for success
 * 			-ve for failure
 */
int app_sensor_distance_mm_get(uint8_t sens_id, float *distance_mm);

/**
 * @brief Link-list of TOF sensor
 * @param distance_list[in] list from data has to be fetched
 * @return 	0 for success
 * 			-ve for failure
 */
int app_sensor_distance_get_all(sys_slist_t *distance_list);

/**
 * @brief Link-list of sensors
 * @param list[in] list to be deleted
 * @return 	0 for success
 * 			-ve for failure
 */
void app_sensor_distance_delete_list(sys_slist_t *list);

/* Temperature sensor */
int app_sensor_temp_c_get(uint8_t sens_id, float *temp_c);
int app_sensor_temp_c_get_all(sys_slist_t *temp_list);
int app_sensor_temp_c_get_count(sys_slist_t *temp_list);
void app_sensor_temperature_delete_list(sys_slist_t *list);

/* Accelerometer */
int app_sensor_3a_accel_get(uint8_t sens_id, float *accel_x, float *accel_y, float *accel_z);

/* IMU */
int app_sensor_imu_get(uint8_t a_id, float *a_x, float *a_y, float *a_z, uint8_t g_id, float *g_x, float *g_y, float *g_z);

/* Get value of a sensor against its id */
int app_sensor_value_get(uint8_t sens_id, struct sensor_value *sens_val);

int app_sensor_init();

#endif /* SRC_INCLUDE_APP_SENSOR_APP_SENSOR_H_ */
