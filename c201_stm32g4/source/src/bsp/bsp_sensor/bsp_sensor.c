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
LOG_MODULE_DECLARE(app_sensor);
#include <stdint.h>
#include <stdlib.h>

#include "acpu_c201_modules.h"
#include "app_sensor/bsp_sensor.h"

//const struct device *m_bmp388_test_dev = NULL;
//const struct device *m_bma253_dev = NULL;

static uint8_t m_sens_id=0;		/* unique sensor id */

/* static functions */
static inline int sinfo_list_add(sys_slist_t *list, struct sinfo *sinf) {
	sys_slist_append(list, &sinf->node);

	return 0;
}

static inline void sinfo_list_remove(sys_slist_t *list) {
	struct sinfo *sinf_data, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(list, sinf_data, tmp, node)
	{
		if (sinf_data) {
			sys_slist_remove(list, NULL, &sinf_data->node);
			free(sinf_data);
		}
	}
}

int bsp_sensor_info_create(enum sensor_channel chan, sys_slist_t *sinfo_list, int *sens_count) {
	int ret=0, count=0;
	const struct device *dev;
	struct sinfo * sen = NULL;
	switch (chan) {
	case SENSOR_CHAN_PRESS:
	{
#if 0
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_TEST);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_TEST, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
#endif
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_INT_POS);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_INT_POS, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}

		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_INT_NEG);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_INT_NEG, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}

		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_EXT_POS);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_EXT_POS, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}

		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_EXT_NEG);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_EXT_NEG, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203);
		if (dev) {
			/* S203 PRESS channel */
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_PRESS;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
	}
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
	{
//		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_TEST);
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_INT_POS);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
//			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_TEST, SENS_NAME_SZ);
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_INT_POS, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_INT_NEG);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_INT_NEG, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_EXT_POS);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_EXT_POS, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_EXT_NEG);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_PRESS_EXT_NEG, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203);
		if (dev) {
			/* S203 TEMP channel */
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
		
#if	(CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
		dev = DEVICE_DT_GET(DT_NODELABEL(ntc1));
		if (dev) {
			sen = (struct sinfo*) calloc(1, sizeof(struct sinfo));
			if (!sen)
				return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_NTC1_THERMISTOR, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}

		dev = DEVICE_DT_GET(DT_NODELABEL(ntc2));
		if (dev) {
			sen = (struct sinfo*) calloc(1, sizeof(struct sinfo));
			if (!sen)
				return -ENOMEM;

			sen->chan = SENSOR_CHAN_AMBIENT_TEMP;
			strncpy(sen->name, ACPU_NTC2_THERMISTOR, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
#endif
	}

		break;


	case SENSOR_CHAN_ACCEL_XYZ:
	{
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_TILT);
		if (dev) {
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_ACCEL_XYZ;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_TILT, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
	}
		break;

#if	(CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	case SENSOR_CHAN_HUMIDITY:
		{
			dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203);
			if (dev) {
				/* S203 HUMID channel */
				sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
				if (!sen)	return -ENOMEM;

				sen->chan = SENSOR_CHAN_HUMIDITY;
				strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_HUMIDITY_S203, SENS_NAME_SZ);
				sen->dev = dev;
				sen->id = ++m_sens_id;
				sen->status = true;
				sinfo_list_add(sinfo_list, sen);
				count++;
			}
		}
			break;
#endif

#if	(CONFIG_BOARD_E206 || CONFIG_BOARD_C208T)
	case SENSOR_CHAN_DISTANCE:
		{
			dev = device_get_binding(ACPU_FLUID_LEVEL);
			//dev = DEVICE_DT_GET(DT_NODELABEL(lidar));
			if (dev) {
				/* FLUID_LEVEL channel */
				sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
				if (!sen)	return -ENOMEM;

				sen->chan = SENSOR_CHAN_DISTANCE;
				strncpy(sen->name, ACPU_FLUID_LEVEL, SENS_NAME_SZ);
				sen->dev = dev;
				sen->id = ++m_sens_id;
				sen->status = true;
				sinfo_list_add(sinfo_list, sen);
				count++;
			}
		}
			break;
#endif

	case SENSOR_CHAN_GYRO_XYZ:
	{
		dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_IMU_1);
		if (dev) {
			/* IMU's ACCEL channel */
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_ACCEL_XYZ;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_IMU_1, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;

			/* IMU's GYRO channel */
			sen = (struct sinfo *) calloc(1, sizeof (struct sinfo));
			if (!sen)	return -ENOMEM;

			sen->chan = SENSOR_CHAN_GYRO_XYZ;
			strncpy(sen->name, ACPU_C201_MOD_NAME_SENS_IMU_1, SENS_NAME_SZ);
			sen->dev = dev;
			sen->id = ++m_sens_id;
			sen->status = true;

			sinfo_list_add(sinfo_list, sen);
			count++;
		}
	}
		break;

	default:
		break;
	}

	*sens_count = count;
	return ret;
}

int bsp_sensor_info_destroy(sys_slist_t *sinfo_list) {
	sinfo_list_remove(sinfo_list);
	return 0;
}

int bsp_sensor_value_get(const struct device *dev, uint8_t chan, struct sensor_value *val) {
	if (!dev) {
		return -1;
	}

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, chan, val);

	return 0;
}


int bsp_sensor_pressure_kpa_get(const struct device *dev, struct sensor_value *press_kpa) {
	if (!dev) {
		return -1;
	}

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, press_kpa);

	return 0;
}

int bsp_sensor_temp_c_get(const struct device *dev, struct sensor_value *temp_c) {
	if (!dev) {
		return -1;
	}

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, temp_c);

	return 0;
}

int bsp_sensor_humid_get(const struct device *dev, struct sensor_value *humid) {
	if (!dev) {
		return -1;
	}

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, humid);
	return 0;
}

int bsp_sensor_distance_get(const struct device *dev, struct sensor_value *distance) {
	if (!dev) {
		return -1;
	}

	sensor_sample_fetch(dev);
	sensor_channel_get(dev, SENSOR_CHAN_DISTANCE, distance);
	return 0;
}

int bsp_sensor_3a_accel_get(const struct device *dev, struct sensor_value *accel) {
	if (!dev) {
		return -1;
	}

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Accel sample fetch error");
		return -1;;
	}
	if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &accel[0]) < 0) {
		LOG_ERR("Accel Channel x get error");
		return -1;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &accel[1]) < 0) {
		LOG_ERR("Accel Channel y get error");
		return -1;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &accel[2]) < 0) {
		LOG_ERR("Accel Channel z get error");
		return -1;
	}
	return 0;
}

int bsp_sensor_gyro_get(const struct device *dev, struct sensor_value *gyro) {
	if (!dev) {
		return -1;
	}

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Gyro sample fetch error");
		return -1;;
	}
	if (sensor_channel_get(dev, SENSOR_CHAN_GYRO_X, &gyro[0]) < 0) {
		LOG_ERR("Gyro Channel x get error");
		return -1;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_GYRO_Y, &gyro[1]) < 0) {
		LOG_ERR("Gyro Channel y get error");
		return -1;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_GYRO_Z, &gyro[2]) < 0) {
		LOG_ERR("Gyro Channel z get error");
		return -1;
	}
	return 0;
}

int bsp_sensor_init() {
//	m_bmp388_test_dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_PRESS_TEST);
//	if (m_bmp388_test_dev == NULL) {
//		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_SENS_PRESS_TEST);
//		return -1;
//	}
//	m_bma253_dev = device_get_binding(ACPU_C201_MOD_NAME_SENS_TILT);
//	if (m_bma253_dev == NULL) {
//		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_SENS_TILT);
//		return -1;
//	}
	return 0;
}
