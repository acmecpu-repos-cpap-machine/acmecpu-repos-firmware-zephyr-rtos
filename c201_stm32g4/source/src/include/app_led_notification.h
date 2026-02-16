/*
 * Copyright (c) 2021 Acme CPU
 */


#ifndef SRC_INCLUDE_APP_LED_NOTIFICATION_H_
#define SRC_INCLUDE_APP_LED_NOTIFICATION_H_

#define APP_LED_NTF_BLINK_DELAY_ON			500
#define APP_LED_NTF_BLINK_DELAY_OFF			500
#define APP_LED_NTF_DEFAULT_BRIGHTNESS		70		/* default brightness in percentage */

/* LED indexes */
#if (CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)

#define APP_LED_POWER		0
#define APP_LED_SYS_ON		1
#define APP_LED_SYS_FAULT	2
#define APP_LED_RESERVED	3

#elif (CONFIG_BOARD_C205)

#define APP_LED_POWER		0
#define APP_LED_SYS_ON		1
#define APP_LED_SYS_FAULT	2
#define APP_LED_RESERVED	3

#endif

//int app_led_show_charging();
//int app_led_show_not_charging();
//int app_led_show_charging_done();
int app_led_show_fault();
//int app_led_show_ble_advertising();
//int app_led_show_ble_connected();
int app_led_init();

#endif /* SRC_INCLUDE_APP_LED_NOTIFICATION_H_ */
