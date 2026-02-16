/*
 * app_heater.c
 *
 *  Created on: 01-Mar-2024
 *      Author: Shubham Keshari (shubhamk@acmecpu.com)
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_heater);

#define VMEASURE_EN_PIN		DT_GPIO_PIN(DT_NODELABEL(vbat_sense_en), gpios)
#define VMEASURE_EN_FLAGS	(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(vbat_sense_en), gpios))

#include "app_thread_configs.h"
#include "app_heater/app_heater.h"
#include "app_sensor/app_sensor.h"
#include "app_settings/app_settings_paths.h"
#include "lib_events/lib_events.h"
#include "app_settings/app_settings.h"

/*Macros for heater state*/
#define HIGH 1
#define LOW 0
#define UNSET -999

/*Macro for MIN and MAX range for variables set from Kconfig file*/
#define HUMIDITY_VAL_MAX CONFIG_HEATER_24W_HUMIDITY_MAX
#define HUMIDITY_VAL_MIN CONFIG_HEATER_24W_HUMIDITY_MIN
#define TEMP_12W_VAL_MAX CONFIG_HEATER_12W_TEMP_MAX
#define TEMP_12W_VAL_MIN CONFIG_HEATER_12W_TEMP_MIN
#define RETRY_COUNTER_24W CONFIG_RETRY_COUNTER_24W
#define RETRY_COUNTER_12W CONFIG_RETRY_COUNTER_12W

/*Macros for default values of heater*/
#define DEFAULT_HUMID_PER 65
#define DEFAULT_TEMP 45
#define DEFAULT_CHECK_INTERVAL 1000
#define MAX_TEMP_NTC 120

/*Static global variables for lib events*/
static struct lib_events_callback m_heater_settings_changed;
static struct lib_events_callback m_heater_eventsuspend;
static struct lib_events_callback m_heater_poweroff;
static struct lib_events_callback m_heater_reboot;

/*24W heater thread static variables*/
K_THREAD_STACK_DEFINE(m_p_24w_stack, APP_THREAD_STACK_SIZE_PID)
;
static struct k_thread m_p_24w_data;
static k_tid_t m_p_24w_tid;

/*12W heater thread static variables*/
K_THREAD_STACK_DEFINE(m_p_12w_stack, APP_THREAD_STACK_SIZE_PID)
;
static struct k_thread m_p_12w_data;
static k_tid_t m_p_12w_tid;

/*Static global variables which contain the values of temperature and humidity for maintain the loop*/
static struct heater_24w_var m_heater_24W_var;
static struct heater_12w_var m_heater_12W_var;

/*Static global variable to kill the thread*/
static int m_exit_24W_heater_thread = LOW;
static int m_exit_12W_heater_thread = LOW;

/*Function to turn on the heater*/
static int app_heater_state_enable(APP_HEATER_MODULE heater_wat) {
	int ret = 0;
	switch (heater_wat) {

	case APP_HEATER_MODULE_24: {
		const struct gpio_dt_spec dev =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hm1_24w_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}
		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}

		/*Setting the GPIO pin high to turn on the 24W heater*/
		ret = gpio_pin_set_dt(&dev, HIGH);
		return ret;
	}
		break;

	case APP_HEATER_MODULE_12: {
		const struct gpio_dt_spec dev =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hm1_12w_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}
		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}

		/*Setting the GPIO pin high to turn on the 12W heater*/
		ret = gpio_pin_set_dt(&dev, HIGH);
		return ret;
	}
		break;
	default:
		break;
	}

	return ret;
}

/*Function to turn off the heater*/
static int app_heater_state_disable(APP_HEATER_MODULE heater_wat) {
	int ret = 0;
	switch (heater_wat) {

	case APP_HEATER_MODULE_24: {
		const struct gpio_dt_spec dev =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hm1_24w_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}
		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}

		/*Setting the GPIO pin low to turn off the 24W heater*/
		ret = gpio_pin_set_dt(&dev, LOW);
		return ret;
	}
		break;

	case APP_HEATER_MODULE_12: {
		const struct gpio_dt_spec dev =
		GPIO_DT_SPEC_GET(DT_NODELABEL(hm1_12w_en), gpios);
		if (!gpio_is_ready_dt(&dev)) {
			LOG_ERR("gpio %d not ready", dev.pin);
			return -1;
		}
		ret = gpio_pin_configure_dt(&dev, (GPIO_OUTPUT | dev.dt_flags));
		if (ret < 0) {
			LOG_ERR("gpio %d configure failed", dev.pin);
			return -1;
		}

		/*Setting the GPIO pin low to turn off the 12W heater*/
		ret = gpio_pin_set_dt(&dev, LOW);
		return ret;
	}
		break;
	default:
		break;
	}

	return ret;
}

/*24W heater thread*/
static void heater_24W_p_thread(void *p1, void *p2, void *p3) {
	float temp_c, humid_per;
	int ret,i = 0;
	while (1) {

		/*Loop to retry specific number of times to fetch the data*/
		for (i = RETRY_COUNTER_24W; i > LOW; i--) {

			/*Fetch the temperature from NTC*/
			ret = app_sensor_temp_c_get(3, &temp_c);
			if (!ret) {

				/*The for loop breaks if the data is been fetched from the sensor*/
				break;
			}

			/*If the data is not been fetched then the loop retries to get the data*/
			LOG_INF("LOG:24W Retry: Fetching NTC data");
		}

		if (!ret) {

			/*Checking if the heater has reached it's absolute maximum heating temperature*/
			if (m_heater_24W_var.ntc_thermistor_c * 1000 > temp_c * 1000) {

				/*Loop to retry specific number of times to fetch the data*/
				for (i = RETRY_COUNTER_24W; i > LOW; i--) {

					/*Fetching the humidity percent from the sensor*/
					ret = app_sensor_humid_percent_get(6, &humid_per);
					if (!ret) {

						/*The for loop breaks if the data is been fetched from the sensor*/
						break;
					}

					/*If the data is not been fetched then the loop retries to get the data*/
					LOG_INF("LOG:24W Retry: Fetching humidity data");
				}


				if (!ret) {

					/*Checking if the humidity has reached it's set level*/
					if (m_heater_24W_var.humidity_per * 1000 > humid_per * 1000) {

						/*If the desired humidity percentage is not achieved then the heater is turned on*/
						LOG_INF("LOG:24W Humidity = %0.2f Reg = ON", (double)humid_per);

						/*Checking if the heater is turning on or not*/
						ret = app_heater_state_enable(APP_HEATER_MODULE_24);
						if (ret != 0) {

							LOG_ERR("LOG:24W Failed to turn on the heater");
							/*Reset the exit variable for thread*/
							m_exit_24W_heater_thread = LOW;
							goto err_24W;
						}
					} else {

						/*If the desired humidity percentage is achieved then the heater is turned off*/
						LOG_INF("LOG:24W Humidity = %0.2f Reg = OFF",
								(double)humid_per);
						/*Turning heater off as the humidity level is reached it's set value*/
						app_heater_state_disable(APP_HEATER_MODULE_24);
					}

				} else {

					/*Disabling the heater as the humidity fetching function fails*/
					LOG_ERR("LOG:24W Failed to receive the humidity readings");

					/*Reset the exit variable for thread*/
					m_exit_24W_heater_thread = LOW;

					goto err_24W;

				}
			} else {

				/*Turning heater off after reaching it's absolute maximum temperature*/
				app_heater_state_disable(APP_HEATER_MODULE_24);
				LOG_INF(
						"LOG:24W Resistor = %0.2f Max-Temp-Reg= OFF",
						(double)temp_c);
			}
		} else {

			/*Disabling the heater as the temperature fetching function fails*/
			LOG_ERR("LOG:24W Failed to receive the temperature readings");

			/*Reset the exit variable for thread*/
			m_exit_24W_heater_thread = LOW;

			goto err_24W;
		}

		LOG_INF("LOG:24W Resistor = %0.2f-C", (double)temp_c);

		/*Thread termination check*/
		if (m_exit_24W_heater_thread == LOW) {

			/*Reset the exit variable for thread*/
			//m_exit_24W_heater_thread = HIGH;

			/*Thread termination*/
			LOG_INF("LOG:24W HEATER-OFF");

			goto err_24W;
		}

		/*Delay of while loop*/
		k_sleep(K_MSEC(m_heater_24W_var.check_interval));
	}

	/*goto statement to handle the error and exits the thread*/
	err_24W:
		app_heater_state_disable(APP_HEATER_MODULE_24);
		return;

}

/*12W heater thread*/
static void heater_12W_p_thread(void *p1, void *p2, void *p3) {
	float temp_c;
	int ret,i = 0;
	while (1) {

		/*Loop to retry specific number of times to fetch the data*/
		for (i = RETRY_COUNTER_12W; i > LOW; i--) {

			/*Fetch the temperature from NTC*/
			ret = app_sensor_temp_c_get(2, &temp_c);
			if (!ret) {

				/*The for loop breaks if the data is been fetched from the sensor*/
				break;
			}

			/*If the data is not been fetched then the loop retries to get the data*/
			LOG_INF("LOG:12W Retry: Fetching NTC data");
		}

		if (!ret) {

			/*Checking if the heater has reached it's heating temperature*/
			if (m_heater_12W_var.ntc_thermistor_temp_c * 1000 > temp_c * 1000) {

				/*If the desired temperature is not achieved then the heater is turned on*/
				LOG_INF("LOG:12W Temperature = %0.2f Reg = ON", (double)temp_c);

				/*Checking if the heater is turning on or not*/
				ret = app_heater_state_enable(APP_HEATER_MODULE_12);
				if (ret != 0) {

					LOG_ERR("LOG:12W Failed to turn on the heater");

					/*Reset the exit variable for thread*/
					m_exit_12W_heater_thread = LOW;
					goto err_12W;
				}
			} else {

				/*If the desired temperature is achieved then the heater is turned off*/
				LOG_INF("LOG:12W Temperature = %0.2f Reg = OFF", (double)temp_c);
				app_heater_state_disable(APP_HEATER_MODULE_12);
			}

		} else {
		
			/*Disabling the heater as the temperature fetching function fails*/
			LOG_ERR("LOG:12W Failed to receive the temperature readings");

			/*Reset the exit variable for thread*/
			m_exit_12W_heater_thread = LOW;
			goto err_12W;
		}

		/*Thread termination check*/
		if (m_exit_12W_heater_thread == LOW) {

			/*Reset the exit variable for thread*/
			//m_exit_12W_heater_thread = HIGH;

			/*Thread termination*/
			LOG_INF("LOG:12W HEATER-OFF");
			goto err_12W;
		}

		/*Delay of while loop*/
		k_sleep(K_MSEC(m_heater_12W_var.chk_interval));
	}

	/*goto statement to handle the error and exits the thread*/
	err_12W:
		app_heater_state_disable(APP_HEATER_MODULE_12);
		return;
}

/*Function event handler*/
static void app_event_handler(struct lib_events_callback *cb,
		LIB_EVENT_TYPE event) {

	/*Switch case for the events which needs to be checked*/
	switch (event) {

	/*When the settings are changed it checks what all settings are changed*/
	case LIB_EVENT_SETTINGS_CHANGED: {

		char changed_setting[SETTINGS_FULLPATH_LEN_MAX] = { 0x00 };
		app_settings_changed_latest_get(changed_setting);

		/*Compares if the 24W heater state is changed or not either turned on/off*/
		if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_HET_W24HST) == 0) {

			struct setting_value heater_24w_state;

			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_HET_W24HST,
					&heater_24w_state, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed",
						SETTINGS_KEY_FULL_CS_HET_W24HST);
			}

			/*Checking the value of the variable when settings is changed*/
			/*If the value is 1 then the heater is turned on*/
			if (heater_24w_state.val1 == 1) {

				/*This condition checks if the thread is running then the thread cannot be start again*/
				if(m_exit_24W_heater_thread == LOW) {
				app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_DEFAULT, UNSET,
						UNSET, UNSET, UNSET, UNSET);
				} else {
					LOG_ERR("LOG:24W Heater is already on!!!");
				}
				
				/*Else if the value is 0 heater is turned off*/
			} else if (heater_24w_state.val1 == 0) {
				app_heater_control_thread_dis(APP_HEATER_MODULE_24);
			}
			break;

			/*Compares if the 12W heater state is changed in the setting or not*/
		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_HET_W12HST)
				== 0) {

			struct setting_value heater_12w_state;

			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_HET_W12HST,
					&heater_12w_state, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed",
						SETTINGS_KEY_FULL_CS_HET_W12HST);
			}

			/*Checking the value of the variable when settings is changed*/
			/*If the value is 1 then the heater is turned on*/
			if (heater_12w_state.val1 == 1) {

				/*This condition checks if the thread is running then the thread cannot be start again*/
				if (m_exit_12W_heater_thread == LOW) {
					app_heater_control_thread_en(APP_HEATER_MODULE_12, HEATER_CONFIG_DEFAULT,
							UNSET, UNSET, UNSET, UNSET, UNSET);
				} else {
					LOG_ERR("LOG:12W Heater is already on!!!");
				}
				
				/*Else if the value is 0 heater is turned off*/
			} else if (heater_12w_state.val1 == 0) {
				app_heater_control_thread_dis(APP_HEATER_MODULE_12);
			}
			break;

			/*Compares if the humidity values are changed*/
		} else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_HET_W24HUM)
				== 0) {

			struct setting_value humidity_per;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_HET_W24HUM,
					&humidity_per, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed",
						SETTINGS_KEY_FULL_CS_HET_W24HUM);
			}

			/*Updates the humidity values*/
			LOG_INF("LOG:24W Humidity = %d-PER Updated!!!", humidity_per.val1);

			if(humidity_per.val1 != UNSET) {
				app_heater_control_thread_en(APP_HEATER_MODULE_24, HEATER_CONFIG_UPDATE, UNSET, UNSET,
									(float) humidity_per.val1, UNSET, UNSET);
			}
			else if(humidity_per.val1 == UNSET) {
				LOG_ERR("LOG:24W INVALID-RANGE Humidity can't be 0");
				LOG_INF("LOG:24W Humidity = %0.2f-PER Remains!!!", (double)m_heater_24W_var.humidity_per);
			}	break;

		}

		/*Compares if the regulated temperature is changed for 12W heater*/
		else if (strcmp(changed_setting, SETTINGS_KEY_FULL_CS_HET_W12TEMP)
				== 0) {

			struct setting_value temp_c;
			if (app_settings_load_single(SETTINGS_KEY_FULL_CS_HET_W12TEMP,
					&temp_c, sizeof(struct setting_value)) != 0) {
				LOG_ERR("app_settings_load_single for %s failed",
						SETTINGS_KEY_FULL_CS_HET_W12TEMP);
			}

			/*Updates the temperature values*/
			LOG_INF("LOG:12W Temperature = %d-C Updated!!!", temp_c.val1);
			if (temp_c.val1 != UNSET) {
				app_heater_control_thread_en(APP_HEATER_MODULE_12, HEATER_CONFIG_UPDATE, UNSET,
						(float) temp_c.val1, UNSET, UNSET, UNSET);

				/*If the temperature value comes 0 here then an error log is generated and modulates the heater with the old temperature value*/
			} else if (temp_c.val1 == UNSET) {
				LOG_ERR("LOG:12W INVALID-RANGE Temperature can't be 0");
				LOG_INF("LOG:12W Temperature = %0.2f-C Remains!!!", (double)m_heater_12W_var.ntc_thermistor_temp_c);
			}	break;
		}


		break;
	}

	case LIB_EVENT_SUSPEND: {

		/*Turning the heaters off*/
		LOG_INF("LIB_EVENT_SUSPEND");
		app_heater_control_thread_dis(APP_HEATER_MODULE_24);
		LOG_INF("LOG:24W HEATER-OFF");
		app_heater_control_thread_dis(APP_HEATER_MODULE_12);
		LOG_INF("LOG:12W HEATER-OFF");
	}
		break;

	case LIB_EVENT_POWER_OFF: {

		/*Turning the heaters off*/
		LOG_INF("LIB_EVENT_POWER_OFF");
		app_heater_control_thread_dis(APP_HEATER_MODULE_24);
		LOG_INF("LOG:24W HEATER-OFF");
		app_heater_control_thread_dis(APP_HEATER_MODULE_12);
		LOG_INF("LOG:12W HEATER-OFF");
	}
		break;

	case LIB_EVENT_REBOOT: {

		/*Turning the heaters off*/
		LOG_INF("LIB_EVENT_REBOOT");
		app_heater_control_thread_dis(APP_HEATER_MODULE_24);
		LOG_INF("LOG:24W HEATER-OFF");
		app_heater_control_thread_dis(APP_HEATER_MODULE_12);
		LOG_INF("LOG:12W HEATER-OFF");
	}
		break;

	default:
		break;
	}
}

/* TODO call this function from event handler */
/*Function to start the heater control thread for heating water and tube*/
int app_heater_control_thread_en(APP_HEATER_MODULE heater_wat,
		APP_HEATER_CONFIG heater_set, float heater_24w_temp_c,
		float heater_12w_temp_c, float humid_24w_per, int interval_24w_ms,
		int interval_12w_ms) {

	int ret = 0;

	/*Switch case to select the heater to be enabled*/
	switch (heater_wat) {

	case APP_HEATER_MODULE_24: {

		/*Switch case to select the default state of heater or wants to update any variables*/
		switch (heater_set) {

		/*Default state is set for heater to start*/
		case HEATER_CONFIG_DEFAULT: {

			/*Default values of temperature, humidity and interval are set when heater is turned on*/
			m_heater_24W_var.ntc_thermistor_c = MAX_TEMP_NTC;
			m_heater_24W_var.humidity_per = DEFAULT_HUMID_PER;
			m_heater_24W_var.check_interval = DEFAULT_CHECK_INTERVAL;
			LOG_INF(
					"LOG:24W Default_Humidity = %d-PER Default_max_temperature - %d-C",
					DEFAULT_HUMID_PER, MAX_TEMP_NTC);

			/*Set the thread variable HIGH as the thread is been created*/
			m_exit_24W_heater_thread = HIGH;

			/*24W heater P thread*/
			m_p_24w_tid = k_thread_create(&m_p_24w_data, m_p_24w_stack,
			K_THREAD_STACK_SIZEOF(m_p_24w_stack), heater_24W_p_thread, NULL,
					NULL, NULL,
					APP_THREAD_PRIO_PID, 0, K_NO_WAIT);

			k_thread_name_set(m_p_24w_tid, APP_THREAD_NAME_24W_P);

		}
			break;

			/*User can update the config as per the requirements*/
		case HEATER_CONFIG_UPDATE: {

			/*Checks the condition which parameter is been set*/
			/*Checks if the temperature is been set or not*/
			if (heater_24w_temp_c != UNSET) {
				m_heater_24W_var.ntc_thermistor_c = heater_24w_temp_c;
			}

			/*If the humidity values are set and the humidity value lies in the MIN and MAX range then this condition works*/
			if (humid_24w_per != UNSET
					&& HUMIDITY_VAL_MIN*1000
							<= humid_24w_per*1000 && humid_24w_per*1000 <= HUMIDITY_VAL_MAX*1000) {

				m_heater_24W_var.humidity_per = humid_24w_per;
				LOG_INF("LOG:24W Humidity = %0.2f-PER Updated!!!", (double)humid_24w_per);
			}

			/*Else if the user trying to set the humidity value lesser than the MIN value then, the MIN value of the range is been set*/
			else if (humid_24w_per != UNSET && HUMIDITY_VAL_MIN*1000 > humid_24w_per*1000) {

				/*Printing ERR LOG user is trying to set invalid range*/
				LOG_ERR("LOG:24W INVALID-RANGE Humidity-range - %d-%d-PER",
						HUMIDITY_VAL_MIN, HUMIDITY_VAL_MAX);
				LOG_INF("LOG:24W Humidity = %d-PER Updated!!!",
						HUMIDITY_VAL_MIN);
				m_heater_24W_var.humidity_per = HUMIDITY_VAL_MIN;
			}

			/*Else if the user trying to set the humidity value more than the MAX value then, the MAX value of the range is been set*/
			else if (humid_24w_per != UNSET && HUMIDITY_VAL_MAX*1000 < humid_24w_per*1000) {

				/*Printing ERR LOG user is trying to set invalid range*/
				LOG_ERR("LOG:24W INVALID-RANGE Humidity_range - %d-%d-PER",
						HUMIDITY_VAL_MIN, HUMIDITY_VAL_MAX);
				LOG_INF("LOG:24W Humidity = %d-PER Updated!!!",
						HUMIDITY_VAL_MAX);
				m_heater_24W_var.humidity_per = HUMIDITY_VAL_MAX;
			}
			break;

			/*Checks if the interval is been set or not*/
			if (interval_24w_ms != UNSET) {
				m_heater_24W_var.check_interval = interval_24w_ms;
			}
		}
			break;

		default:
			break;
		}

	}
		break;

	case APP_HEATER_MODULE_12: {

		/*Switch case to select the default state of heater or wants to update any variables*/
		switch (heater_set) {

		/*Default state is set for heater to start*/
		case HEATER_CONFIG_DEFAULT: {

			/*Default values of temperature and interval are set when heater is turned on*/
			m_heater_12W_var.ntc_thermistor_temp_c = DEFAULT_TEMP;
			m_heater_12W_var.chk_interval = DEFAULT_CHECK_INTERVAL;
			LOG_INF("LOG:12W Default_temperature = %d-C", DEFAULT_TEMP);

			/*Set the thread variable HIGH as the thread is been created*/
			m_exit_12W_heater_thread = HIGH;

			/*12W heater P thread*/
			m_p_12w_tid = k_thread_create(&m_p_12w_data, m_p_12w_stack,
			K_THREAD_STACK_SIZEOF(m_p_12w_stack), heater_12W_p_thread, NULL,
					NULL, NULL,
					APP_THREAD_PRIO_PID, 0, K_NO_WAIT);

			k_thread_name_set(m_p_12w_tid, APP_THREAD_NAME_12W_P);

		}
			break;

			/*User can update the config as per the requirements*/
		case HEATER_CONFIG_UPDATE: {

			/*Checks the condition which parameter is been set*/
			/*Checks the condition if the temperature is been set and also the temperature lies in the desired range*/
			if (heater_12w_temp_c != UNSET
					&& TEMP_12W_VAL_MIN*1000
							<= heater_12w_temp_c*1000 && heater_12w_temp_c*1000 <= TEMP_12W_VAL_MAX*1000) {

				m_heater_12W_var.ntc_thermistor_temp_c = heater_12w_temp_c;
				LOG_INF("LOG:12W Temperature = %0.2f-C Updated!!!",
						(double)heater_12w_temp_c);
			}

			/*Checks if the temperature set is below the MIN range, then MIN value is assigned*/
			else if (heater_12w_temp_c != UNSET
					&& TEMP_12W_VAL_MIN*1000 > heater_12w_temp_c*1000) {

				/*Printing ERR LOG user is trying to set invalid range*/
				LOG_ERR("LOG:12W INVALID-RANGE Temperature_range - %d-%d-C",
						TEMP_12W_VAL_MIN, TEMP_12W_VAL_MAX);
				LOG_INF("LOG:12W Temperature = %d-C Updated!!!",
						TEMP_12W_VAL_MIN);
				m_heater_12W_var.ntc_thermistor_temp_c = TEMP_12W_VAL_MIN;
			}

			/*Checks if the temperature set is above the MAX range, then MAX range is assigned*/
			else if (heater_12w_temp_c != UNSET
					&& TEMP_12W_VAL_MAX*1000 < heater_12w_temp_c*1000) {

				/*Printing ERR LOG user is trying to set invalid range*/
				LOG_ERR("LOG:12W INVALID-RANGE Temperature-RANGE - %d-%d-C",
						TEMP_12W_VAL_MIN, TEMP_12W_VAL_MAX);
				LOG_INF("LOG:12W Temperature = %d-C Updated!!!",
						TEMP_12W_VAL_MAX);
				m_heater_12W_var.ntc_thermistor_temp_c = TEMP_12W_VAL_MAX;
			}
			break;

			/*Checks if the interval is been set*/
			if (interval_12w_ms != UNSET) {
				m_heater_12W_var.chk_interval = interval_12w_ms;
			}
		}
			break;

		default:
			break;
		}
	}

		break;

	default:
		break;
	}

	return ret;
}

/*Function to disable the heater control thread*/
int app_heater_control_thread_dis(APP_HEATER_MODULE heater_wat) {

	int ret = 0;

	switch (heater_wat) {
	case APP_HEATER_MODULE_24: {

		/*Static global variable checking in while loop, when set the execution of the thread terminates by returning*/
		m_exit_24W_heater_thread = LOW;
	}
		break;

	case APP_HEATER_MODULE_12: {

		/*Static global variable checking in while loop, when set the execution of the thread terminates by returning*/
		m_exit_12W_heater_thread = LOW;
	}
		break;

	default:
		break;
	}

	return ret;
}

/*Init function for heater module*/
int app_heater_init() {
	int ret = 0;

	/*Function to call the lib events for settings changed*/
	ret = lib_events_callback_add(&m_heater_settings_changed, app_event_handler,
			LIB_EVENT_SETTINGS_CHANGED);

	ret = lib_events_callback_add(&m_heater_eventsuspend, app_event_handler,
			LIB_EVENT_SUSPEND);

	ret = lib_events_callback_add(&m_heater_poweroff, app_event_handler,
			LIB_EVENT_POWER_OFF);

	ret = lib_events_callback_add(&m_heater_reboot, app_event_handler,
			LIB_EVENT_REBOOT);

	return 0;
}
