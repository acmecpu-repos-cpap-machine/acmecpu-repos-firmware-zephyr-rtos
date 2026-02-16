/*
 * Copyright (c) 2021 Acme CPU
 *
 *  Created on: 14-Dec-2021
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_sensor);
#include <zephyr/drivers/sensor.h>
#include <stdint.h>
#include <stdlib.h>

#include "app_sensor/app_sensor.h"
#include "app_sensor/bsp_sensor.h"
#if (CONFIG_APP_HAS_PPG_SENSOR && CONFIG_SHMAX30101)
	#include "shmax30101.h"
#endif
#if (CONFIG_APP_HAS_EKG_SENSOR && CONFIG_MAX30001)
	#include "max30001.h"
#endif

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
	APP_SENSOR_TYPE_MAX,
};

//void app_sensor_info_delete(sys_slist_t *sinfo_list) {
//	bsp_sensor_info_destroy(sinfo_list);
//}

//****************************************************************************
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

	return ret;
#else
	*sens_count = m_sens_count;
	return &m_sens_list;
#endif
}

int app_sensor_chan_to_id(int chan, uint8_t *psen_id, int *pcount)
{
	int ret=0, i=0;
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (chan == sen->chan) {
			*(psen_id + i++) = sen->id;
		}
	}
	if (i == 0) {
		ret = -1;
	}
	*pcount = i;
	return ret;
}

int app_sensor_id_to_chan(uint8_t id, int *pchan)
{
	int ret=0, match=0;
	struct sinfo *sen, *tmp;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if (id == sen->id) {
			*pchan = sen->chan;
			match = 1;
			break;
		}
	}
	if (!match) {
		ret = -1;
	}
	return ret;
}

//****************************************************************************
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

//****************************************************************************
#if CONFIG_APP_HAS_TEMPERATURE_SENSOR
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

//****************************************************************************
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

//****************************************************************************
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

	LOG_INF("Device %p name is %s", dev, dev->name);

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

	printk("\n");
	printk("AX: %d.%06d; AY: %d.%06d; AZ: %d.%06d; "
	       "GX: %d.%06d; GY: %d.%06d; GZ: %d.%06d;\n",
	       acc[0].val1, acc[0].val2,
	       acc[1].val1, acc[1].val2,
	       acc[2].val1, acc[2].val2,
	       gyr[0].val1, gyr[0].val2,
	       gyr[1].val1, gyr[1].val2,
	       gyr[2].val1, gyr[2].val2);

}
#endif	/* CONFIG_APP_HAS_IMU_SENSOR */

//****************************************************************************
#if (CONFIG_APP_HAS_PPG_SENSOR)
#define PPG_SENSOR_MAX	1
#define HR_MIN	0
#define HR_MAX	200
#define HR_RANGE_VALID(x)	((x >= HR_MIN) && (x <= HR_MAX))
/* ppg thread variables */
K_THREAD_STACK_DEFINE(m_ppg_stack, 2048);
static struct k_thread m_ppg_data;
static k_tid_t m_ppg_tid;
#define PPG_THREAD_PRIO		10
struct ppg_ctrl {
	uint8_t id;
	bool run;
};
static struct ppg_ctrl m_ppg[PPG_SENSOR_MAX];
static int8_t m_ppg_idx = -1;

static void ppg_thread(void *p1, void *p2, void *p3)
{
	struct ppg_ctrl *ppg = (struct ppg_ctrl*)p1;
	int ret=0;
	struct sensor_value val[3];
	float hr=0, spo2=0;
	uint8_t status=0, hr_conf=0;

	app_sensor_ppg_init();
	while (ppg->run) {
		/* get hr and spo2 */
		ret = app_sensor_value_get(ppg->id, val);
		if (!ret) {
			status = val[2].val1;
			if (status == 3) {			/* finger detected */
				hr_conf = val[0].val2;//*0.392157;
				hr = val[0].val1/10;
				spo2 = val[1].val1/10;	// TODO add a validation range
				if (HR_RANGE_VALID(hr))
					LOG_INF("%0.1f,%d,%0.1f,%d", hr, hr_conf, spo2, status);
			} else if (status == 0) {	/* no object detected */
				// LOG_ERR("nod");
			}
		} else {
			LOG_ERR("error getting sensor data!");
			// TODO reset the sensor
		}
		k_sleep(K_MSEC(100));
	}
	/* TODO disable sensor */
	LOG_INF("Stopping PPG sensor id %d", ppg->id);
	app_sensor_ppg_deinit();
	m_ppg_idx--;
}
int app_sensor_ppg_get_start(uint8_t ppg_id)
{
	int ret = 0;
	if (++m_ppg_idx > (PPG_SENSOR_MAX-1)) {
		LOG_ERR("Cannot start ppg read, max %d PPG sensors supported, curently running %d PPG sensors", PPG_SENSOR_MAX, (m_ppg_idx));
		return -1;
	}
	m_ppg[m_ppg_idx].id = ppg_id;
	m_ppg[m_ppg_idx].run = true;
	m_ppg_tid = k_thread_create(&m_ppg_data,
			m_ppg_stack,
			K_THREAD_STACK_SIZEOF(m_ppg_stack),
			ppg_thread, &m_ppg[m_ppg_idx], NULL, NULL, PPG_THREAD_PRIO,
			0, K_NO_WAIT);
	ret = k_thread_name_set(m_ppg_tid, "ppg");
	return ret;
}

int app_sensor_ppg_get_stop(uint8_t ppg_id)
{
	int i, ret = 0;
	for (i=0; i<PPG_SENSOR_MAX; i++) {
		if (m_ppg[i].id == ppg_id) {
			m_ppg[i].run = false;
			break;
		}
	}
	return ret;
}

/* Ref: Maxim AN6921 - Measuring Blood Pressure, Heart Rate, and SpO2 Using MAX32664D*/
static void setup_ppg_bpt(const struct device *const dev)
{
	struct sensor_value val;

	/* 1. load 824 bytes of calibration vector data */

	/* 2. Set if the user is on BP medication */
	val.val1 = 0;
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SET_CFG_BPT_MED, &val);

	/* 3. Set user in Reset mode */
	val.val1 = 0;
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SET_CFG_BPT_NONREST, &val);

	/* 4. Set data and time as two 32-bit numbers for YYMMDD and HHMMSS in little-endian format. */
	struct sensor_value datetime[2];
	datetime[0].val1 = 0xC55F0300;
	datetime[1].val2 = 0x80E71000;
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SET_CFG_BPT_DATE, datetime);

	/* 5. Set SpO2 calibration coefficients */
	struct sensor_value spo2_cal_coef[3];
	spo2_cal_coef[0].val1 = 0x00026F60;
	spo2_cal_coef[1].val1 = 0xFFCB1D12;
	spo2_cal_coef[2].val1 = 0x00ABF37B;
	spo2_cal_coef[3].val1 = 0x04;	// bpt algo idx
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SET_CFG_SPO2_CAL, spo2_cal_coef);

	/* now enable the AFE and Algo and start read */
	val.val1 = 0;
#if (CONFIG_BOARD_H205C_NRF5340_CPUAPP || CONFIG_BOARD_H205C_NRF5340_CPUAPP_NS)
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_READ_BPT_1, &val);
#elif (CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_READ_PPG_0, &val);
#endif
}
static void setup_ppg_whrm(const struct device *const dev)
{
	struct sensor_value spo2_cal_coef[4];
	spo2_cal_coef[0].val1 = 0x00026F60;
	spo2_cal_coef[1].val1 = 0xFFCB1D12;
	spo2_cal_coef[2].val1 = 0x00ABF37B;
	spo2_cal_coef[3].val1 = 0x02;	// whrm algo idx
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_SET_CFG_SPO2_CAL, spo2_cal_coef);

	struct sensor_value val;
	val.val1 = 0;
	sensor_attr_set(dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_READ_PPG_0, &val);
}
int app_sensor_ppg_init()
{
	int ret=0, match=0;
	struct sinfo *sen, *tmp;
	int channel = -1;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		channel = sen->chan;	// done to get rid of compiler warnings
		if ((channel == SENSOR_CHAN_HR_AND_SPO2)) {
			// setup_ppg_bpt(sen->dev);
			setup_ppg_whrm(sen->dev);
			match=1;
			break;
		}
	}
	if (!match) {
		ret = -1;
	}

	return ret;
}
int app_sensor_ppg_deinit()
{
	int ret=0, match=0;
	struct sinfo *sen, *tmp;
	int channel = -1;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		channel = sen->chan;	// done to get rid of compiler warnings
		if ((channel == SENSOR_CHAN_HR_AND_SPO2)) {
			struct sensor_value val;
			val.val1 = 0;
			sensor_attr_set(sen->dev, SENSOR_CHAN_SPO2, SENSOR_ATTR_STOP_PPG_0, &val);
			match=1;
			break;
		}
	}
	if (!match) {
		ret = -1;
	}

	return ret;
}
#endif	/*CONFIG_APP_HAS_PPG_SENSOR*/

//****************************************************************************
#if CONFIG_APP_HAS_EKG_SENSOR
struct ekg_ctrl {
	uint8_t id;
	bool run;
};
struct ekg_data {
	uint32_t *data;
	uint32_t len;	// number of uint32_t words
};
static struct ekg_ctrl m_ekg = {0, false};
K_THREAD_STACK_DEFINE(m_ekg_stack, 1024);
static struct k_thread m_ekg_data;
static k_tid_t m_ekg_tid;
#define EKG_THREAD_PRIO		10
K_FIFO_DEFINE(m_ekg_fifo);

static void ekg_thread(void *p1, void *p2, void *p3)
{
	struct ekg_ctrl *ekg = (struct ekg_ctrl*)p1;
	int ret=0;
	struct ekg_data *edata = NULL;

	ret = app_sensor_ekg_init();
	if (ret < 0) {
		LOG_ERR("EKG init failed!");
		return;
	}
	while (ekg->run) {
		// get EKG data from FIFO and process
		edata = k_fifo_get(&m_ekg_fifo, K_FOREVER);
		if (edata == NULL) {
			continue;
		}

		// print (consume) the data
		for (int i=0; i<edata->len; i++)	{
			LOG_PRINTK("%d ", edata->data[i]);
		}

		// free the data
		free(edata->data);
		free(edata);

		// get RTOR data if enabled

		// sleep
		// k_sleep(K_MSEC(100));
	}

	/* TODO disable sensor */
	LOG_INF("Stopping EKG sensor id %d", ekg->id);
}
int app_sensor_ekg_start()
{
	int ret = 0;
	uint8_t ekg_id;
	int temp;
	app_sensor_chan_to_id(SENSOR_CHAN_MAX30001_EKG_RTOR, &ekg_id, &temp);

	if (m_ekg.run)	{
		LOG_ERR("EKG thread already running, aborting");
		return -1;
	}
	m_ekg.id = ekg_id;
	m_ekg.run = true;
	m_ekg_tid = k_thread_create(&m_ekg_data,
			m_ekg_stack,
			K_THREAD_STACK_SIZEOF(m_ekg_stack),
			ekg_thread, &m_ekg, NULL, NULL, EKG_THREAD_PRIO,
			0, K_NO_WAIT);
	ret = k_thread_name_set(m_ekg_tid, "ekg");
	return ret;
}
int app_sensor_ekg_stop()
{
	m_ekg.run = false;
	return 0;
}
void stream_packet_uint32_ecg(uint32_t id, uint32_t *buffer, uint32_t number)
{
    if (id == MAX30001_DATA_ECG) {
		struct ekg_data *edata = (struct ekg_data *)calloc(1, sizeof(struct ekg_data));
		edata->data = (uint32_t*) calloc(1, number);
		if (edata->data == NULL) {
			LOG_ERR("edata->data calloc failed!");
			return;
		}
      	// for (int i = 0; i < number; i++) {
        // 	ecg_buffer[i] = buffer[i];
        // }
		memcpy(edata->data, buffer, number * sizeof(uint32_t));
		edata->len = number;

		// fifo put
		k_fifo_put(&m_ekg_fifo, edata);
	}

   if (id == MAX30001_DATA_BIOZ) {
         /// Add code for reading BIOZ data
   }
   if (id == MAX30001_DATA_PACE) {
         ///  Add code for reading Pace data
   }
   if (id == MAX30001_DATA_RTOR) {
         /// Add code for reading RtoR data
   }
}

/// Initialization values for ECG_InitStart()
#define EN_ECG 0b1
#define OPENP 0b0
#define OPENN 0b0
#define POL 0b0
#define CALP_SEL 0b0
#define CALN_SEL 0b0
#define E_FIT 0xf
#define RATE 0b10
#define GAIN 0b00
#define DHPF 0b01
#define DLPF 0b01
/// Initialization values for CAL_InitStart()
#define EN_VCAL 0b1
#define VMODE 0b1
#define VMAG 0b1
#define FCAL 0b011
#define THIGH 0x7FF
#define FIFTY 0b0
/// Initializaton values for Rbias_FMSTR_Init()
#define EN_RBIAS 0b01
#define RBIASV 0b10
#define RBIASP 0b1
#define RBIASN 0b1
#define FMSTR 0b00
static void setup_ekg_max30001(const struct device *const dev)
{	
	int ret = 0;
	struct sensor_value val;

	/* Do a software reset of max30001 */
	val.val1 = 0; val.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_MAX30001_SW_RESET, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_MAX30001_SW_RESET failed %d", ret);
		return;
	}

	/* CNFG_EMUX */
	val.val1 = CNFG_EMUX;	// addr
	val.val2 = 0x0;			// val
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_CONFIGURATION, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_CONFIGURATION CNFG_EMUX failed %d", ret);
		return;
	}

	/* CNFG_GEN */
	val.val1 = CNFG_GEN;	// addr
	val.val2 = 0x80004;		// val
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_CONFIGURATION, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_CONFIGURATION CNFG_GEN failed %d", ret);
		return;
	}

	/* EN_INT */
	val.val1 = EN_INT;	// addr
	val.val2 = 3;		// val
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_CONFIGURATION, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_CONFIGURATION EN_INT failed %d", ret);
		return;
	}

	/* EN_INT2 */
	val.val1 = EN_INT2;	// addr
	val.val2 = 3;		// val
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_CONFIGURATION, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_CONFIGURATION EN_INT2 failed %d", ret);
		return;
	}

	/* ECG Initialization */
	{
		struct sensor_value ecg_init[11];
		ecg_init[0].val1 = EN_ECG;		// en_enint_loc
		ecg_init[1].val1 = OPENP;		// en_eovf_loc
		ecg_init[2].val1 = OPENN;		// en_fstint_loc
		ecg_init[3].val1 = POL;			// en_dcloffint_loc
		ecg_init[4].val1 = CALP_SEL;	// en_bint_loc
		ecg_init[5].val1 = CALN_SEL;	// en_bovf_loc
		ecg_init[6].val1 = E_FIT;		// en_bover_loc
		ecg_init[7].val1 = RATE;		// en_bundr_loc
		ecg_init[8].val1 = GAIN;		// en_bcgmon_loc
		ecg_init[9].val1 = DHPF;		// en_pint_loc
		ecg_init[10].val1 = DLPF;		// en_povf_loc

		ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_MAX30001_ECG_INIT_START, ecg_init);
		if (ret) {
			LOG_ERR("sensor_attr_set SENSOR_ATTR_MAX30001_ECG_INIT_START failed %d", ret);
			return;
		}
	}

	/* RtoR Initialization */

	/* assigns interrupts */
	{
		struct sensor_value int_assign[17];
		int_assign[0].val1 = MAX30001_INT_B;	// en_enint_loc
		int_assign[1].val1 = MAX30001_NO_INT;	// en_eovf_loc
		int_assign[2].val1 = MAX30001_NO_INT;	// en_fstint_loc

		int_assign[3].val1 = MAX30001_INT_2B;	// en_dcloffint_loc
		int_assign[4].val1 = MAX30001_INT_B;	// en_bint_loc
		int_assign[5].val1 = MAX30001_NO_INT;	// en_bovf_loc
		
		int_assign[6].val1 = MAX30001_INT_2B;	// en_bover_loc
		int_assign[7].val1 = MAX30001_INT_2B;	// en_bundr_loc
		int_assign[8].val1 = MAX30001_NO_INT;	// en_bcgmon_loc

		int_assign[9].val1 = MAX30001_INT_B;	// en_pint_loc
		int_assign[10].val1 = MAX30001_NO_INT;	// en_povf_loc
		int_assign[11].val1 = MAX30001_NO_INT;	// en_pedge_loc

		int_assign[12].val1 = MAX30001_INT_2B;	// en_lonint_loc
		int_assign[13].val1 = MAX30001_INT_B;	// en_rrint_loc
		int_assign[14].val1 = MAX30001_NO_INT;	// en_samp_loc

		int_assign[15].val1 = MAX30001_INT_ODNR;	// intb_Type
		int_assign[16].val1 = MAX30001_INT_ODNR;	// int2b_Type

		ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_MAX30001_INT_ASSIGN, int_assign);
		if (ret) {
			LOG_ERR("sensor_attr_set SENSOR_ATTR_MAX30001_INT_ASSIGN failed %d", ret);
			return;
		}
	}

	/* add callback to get data from driver */
	val.val1 = (int32_t)stream_packet_uint32_ecg;
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_MAX30001_CALLBACK, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_MAX30001_CALLBACK failed %d", ret);
		return;
	}

	/* synch */
	val.val1 = 0; val.val2 = 0;
	ret = sensor_attr_set(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_MAX30001_SYNCH, &val);
	if (ret) {
		LOG_ERR("sensor_attr_set SENSOR_ATTR_MAX30001_SYNCH failed %d", ret);
		return;
	}

	/* read status */
	val.val1 = STATUS;	// addr
	val.val2 = 0;		// return data
	ret = sensor_attr_get(dev, SENSOR_CHAN_MAX30001_EKG_RTOR, SENSOR_ATTR_CONFIGURATION, &val);	
	if (ret) {
		LOG_ERR("sensor_attr_get SENSOR_ATTR_CONFIGURATION STATUS failed %d", ret);
		return;
	}
}
int app_sensor_ekg_init()
{
	int ret=0, match=0;
	struct sinfo *sen, *tmp;
	int channel = -1;
	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		channel = sen->chan;	// done to get rid of compiler warnings
		if ((channel == SENSOR_CHAN_MAX30001_EKG_RTOR)) {
			setup_ekg_max30001(sen->dev);
			match=1;
			break;
		}
	}
	if (!match) {
		ret = -1;
	}
	return ret;
}
#endif /*CONFIG_APP_HAS_EKG_SENSOR*/

//****************************************************************************
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
			break;
		}
	}

	if (!match) {
		ret = -1;
	}
	return ret;
}


int app_sensor_init() {
	int ret = 0;
	struct sinfo *sen, *tmp;
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

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	{
		if ((sen->chan == SENSOR_CHAN_GYRO_XYZ)) {
			setup_imu(sen->dev);
			// break;
		}
	}
#endif

#if (CONFIG_APP_HAS_PPG_SENSOR)
	// ret = bsp_sensor_info_create(SENSOR_CHAN_HEART_RATE, &m_sens_list, &count);
	// if (ret != 0) return ret;
	// m_sens_count += count;

	// ret = bsp_sensor_info_create(SENSOR_CHAN_SPO2, &m_sens_list, &count);
	// if (ret != 0) return ret;
	// m_sens_count += count;

	ret = bsp_sensor_info_create(SENSOR_CHAN_HR_AND_SPO2, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;

	// int channel = -1;
	// SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&m_sens_list, sen, tmp, node)
	// {
	// 	channel = sen->chan;	// done to get rid of compiler warnings
	// 	if ((channel == SENSOR_CHAN_HR_AND_SPO2)) {
	// 		// setup_ppg_bpt(sen->dev);
	// 		setup_ppg_whrm(sen->dev);
	// 		break;
	// 	}
	// }
#endif	/*(CONFIG_APP_HAS_PPG_SENSOR)*/

#if (CONFIG_APP_HAS_EKG_SENSOR)
	ret = bsp_sensor_info_create(SENSOR_CHAN_MAX30001_EKG_RTOR, &m_sens_list, &count);
	if (ret != 0) return ret;
	m_sens_count += count;
#endif	/*(CONFIG_APP_HAS_EKG_SENSOR)*/

//	ret = app_sensor_info_get(&m_sens_list, &sens_count);
	return ret;
}
