/*
 * Copyright (c) 2021 STMicroelectronics
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                         opensource.org/licenses/BSD-3-Clause
 *
 *Copyright (c) 2024 Acme CPU
 *
 * app_fluid_level.c
 *
 *  Created on: 30-Apr-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_fluid_level);
#include <zephyr/drivers/sensor.h>
#include <zephyr/fs/fs.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "app_sensor/app_sensor.h"
#include "app_fluid_level/app_fluid_level.h"

/*Number of sampling data needed for averaging*/
#define SAMPLE_COUNT 10

/*Number of measuring levels in percentage*/
#define MAX_LABEL 10

/*File name which consists compensation data*/
#define FATFS_MNTPP "/lfs"
#define FILE_NAME FATFS_MNTPP"/vl53l4cd_compensation_data.txt"
/*#define FAT_FILE_DATA fat_fs*/

/*Total distance from sensor to the bottom of the container*/
/*TODO Later it will be read from the file*/
/*#define SENSOR_PLACEMENT 36*/

/*Sample compensation table data for testing*/
/*TODO Later it will be read from the file*/
typedef struct compensation_table {
	int level;
	int expected_level;
	int err_val;
	int sensor_placement;
} compensation_table;

compensation_table compensation_table_inst[MAX_LABEL];

/*
 compensation_table compensation_table_inst[MAX_LABEL] = {
 {  9, 31, 12, 36 },
 {  8,32, 13, 36 },
 {  7, 33, 14, 36 },
 {  6, 34, 14, 36 },
 {  5, 35, 10, 36 },
 {  4, 36, 10, 36 },
 {  3, 37, 9, 36 },
 {  2, 38, 9, 36 },
 {  1, 40, 8, 36 },
 {  0, 41, 0, 36 },
 };
 */

/*Calculates the average of the raw distance*/
static int app_fluid_level_ranging(int *mean_distance_mm) {

	int ret, i = 0;
	float raw_distance_mm;
	int mean =0;
	int mean_ranging_mm = 0;

	/*Loop to fetch the data for averaging*/
	for (i = 0; i < SAMPLE_COUNT; i++) {

		/*Fetch the raw data from fluid_level sensor*/
		/*TODO Change the sensor id while testing*/
		ret = app_sensor_distance_mm_get(3, &raw_distance_mm);
		if (!ret) {

			mean_ranging_mm = mean_ranging_mm + (int)raw_distance_mm;

			/*Sensor data not found*/
		} else {
			LOG_ERR("fluid_level sensor not found");
			return -1;
		}
	}

	/*Calculates the average*/
	mean = mean_ranging_mm / SAMPLE_COUNT;
	*mean_distance_mm = mean;

	return ret;
}

/*Measures the liquid level*/
static int app_fluid_level_measurement(int distance_mm, int *liquidlevel_mm) {

	int ret, i = 0;
	int value, pos, measured_level, comp_val;

	/*Traverse through the compensation table*/
	for (i = 0; i < MAX_LABEL; i++) {

		/*Checking if the sensor height is correct*/
		if (distance_mm < compensation_table_inst[i].sensor_placement + 5) {

			/*Checks if the mean distance less than the expected distance*/
			if (distance_mm < compensation_table_inst[i].expected_level) {

				/*Condition where the water level is greater than zero percentage */
				if (i > 0) {

					/*Averages the expected water distance after searching from the compensation table*/
					value = compensation_table_inst[i].expected_level + compensation_table_inst[i].expected_level / 2;
				}

				/*Condition where water level is zero percent*/
				else {
					value = compensation_table_inst[i].expected_level;
				}

				if (distance_mm <= value) {

					/*First position*/
					if (i == 0) {
						pos = 1;
						break;
					}
					pos = 1 + (i - 1);
					break;
				}

				else {
					pos = 1 + i;
					break;
				}
			}

			else {
				if (distance_mm > compensation_table_inst[i].sensor_placement + 5) {
					LOG_ERR("Sensor is not placed correctly");
					return -1;
				} else
					pos = 6;
			}
		}

		else {
			LOG_ERR("Valid level not found");
			return -1;
		}
	}

	/*Compares the value according to the position in compensation table*/
	comp_val = distance_mm - compensation_table_inst[pos].err_val;

	/*TODO while testing if the code needs modification then this section can be uncommented*/
	/*Compares the value according to the position in compensation table*/
	/*	 switch (pos)
	 {
	 case 1:
	 comp_val = distance_mm + compensation_table_inst[pos].err_val;
	 break;
	 case 2:
	 comp_val = distance_mm + compensation_table_inst[pos].err_val;
	 break;
	 case 3:
	 comp_val = distance_mm + compensation_table_inst[pos].err_val;
	 break;
	 case 4:
	 comp_val = distance_mm + compensation_table_inst[pos].err_val;
	 break;
	 case 5:
	 comp_val = distance_mm - compensation_table_inst[pos].err_val;
	 break;
	 case 6:
	 comp_val = distance_mm - compensation_table_inst[pos].err_val;
	 break;
	 case 7:
	 comp_val = distance_mm - compensation_table_inst[pos].err_val;
	 break;
	 case 8:
	 comp_val = distance_mm - compensation_table_inst[pos].err_val;
	 break;
	 case 9:
	 comp_val = distance_mm - compensation_table_inst[pos].err_val;
	 break;
	 default:
	 LOG_ERR("Valid water level not found");
	 return -1;
	 }
	 */

	/*Calculated water level*/
	measured_level = compensation_table_inst[i].sensor_placement - comp_val;

	*liquidlevel_mm = measured_level;

	return ret;
}

/*Calculates the water level in mm*/
int app_fluid_level_mm(int *pval_raw_mm, int *pval_liquid_mm) {

	int ret = 0;
	int distance_mm;
	int liquidlevel_mm;

	/*Mean of the distance is calculated*/
	ret = app_fluid_level_ranging(&distance_mm);
	if (!ret) {

		*pval_raw_mm = distance_mm;

		/*Calculates the liquid level using compensation table*/
		ret = app_fluid_level_measurement(distance_mm, &liquidlevel_mm);
		if (!ret) {

			*pval_liquid_mm = liquidlevel_mm;

		} else {
			LOG_ERR("Liquid level measurement failed");
			return -1;
		}

	} else {
		LOG_ERR("Ranging error");
		return -1;
	}
	return ret;
}

/*TODO add it where all the init functions are called*/
int app_fluid_level_init() {

	int count = 0;
	int ret = 0;
	struct fs_file_t file;
	int rc;

	/*Init function for file system*/
	fs_file_t_init(&file);

	/*Opening the file in the  read mode*/
	rc = fs_open(&file, FILE_NAME, FS_O_READ);
	if (rc < 0) {
		LOG_ERR("FAIL: open the file");
		return rc;
	}

	/*Reading the csv file from the storage and fetches the compensation table from there*/
	char line[15];

	while (fs_read(&file, line, sizeof(line))) {

		/*strtok() method splits string according to given delimiters and returns the next data*/
		char *data = strtok(line, ",\n");
		/*Copy the data to the global structure also converts string data to integer*/
		compensation_table_inst[count].level = atoi(data);

		data = strtok(NULL, ",\n");
		compensation_table_inst[count].expected_level = atoi(data);

		data = strtok(NULL, ",\n");
		compensation_table_inst[count].err_val = atoi(data);

		data = strtok(NULL, ",\n");
		compensation_table_inst[count].sensor_placement = atoi(data);

		count++;
		if (count >= MAX_LABEL) {
			break;
		}
	}
	fs_close(&file);

	return ret;
}
