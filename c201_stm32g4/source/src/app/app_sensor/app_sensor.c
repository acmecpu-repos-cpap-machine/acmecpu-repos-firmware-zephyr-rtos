/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 14-Dec-2021
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_sensor);
#include <zephyr/drivers/sensor.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "app_sensor/app_sensor.h"

static sys_slist_t m_sens_list;
static int m_sens_count = 0;

enum app_sensor_types {
	APP_SENSOR_TYPE_START=0,
#if CONFIG_APP_HAS_PRESSURE_SENSOR
	APP_SENSOR_TYPE_PRESSURE,
#endif
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
	APP_SENSOR_TYPE_TEMPERATURE,
#endif
#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
	APP_SENSOR_TYPE_ACCELEROMETER,
#endif
#if CONFIG_APP_HAS_IMU_SENSOR
	APP_SENSOR_TYPE_IMU,
#endif
#if CONFIG_APP_HAS_HUMIDITY_SENSOR
	APP_SENSOR_TYPE_HUMIDITY,
#endif
#if CONFIG_APP_HAS_FLUID_LEVEL
	APP_SENSOR_TYPE_FLUID_LEVEL,
#endif

	APP_SENSOR_TYPE_MAX,
};

//void app_sensor_info_delete(sys_slist_t *sinfo_list) {
//	bsp_sensor_info_destroy(sinfo_list);
//}

sys_slist_t * app_sensor_info_get(int *sens_count) {
#if 0
	int ret=0, count=0;
	*sens_count = 0;

#if CONFIG_APP_HAS_PRESSURE_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_PRESS, sinfo_list, &count);
	if (ret != 0) return ret;
	*sens_count += count;
#endif

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_AMBIENT_TEMP, sinfo_list, &count);
	if (ret != 0) return ret;
	*sens_count += count;
#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_ACCEL_XYZ, sinfo_list, &count);
	if (ret != 0) return ret;
	*sens_count += count;
#endif

#if CONFIG_APP_HAS_IMU_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_GYRO_XYZ, sinfo_list, &count);
	if (ret != 0) return ret;
	*sens_count += count;
#endif

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_HUMIDITY, sinfo_list, &count);
	if (ret != 0) return ret;
	*sens_count += count;
#endif

	return ret;
#else
	*sens_count = m_sens_count;
	return &m_sens_list;
#endif
}

#if CONFIG_APP_HAS_PRESSURE_SENSOR
int app_sensor_pressure_kpa_get(uint8_t sens_id, float *press) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_PRESS) && (sen->id == sens_id)) {
			match=1;
			struct sensor_value s_press_kpa;
			ret = bsp_sensor_pressure_kpa_get(sen->dev, &s_press_kpa);
			if (ret)	return -1;
			/*val1 + val2 * 10^(-6)*/
//			float tmp = ((float)s_press_kpa.val2 / 1000000);
//			*press_pa = ((float) s_press_kpa.val1 + tmp) * 1000;

			*press = sensor_value_to_double(&s_press_kpa);
		}
	}

	if (!match) {
		*press = 0;
		ret = -1;
	}
	return ret;
}

int app_sensor_pressure_get_all(sys_slist_t *press_list) {
	int ret = 0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_PRESS) {
			struct pressure_val *pv = (struct pressure_val*) calloc(1,
					sizeof(struct pressure_val));
			if (!pv)
				return -ENOMEM;

			struct sensor_value s_press_kpa;
			ret = bsp_sensor_pressure_kpa_get(sen->dev, &s_press_kpa);
			if (ret)
				return -1;

			pv->id = sen->id;
			pv->val = sensor_value_to_double(&s_press_kpa);
			sys_slist_append(press_list, &pv->node);
		}
	}
	return ret;
}

int app_sensor_pressure_get_count(sys_slist_t *press_list) {
	int count = 0;
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_PRESS)
			count++;
	}
	return count;
}

void app_sensor_pressure_delete_list(sys_slist_t *list) {
	struct pressure_val *pv, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, pv, tmp, node)
	{
		if (pv) {
			sys_slist_remove(list, NULL, &pv->node);
			free(pv);
		}
	}
}
#endif	/* CONFIG_APP_HAS_PRESSURE_SENSOR */

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
int app_sensor_humid_get_all(sys_slist_t *humid_list) {
	int ret = 0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_HUMIDITY) {
			struct humidity_val *hv = (struct humidity_val*) calloc(1,
					sizeof(struct humidity_val));
			if (!hv)
				return -ENOMEM;

			struct sensor_value s_humid_per;
			ret = bsp_sensor_humid_get(sen->dev, &s_humid_per);
			if (ret)
				return -1;

			hv->id = sen->id;
			hv->val = sensor_value_to_double(&s_humid_per);
			sys_slist_append(humid_list, &hv->node);
			}
		}
		return ret;
}
int app_sensor_humid_percent_get(uint8_t sens_id, float *humid_per) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_HUMIDITY) && (sen->id == sens_id)) {
			match=1;
			struct sensor_value s_humid_per;
			ret = bsp_sensor_humid_get(sen->dev, &s_humid_per);
			if (ret)	return -1;
			*humid_per = sensor_value_to_double(&s_humid_per);
		}
	}

	if (!match) {
		*humid_per = 0;
		ret = -1;
	}
	return ret;
}

void app_sensor_humidity_delete_list(sys_slist_t *list) {
	struct humidity_val *hv, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, hv, tmp, node)
	{
		if (hv) {
			sys_slist_remove(list, NULL, &hv->node);
			free(hv);
		}
	}
}

#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
int app_sensor_distance_get_all(sys_slist_t *distance_list) {
	int ret = 0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_DISTANCE) {
			struct distance_val *dv = (struct distance_val*) calloc(1,
					sizeof(struct distance_val));
			if (!dv)
				return -ENOMEM;

			struct sensor_value s_distance_mm;
			ret = bsp_sensor_distance_get(sen->dev, &s_distance_mm);
			if (ret)
				return -1;

			dv->id = sen->id;
			dv->val = sensor_value_to_double(&s_distance_mm);
			sys_slist_append(distance_list, &dv->node);
			}
		}
		return ret;
}

int app_sensor_distance_mm_get(uint8_t sens_id, float *distance_mm) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_DISTANCE) && (sen->id == sens_id)) {
			match=1;
			struct sensor_value s_distance_mm;
			ret = bsp_sensor_distance_get(sen->dev, &s_distance_mm);
			if (ret)	return -1;
			*distance_mm = sensor_value_to_double(&s_distance_mm);
		}
	}

	if (!match) {
		*distance_mm = 0;
		ret = -1;
	}
	return ret;
}

void app_sensor_distance_delete_list(sys_slist_t *list) {
	struct distance_val *dv, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, dv, tmp, node)
	{
		if (dv) {
			sys_slist_remove(list, NULL, &dv->node);
			free(dv);
		}
	}
}

#endif

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
int app_sensor_temp_c_get(uint8_t sens_id, float *temp_c) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_AMBIENT_TEMP) && (sen->id == sens_id)) {
			match=1;
			struct sensor_value s_temp_c;
			ret = bsp_sensor_temp_c_get(sen->dev, &s_temp_c);
			if (ret)	return -1;
			*temp_c = sensor_value_to_double(&s_temp_c);
		}
	}

	if (!match) {
		*temp_c = 0;
		ret = -1;
	}
	return ret;
}

int app_sensor_temp_c_get_all(sys_slist_t *temp_list) {
	int ret = 0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_AMBIENT_TEMP) {
			struct temperature_val *tv = (struct temperature_val*) calloc(1, sizeof(struct temperature_val));
			if (!tv)
				return -ENOMEM;

			struct sensor_value s_temp_c;
			ret = bsp_sensor_temp_c_get(sen->dev, &s_temp_c);
			if (ret)
				return -1;

			tv->id = sen->id;
			tv->val = sensor_value_to_double(&s_temp_c);
			sys_slist_append(temp_list, &tv->node);
		}
	}
	return ret;
}

int app_sensor_temp_c_get_count(sys_slist_t *temp_list) {
	int count = 0;
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->chan == SENSOR_CHAN_AMBIENT_TEMP)
			count++;
	}
	return count;
}

void app_sensor_temperature_delete_list(sys_slist_t *list) {
	struct temperature_val *tv, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, tv, tmp, node)
	{
		if (tv) {
			sys_slist_remove(list, NULL, &tv->node);
			free(tv);
		}
	}
}

#endif	/* CONFIG_APP_HAS_TEMPERATURE_SENSOR */

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
int app_sensor_3a_accel_get(uint8_t sens_id, float *accel_x, float *accel_y, float *accel_z) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_ACCEL_XYZ) && (sen->id == sens_id)) {
			match=1;

			struct sensor_value accel[3];
			ret = bsp_sensor_3a_accel_get(sen->dev, accel);
			if (ret) return -1;

			*accel_x = sensor_value_to_double(&accel[0]);
			*accel_y = sensor_value_to_double(&accel[1]);
			*accel_z = sensor_value_to_double(&accel[2]);
		}
	}

	if (!match) {
		*accel_x = 0;
		*accel_y = 0;
		*accel_z = 0;
		ret = -1;
	}

	return ret;
}
#endif	/* CONFIG_APP_HAS_ACCELEROMETER_SENSOR */

#if CONFIG_APP_HAS_IMU_SENSOR
int app_sensor_imu_get(uint8_t a_id, float *a_x, float *a_y, float *a_z, uint8_t g_id, float *g_x, float *g_y, float *g_z) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_ACCEL_XYZ) && (sen->id == a_id)) {
			match++;

			struct sensor_value accel[3];
			ret = bsp_sensor_3a_accel_get(sen->dev, accel);
			if (ret) return -1;

			*a_x = sensor_value_to_double(&accel[0]);
			*a_y = sensor_value_to_double(&accel[1]);
			*a_z = sensor_value_to_double(&accel[2]);
		}
		if ((sen->chan == SENSOR_CHAN_GYRO_XYZ) && (sen->id == g_id)) {
			match++;

			struct sensor_value gyro[3];
			ret = bsp_sensor_gyro_get(sen->dev, gyro);
			if (ret) return -1;

			*g_x = sensor_value_to_double(&gyro[0]);
			*g_y = sensor_value_to_double(&gyro[1]);
			*g_z = sensor_value_to_double(&gyro[2]);
		}
	}

	if (match < 2) {
		ret = -1;
	}
	return ret;
}

static void setup_imu(const struct device *const dev)
{
	struct sensor_value acc[3], gyr[3];
	struct sensor_value full_scale, sampling_freq, oversampling;

	if (!device_is_ready(dev)) {
		LOG_ERR("Device %s is not ready\n", dev->name);
		return;
	}

	LOG_INF("Device %p name is %s\n", dev, dev->name);

	/* Setting scale in G, due to loss of precision if the SI unit m/s^2
	 * is used
	 */
	full_scale.val1 = 2;            /* G */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change to 0.0Hz before changing
	 * other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);


	/* Setting scale in degrees/s to match the sensor scale */
	full_scale.val1 = 500;          /* dps */
	full_scale.val2 = 0;
	sampling_freq.val1 = 100;       /* Hz. Performance mode */
	sampling_freq.val2 = 0;
	oversampling.val1 = 1;          /* Normal mode */
	oversampling.val2 = 0;

	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			&full_scale);
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			&oversampling);
	/* Set sampling frequency last as this also sets the appropriate
	 * power mode. If already sampling, change sampling frequency to
	 * 0.0Hz before changing other attributes
	 */
	sensor_attr_set(dev, SENSOR_CHAN_GYRO_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY,
			&sampling_freq);

	sensor_sample_fetch(dev);

	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, acc);
	sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyr);

	printf("AX: %d.%06d; AY: %d.%06d; AZ: %d.%06d; "
	       "GX: %d.%06d; GY: %d.%06d; GZ: %d.%06d;\n",
	       acc[0].val1, acc[0].val2,
	       acc[1].val1, acc[1].val2,
	       acc[2].val1, acc[2].val2,
	       gyr[0].val1, gyr[0].val2,
	       gyr[1].val1, gyr[1].val2,
	       gyr[2].val1, gyr[2].val2);

}

#endif	/* CONFIG_APP_HAS_IMU_SENSOR */

int app_sensor_value_get(uint8_t sens_id, struct sensor_value *sens_val) {
	int ret = 0, match=0;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (sen->id == sens_id) {
//			switch (sen->id) {
//			case SENSOR_CHAN_ACCEL_X:
//				break;
//			case SENSOR_CHAN_ACCEL_Y:
//				break;
//			case SENSOR_CHAN_ACCEL_Z:
//				break;
//			case SENSOR_CHAN_AMBIENT_TEMP:
//				break;
//			case SENSOR_CHAN_PRESS:
//				break;
//			default:
//				break;
//			}
			match=1;
			ret = bsp_sensor_value_get(sen->dev, sen->chan, sens_val);
			if (ret)	return -1;
		}
	}

	if (!match) {
		ret = -1;
	}
	return ret;
}


int app_sensor_init() {
	int ret = 0;
//	ret = bsp_sensor_init();

	int count=0;

#if CONFIG_APP_HAS_PRESSURE_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_PRESS, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;
#endif

#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_AMBIENT_TEMP, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;
#endif

#if CONFIG_APP_HAS_ACCELEROMETER_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_ACCEL_XYZ, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;
#endif

#if CONFIG_APP_HAS_IMU_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_ACCEL_XYZ, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;

	ret = bsp_sensor_info_create(SENSOR_CHAN_GYRO_XYZ, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;

	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_GYRO_XYZ)) {
			setup_imu(sen->dev);
			break;
		}
	}
#endif

#if CONFIG_APP_HAS_HUMIDITY_SENSOR
	ret = bsp_sensor_info_create(SENSOR_CHAN_HUMIDITY, &m_sens_list, &count);
		if (ret != 0) return ret;
		m_sens_count += count;
#endif

#if CONFIG_APP_HAS_FLUID_LEVEL
	ret = bsp_sensor_info_create(SENSOR_CHAN_DISTANCE, &m_sens_list, &count);
		if (ret != 0) return ret;
		m_sens_count += count;
#endif

//	ret = app_sensor_info_get(&m_sens_list, &sens_count);
	return ret;
}
