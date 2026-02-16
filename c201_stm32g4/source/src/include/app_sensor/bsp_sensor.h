/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 14-Dec-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_SENSOR_BSP_SENSOR_H_
#define SRC_INCLUDE_APP_SENSOR_BSP_SENSOR_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>

#define SENS_NAME_SZ 20

struct sinfo {
	/* This is meant to be used in the library and the user should not mess with it */
	sys_snode_t node;

	/* Data */
	enum sensor_channel chan;
	char name[SENS_NAME_SZ];
	const struct device *dev;
	uint8_t id;
	bool status;
};

/**
 * @brief
 * 		Checks sensors of a given channel type and returns a populated list of struct sinfo data
 * 		Also returns the count of active sensors of a particular type as an out parameter
 * @param
 * 		chan[in]			sensor channel type
 * 		sinfo_list[in out]	list variable passed by caller to be populated
 * 		sens_count[out]		number of sensors of a particular type
 *
 * @return
 * 		0 success
 * 		-ENOMEM	no memory
 */
int bsp_sensor_info_create(enum sensor_channel chan, sys_slist_t *sinfo_list, int *sens_count);

/**
 * @brief
 * 		Deletes the list previously created by bsp_sensor_info_create()
 * @note bsp_sensor_info_create() should have been called previously
 *
 * @param
 * 		sinfo_list[in]	list variable to be deleted
 *
 * @return
 * 		0 SUCCESS
 * 		other values FAILURE
 */
int bsp_sensor_info_destroy(sys_slist_t *sinfo_list);

/**
 * @brief
 * 		Gets value of a particular sensor channel
 *
 * @param
 * 		dev [in] 	the device address of the sensor channel
 * 		chan [in]	the channel number
 * 		val[out]	output value
 *
 * @return
 * 		0 SUCCESS
 * 		other values FAILURE
 */
int bsp_sensor_value_get(const struct device *dev, uint8_t chan, struct sensor_value *val);

/**
 * acquire pressure in Kilo Pascal
 */
int bsp_sensor_pressure_kpa_get(const struct device *dev, struct sensor_value *press_kpa);

/**
 * acquire humidity,temp and pressure
 */
int bsp_sensor_humid_get(const struct device *dev, struct sensor_value *humid);

/**
 * acquire fluid level
 */
/**
 * @brief
 * 		Checks sensors of a given channel in this case is distance and fetches the values from the sensor
 * @param
 * 		dev[in] 	the device address of the sensor channel
 * 		distance[out]	output value
 *
 * @return
 * 		0 success
 * 		-1	fail
 */
int bsp_sensor_distance_get(const struct device *dev, struct sensor_value *distance);

/**
 * acquire temperature in degree Celcius
 */
int bsp_sensor_temp_c_get(const struct device *dev, struct sensor_value *temp_c);

/**
 * acquire 3-axis accelerometer values
 */
int bsp_sensor_3a_accel_get(const struct device *dev, struct sensor_value *accel);

/**
 * acquire gyroscope values
 */
int bsp_sensor_gyro_get(const struct device *dev, struct sensor_value *gyro);

/**
 * initialize all sensors
 */
int bsp_sensor_init();

#endif /* SRC_INCLUDE_APP_SENSOR_BSP_SENSOR_H_ */
