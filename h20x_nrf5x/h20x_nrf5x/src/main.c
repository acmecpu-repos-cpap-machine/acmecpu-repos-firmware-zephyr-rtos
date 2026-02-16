/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 20-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#if CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif

#include <zephyr/drivers/gpio.h>
#if (!CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	#include "app_utils/app_utils.h"
	#include "app_audio/app_audio.h"
	#include "app_storage/app_storage.h"
#endif
#include "app_sensor/app_sensor.h"
#if (CONFIG_APP_HAS_PPG_SENSOR)
	#include "shmax30101.h"
#endif
#include "app_analog/app_analog.h"
#include "app_uart/app_uart.h"
#include "app_haptic/app_haptic.h"
#include "app_battery/app_battery.h"
#include "app_push_switch/app_push_switch.h"
#include "lib_push_switch/lib_push_switch.h"

// #define CHECK_OXIMETER			1
// #define CHECK_ADC				1
// #define CHECK_UART_M2M			1
// #define CHECK_HAPTIC				1
// #define CHECK_H205C_AB_DET		1
// #define CHECK_SWITCH_POLL			1

/* oxh_rst */
// #define OXH_RST_DEV_NAME 	DT_GPIO_LABEL(DT_NODELABEL(oxh_rst), gpios)
#define OXH_RST_PIN			DT_GPIO_PIN(DT_NODELABEL(oxh_rst), gpios)
#define OXH_RST_FLAGS		(GPIO_OUTPUT_INACTIVE | GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(oxh_rst), gpios))

/* h205c_ab_det */
#define H205C_AB_DET_PIN	DT_GPIO_PIN(DT_NODELABEL(h205c_ab_det), gpios)
#define H205C_AB_DET_FLAGS	DT_GPIO_FLAGS(DT_NODELABEL(h205c_ab_det), gpios)

static void main_print_message()
{
	printk("*** Starting Acme CPU H20X application: %s ***\n\n", CONFIG_BOARD);
}

int main_app_init()
{
	int ret = 0;

#if (!CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
	/* initialize storage */
	ret |= app_storage_init();
	
	/* initialize audio */
	ret |= app_audio_init();
#endif	/*(!CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)*/

	/* initialize sensors */
	ret |= app_sensor_init();

	/* initialize adc channels */
	ret |= app_analog_init();

	/* initialize uart_m2m */
	ret |= app_uart_m2m_com_init();

	/* initialize haptic feedback */
	ret |= app_haptic_init();

	/* initialize the charger and fuel gauge app */
	ret |= app_battery_init();

	/* initialize push switches */
	ret |= app_push_switch_init();

	return ret;
}

void main(void)
{
	int ret = 0;

	main_print_message();

	// ret = app_utils_power_enable();
	// if (ret < 0) {
	// 	LOG_ERR("enable_power failed");
	// 	return;
	// }

#if (CONFIG_BOARD_H205C_NRF5340_CPUAPP || CONFIG_BOARD_H205C_NRF5340_CPUAPP_NS)
	/* select charger USB */
	ret = app_utils_usb_channel_select(USB_DATA_CHANNEL_CHARGER);
	// ret = app_utils_usb_channel_select(USB_DATA_CHANNEL_HOST);
	if (ret < 0) {
		LOG_ERR("app_utils_usb_channel_select failed");
		return;
	}
#endif

#if CONFIG_USB_DEVICE_STACK
	const struct device *dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
	if (!device_is_ready(dev)) {
		LOG_ERR("CDC ACM device not ready");
		return;
	}

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}
#endif	/* CONFIG_USB_DEVICE_STACK */

	/* initialize applications */
	ret = main_app_init();
	if (ret == 0) {
		LOG_INF("Application init successful");
	} else {
		LOG_ERR("Application init failed");
		// return;
	}

#ifdef CHECK_H205C_AB_DET
	const struct device *h205c_ab_det_dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(h205c_ab_det), gpios));
	if (h205c_ab_det_dev == NULL) {
		LOG_ERR("Device not found: %s", h205c_ab_det_dev->name);
		return;
	}
	ret = gpio_pin_configure(h205c_ab_det_dev, H205C_AB_DET_PIN, (GPIO_INPUT | H205C_AB_DET_FLAGS));
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return;
	}
#endif

	while (1) {
#ifdef CHECK_OXIMETER
		uint8_t sen_ids[10] = {0};
		int count=0;
		struct sensor_value val[3];
		float hr=0, spo2=0;
		uint8_t status=0, hr_conf=0;

		/* get hr and spo2 */
		ret = app_sensor_chan_to_id(SENSOR_CHAN_HR_AND_SPO2, sen_ids, &count);
		if (ret < 0) {
			k_sleep(K_MSEC(100));
			continue;
		}
		ret = app_sensor_value_get(sen_ids[0], val);
		if (!ret) {
			status = val[2].val1;
			if (status == 3) {			/* finger detected */
				hr_conf = val[0].val2;//*0.392157;
				// if ((hr_conf > 90.0)) {
					hr = val[0].val1/10;	// TODO add a validation range
					spo2 = val[1].val1/10;	// TODO add a validation range

					LOG_INF("%0.1f,%d,%0.1f,%d", hr, hr_conf, spo2, status);
				// }
			} else if (status == 0) {	/* no object detected */
				// LOG_ERR("nod");
			}
		} else {
			LOG_ERR("error getting sensor data!");
			// TODO reset the sensor
		}

		k_sleep(K_MSEC(100));
#endif

#ifdef CHECK_ADC
		int32_t vbat_mv, vbus_mv;
		ret = app_analog_vbat_mv_get(&vbat_mv);
		// LOG_INF("VBAT = %d, ret = %d", vbat_mv, ret);
		ret = app_analog_vbus_mv_get(&vbus_mv);
		LOG_INF("VBAT = %d, VBUS = %d, ret = %d", vbat_mv, vbus_mv, ret);

		k_sleep(K_MSEC(1000));
#endif

#ifdef CHECK_UART_M2M
	app_uart_get_and_print();
#endif

#ifdef CHECK_HAPTIC
	app_haptic_on();
	k_sleep(K_MSEC(1000));
	app_haptic_off();
	k_sleep(K_MSEC(1000));
#endif

#ifdef CHECK_H205C_AB_DET
	ret = gpio_pin_get_raw(h205c_ab_det_dev, H205C_AB_DET_PIN);
	if (ret == 0) {
		LOG_INF("H205C AB connected");
	} else if (ret == 1) {
		LOG_INF("H205C AB disconnected");
	}
	k_sleep(K_MSEC(5000));
#endif

#ifdef CHECK_SWITCH_POLL
	if (lib_push_switch_state_get(BSP_MEMBR_SWITCH_DEVICE_MIC, BSP_MEMBR_SWITCH_PIN_MIC) == SWITCH_ASSERTED) {
		lib_push_switch_poll_handler(BSP_MEMBR_SWITCH_DEVICE_MIC, BSP_MEMBR_SWITCH_MASK_MIC, BSP_MEMBR_SWITCH_PIN_MIC);
	}
	k_sleep(K_MSEC(100));
#endif
	k_sleep(K_MSEC(1000));
	}
}
