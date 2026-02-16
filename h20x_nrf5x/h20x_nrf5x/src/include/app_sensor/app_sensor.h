/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 14-Dec-2021
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
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

/**
 * @brief	Gets all the sensor IDs for a channel
 * 
 * @param
 * 	chan[in]		channel number
 * 	psen_id[out]	array of sensor ids, memory must be allocated by caller
 * 	pcount[out]		number of sensors of type chan
 * 
 * @return
 * 	0 if at least one id is found for the channel
 * 	negative number if no sensor is found or any other error occurs
*/
int app_sensor_chan_to_id(int chan, uint8_t *psen_id, int *pcount);

/**
 * @brief	Gets the sensor channel for a sensor ID
 * 
 * @param
 * 	id[in]		sensor id
 * 	pchan[out]	channel number
 * 
 * @return
 * 	0 if the id was found
 * 	negative number if no sensor id is found or any other error occurs
*/
int app_sensor_id_to_chan(uint8_t id, int *pchan);

/* Pressure sensor */
int app_sensor_pressure_kpa_get(uint8_t sens_id, float *press);
int app_sensor_pressure_get_all(sys_slist_t *press_list);
void app_sensor_pressure_delete_list(sys_slist_t *list);

/* Temperature sensor */
int app_sensor_temp_c_get_all(sys_slist_t *temp_list);
int app_sensor_temp_c_get(uint8_t sens_id, float *temp_c);
void app_sensor_temperature_delete_list(sys_slist_t *list);

/* Accelerometer */
int app_sensor_3a_accel_get(uint8_t sens_id, float *accel_x, float *accel_y, float *accel_z);

/* IMU */
int app_sensor_imu_get(uint8_t a_id, float *a_x, float *a_y, float *a_z, uint8_t g_id, float *g_x, float *g_y, float *g_z);

/* PPG */
/**
 * @brief	Initializes the PPG sensor (heart rate + spo2) and enables it 
 * 			for data acquisition. The data can be obtained by calling
 * 			app_sensor_value_get(). This function must be called once before
 * 			subsequent calls to app_sensor_value_get()
 * 
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ppg_init();

/**
 * @brief	Starts the PPG sensor (heart rate + spo2) thread for data acquisition
 * 
 * @note	Currently only 1 PPG sensor is supported
 * 			
 * @param
 * 	ppg_id[in]	sensor id obtained from app_sensor_chan_to_id()
 * 
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ppg_get_start(uint8_t ppg_id);

/**
 * @brief	Stops the PPG sensor (heart rate + spo2) thread
 *
 * @note	Currently only 1 PPG sensor is supported
 * 			
 * @param
 * 	ppg_id[in]	sensor id obtained from app_sensor_chan_to_id()
 * 
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ppg_get_stop(uint8_t ppg_id);

/**
 * @brief	Stops the PPG sensor (heart rate + spo2) and disables the AFE
 * 
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ppg_deinit();

/* EKG */
/**
 * @brief	Initializes the EKG sensor and enables it 
 * 			for data acquisition. The data can be obtained by calling
 * 			app_sensor_value_get(). This function must be called once before
 * 			subsequent calls to app_sensor_value_get()
 * 
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ekg_init();

/**
 * @brief	Starts the EKG sensor thread for data acquisition
 * 
 * @note	Currently only 1 EKG sensor is supported
 * 			
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ekg_start();

/**
 * @brief	Stops the EKG sensor thread
 *
 * @note	Currently only 1 EKG sensor is supported
 * 			
 * @return
 * 	0 if succeeded
 * 	negative value for failure
*/
int app_sensor_ekg_stop();

/* Get value of a sensor against its id */
int app_sensor_value_get(uint8_t sens_id, struct sensor_value *sens_val);

int app_sensor_init();

#endif /* SRC_INCLUDE_APP_SENSOR_APP_SENSOR_H_ */
