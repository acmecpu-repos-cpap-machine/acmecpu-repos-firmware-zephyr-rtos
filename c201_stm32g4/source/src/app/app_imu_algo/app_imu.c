/*
 * app_imu.c
 *
 *  Created on: 17-Jul-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <ctype.h>
LOG_MODULE_REGISTER(app_imu_algo);

#include "app_imu_algo/app_imu_algo.h"
#include "app_imu_algo/bmi270_legacy.h"
#include "app_imu_algo/common.h"
#include "app_imu_algo/bmi270.h"

/**\name Orientation output macros */
#define BMI270_LEGACY_FACE_UP            UINT8_C(0x00)
#define BMI270_LEGACY_FACE_DOWN          UINT8_C(0x01)

#define BMI270_LEGACY_PORTRAIT_UP_RIGHT  UINT8_C(0x00)
#define BMI270_LEGACY_LANDSCAPE_LEFT     UINT8_C(0x01)
#define BMI270_LEGACY_PORTRAIT_UP_DOWN   UINT8_C(0x02)
#define BMI270_LEGACY_LANDSCAPE_RIGHT    UINT8_C(0x03)

/*Function to set the */
static int8_t set_feature_config_any_motion(struct bmi2_dev *bmi2_dev) {
	/* Status of api are returned to this variable. */
	int8_t rslt;

	/* Structure to define the type of sensor and its configurations. */
	struct bmi2_sens_config config;

	/* Configure the type of any-motion feature. */
	config.type = BMI2_ANY_MOTION;

	/* Get default configurations for the type of feature selected. */
	rslt = bmi270_legacy_get_sensor_config(&config, 1, bmi2_dev);
	bmi2_error_codes_print_result(rslt);

	if (rslt == BMI2_OK) {
		/* NOTE: The user can change the following configuration parameters according to their requirement. */
		/* 1LSB equals 20ms. Default is 100ms, setting to 80ms. */
		config.cfg.any_motion.duration = 0x04;

		/* 1LSB equals to 0.48mg. Default is 83mg, setting to 50mg. */
		config.cfg.any_motion.threshold = 0x68;

		/* Set new configurations. */
		rslt = bmi270_legacy_set_sensor_config(&config, 1, bmi2_dev);
		bmi2_error_codes_print_result(rslt);
	}

	return rslt;
}

/*!
 * @brief This internal API is used to set configurations for step counter.
 */
static int8_t set_feature_config_step_count(struct bmi2_dev *bmi2_dev) {
	/* Status of api are returned to this variable. */
	int8_t rslt;

	/* Structure to define the type of sensor and its configurations. */
	struct bmi2_sens_config config;

	/* Configure the type of sensor. */
	config.type = BMI2_STEP_COUNTER;

	/* Get default configurations for the type of feature selected. */
	rslt = bmi270_legacy_get_sensor_config(&config, 1, bmi2_dev);
	bmi2_error_codes_print_result(rslt);

	if (rslt == BMI2_OK) {
		/* Setting water-mark level to 1 for step counter to get interrupt after 20 step counts. Every 20 steps once
		 * output triggers. */
		config.cfg.step_counter.watermark_level = 1;

		LOG_INF("Step counter watermark level set to 1 (20 steps)\n");

		/* Set new configuration. */
		rslt = bmi270_legacy_set_sensor_config(&config, 1, bmi2_dev);
		bmi2_error_codes_print_result(rslt);
	}

	return rslt;
}

/*Fetches the readings from the BMI270 IMU sensor and with the help of the interrupts
 * generated it detects upon the events have taken place*/

/*Detects if the device has occurred any motion*/
/*Any motion detection*/
int app_imu_algo(BMI_DETECTION bmi_algo) {

	int ret = 0;

	/* Sensor initialization configuration. */
	struct bmi2_dev bmi;
	int8_t rslt;

	/* Interface reference is given as a parameter
	 * For I2C : BMI2_I2C_INTF
	 * For SPI : BMI2_SPI_INTF
	 */
	rslt = bmi2_interface_init(&bmi, BMI2_I2C_INTF);
	bmi2_error_codes_print_result(rslt);

	/* Initialize bmi270_legacy. */
	rslt = bmi270_legacy_init(&bmi);
	bmi2_error_codes_print_result(rslt);

	switch (bmi_algo) {

	case ANY_MOTION: {
		struct bmi2_sens_int_config sens_int = { .type = BMI2_ANY_MOTION,
				.hw_int_pin = BMI2_INT1 };
		uint8_t sens_list[2] = { BMI2_ACCEL, BMI2_ANY_MOTION };
		uint16_t int_status = 0;
		//int index = 0;

		/* Enable the selected sensors. */
		rslt = bmi270_legacy_sensor_enable(sens_list, 2, &bmi);
		bmi2_error_codes_print_result(rslt);

		/* Set feature configurations for any-motion. */
		rslt = set_feature_config_any_motion(&bmi);
		bmi2_error_codes_print_result(rslt);

		/* Map the feature interrupt for any-motion. */
		rslt = bmi270_legacy_map_feat_int(&sens_int, 1, &bmi);
		bmi2_error_codes_print_result(rslt);

		/* Clear the buffer. */
		int_status = 0;

		/* To get the interrupt status of any-motion. */
		rslt = bmi2_get_int_status(&int_status, &bmi);
		bmi2_error_codes_print_result(rslt);

		/* To check the interrupt status of any-motion. */
		if (int_status & BMI270_LEGACY_ANY_MOT_STATUS_MASK) {
			LOG_INF("Any-motion interrupt is generated");
		}
	}
		break;

	case G_DETECTION: {

		/* Structure to define the type of sensor and its configurations. */
		struct bmi2_sens_config config[2];

		/* Accel sensor and high-g feature are listed in array. */
		//uint8_t high_g_sens_list[2] = { BMI2_ACCEL, BMI2_HIGH_G };
		/* Accel sensor and low-g feature are listed in array. */
		//uint8_t low_g_sens_list[2] = { BMI2_ACCEL, BMI2_LOW_G };
		/* Structure to define type of sensor and their respective data. */
		//struct bmi2_feat_sensor_data sensor_data = { 0 };
		/* Variable to get high-g and low-g interrupt status. */
		uint16_t int_status = 0;

		/* Variables to store the output of high-g. */
		//uint8_t high_g_out = 0;
		/* Select features and their pins to be mapped to. */
		//struct bmi2_sens_int_config high_g_int = { .type = BMI2_HIGH_G, .hw_int_pin = BMI2_INT1 };
		struct bmi2_sens_int_config low_g_int = { .type = BMI2_LOW_G,
				.hw_int_pin = BMI2_INT2 };

		if (rslt == BMI2_OK) {
			/* Enable high-g feature. */
			//rslt = bmi270_legacy_sensor_enable(high_g_sens_list, 2, &bmi);
			//bmi2_error_codes_print_result(rslt);
			/* Configure the type of feature. */
			config[0].type = BMI2_HIGH_G;
			config[1].type = BMI2_LOW_G;

			if (rslt == BMI2_OK) {

				/* Get default configurations for the type of feature selected. */
				rslt = bmi270_legacy_get_sensor_config(config, 2, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* Disable high-g feature. */
				//rslt = bmi270_legacy_sensor_disable(high_g_sens_list, 2, &bmi);
				//bmi2_error_codes_print_result(rslt);
				/* Enable low-g feature. */
				//rslt = bmi270_legacy_sensor_enable(low_g_sens_list, 2, &bmi);
				//bmi2_error_codes_print_result(rslt);
				/* Map low_g feature interrupt to interrupt pin. */
				rslt = bmi270_legacy_map_feat_int(&low_g_int, 1, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* To generate low-g interrupt */
				//LOG_INF("\n\nDrop the board in free fall\n");

				/* Clear the buffer. */
				int_status = 0;

				/* To get the interrupt status of low-g. */
				rslt = bmi2_get_int_status(&int_status, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* To check the interrupt status of low-g. */
				if (int_status & BMI270_LEGACY_LOW_G_STATUS_MASK) {
					LOG_INF("Low-g interrupt is generated\n");
					break;
				}
			} else
				LOG_INF("G_DETECTION Error detected");
		} else
			LOG_INF("G_DETECTION Error detected");
	}
		break;

	case ORIENT_DETECTION: {

		/* Structure to define the type of sensor and its configurations. */
		struct bmi2_sens_config config;

		/* Variables to store the output of orientation. */
		uint8_t orientation_out = 0;
		uint8_t orientation_faceup_down = 0;

		/* Structure to define type of sensor and their respective data. */
		struct bmi2_feat_sensor_data sensor_data = { 0 };

		/* Accel sensor and orientation feature are listed in array. */
		uint8_t sens_list[2] = { BMI2_ACCEL, BMI2_ORIENTATION };

		/* Variable to get orientation interrupt status. */
		uint16_t int_status = 0;

		/* Select features and their pins to be mapped to. */
		struct bmi2_sens_int_config sens_int = { .type = BMI2_ORIENTATION,
				.hw_int_pin = BMI2_INT2 };

		if (rslt == BMI2_OK) {
			/* Enable the selected sensors. */
			rslt = bmi270_legacy_sensor_enable(sens_list, 2, &bmi);
			bmi2_error_codes_print_result(rslt);

			/* Configure the type of feature. */
			config.type = BMI2_ORIENTATION;

			if (rslt == BMI2_OK) {
				/* Get default configurations for the type of feature selected. */
				rslt = bmi270_legacy_get_sensor_config(&config, 1, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* Set orientation configurations */
				config.cfg.orientation.ud_en = 0x01;
				config.cfg.orientation.mode = 0x02;
				config.cfg.orientation.blocking = 0x01;
				config.cfg.orientation.theta = 0x33;
				config.cfg.orientation.hysteresis = 0x80;

				rslt = bmi270_legacy_set_sensor_config(&config, 1, &bmi);
				bmi2_error_codes_print_result(rslt);

				if (rslt == BMI2_OK) {
					/* Map the feature interrupt for orientation. */
					rslt = bmi270_legacy_map_feat_int(&sens_int, 1, &bmi);
					bmi2_error_codes_print_result(rslt);

					/* Sensor type of sensor to get data */
					sensor_data.type = BMI2_ORIENTATION;

					/* To get the interrupt status of orientation. */
					rslt = bmi2_get_int_status(&int_status, &bmi);
					bmi2_error_codes_print_result(rslt);

					/* To check the interrupt status of orientation. */
					if ((rslt == BMI2_OK)
							&& (int_status & BMI270_LEGACY_ORIENT_STATUS_MASK)) {
						LOG_INF("Orientation interrupt is generated\n");
						rslt = bmi270_legacy_get_feature_data(&sensor_data, 1,
								&bmi);

						orientation_out =
								sensor_data.sens_data.orient_output.portrait_landscape;
						orientation_faceup_down =
								sensor_data.sens_data.orient_output.faceup_down;

						LOG_INF("The Orientation output is %d \n",
								orientation_out);
						LOG_INF("The Orientation faceup/down output is %d\n",
								orientation_faceup_down);

						switch (orientation_out) {
						case BMI270_LEGACY_LANDSCAPE_LEFT:
							LOG_INF("Orientation state is landscape left\n");
							break;
						case BMI270_LEGACY_LANDSCAPE_RIGHT:
							LOG_INF("Orientation state is landscape right\n");
							break;
						case BMI270_LEGACY_PORTRAIT_UP_DOWN:
							LOG_INF(
									"Orientation state is portrait upside down\n");
							break;
						case BMI270_LEGACY_PORTRAIT_UP_RIGHT:
							LOG_INF("Orientation state is portrait upright\n");
							break;
						default:
							break;
						}

						switch (orientation_faceup_down) {
						case BMI270_LEGACY_FACE_UP:
							LOG_INF("Orientation state is face up\n");
							break;
						case BMI270_LEGACY_FACE_DOWN:
							LOG_INF("Orientation state is face down\n");
							break;
						default:
							break;
						}

						break;
					}
				}
			}
		}
	}
		break;

	case STEP_COUNT: {

		/* Structure to define type of sensor and their respective data. */
		struct bmi2_feat_sensor_data sensor_data = { 0 };

		/* Status of api are returned to this variable. */
		int8_t rslt;

		/* Accel sensor and step counter feature are listed in array. */
		uint8_t sensor_sel[2] = { BMI2_ACCEL, BMI2_STEP_COUNTER };

		/* Variable to get step counter interrupt status. */
		uint16_t int_status = 0;

		/* Select features and their pins to be mapped to. */
		struct bmi2_sens_int_config sens_int = { .type = BMI2_STEP_COUNTER,
				.hw_int_pin = BMI2_INT2 };

		/* Type of sensor to get step counter data. */
		sensor_data.type = BMI2_STEP_COUNTER;

		/* Enable the selected sensor. */
		rslt = bmi270_legacy_sensor_enable(sensor_sel, 2, &bmi);
		bmi2_error_codes_print_result(rslt);

		if (rslt == BMI2_OK) {
			/* Set the feature configuration for step counter. */
			rslt = set_feature_config_step_count(&bmi);
			bmi2_error_codes_print_result(rslt);

			if (rslt == BMI2_OK) {
				/* Map the step counter feature interrupt. */
				rslt = bmi270_legacy_map_feat_int(&sens_int, 1, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* Loop to get number of steps counted. */
				/*TODO Test this if it runs fine with the main loop*/
				//	do {
				/* To get the interrupt status of the step counter. */
				rslt = bmi2_get_int_status(&int_status, &bmi);
				bmi2_error_codes_print_result(rslt);

				/* To check the interrupt status of the step counter. */
				if (int_status & BMI270_LEGACY_STEP_CNT_STATUS_MASK) {
					LOG_INF(
							"Step counter interrupt occurred when watermark level (20 steps) is reached\n");

					/* Get step counter output. */
					rslt = bmi270_legacy_get_feature_data(&sensor_data, 1,
							&bmi);
					bmi2_error_codes_print_result(rslt);

					/* Print the step counter output. */
					LOG_INF("No of steps counted  = %lu\n",
							(unsigned long int) sensor_data.sens_data.step_counter_output);

					break;
				}
//				} while (rslt == BMI2_OK);
			}
		}
	}
		break;

	default:
		break;
	}

	return ret;
}
