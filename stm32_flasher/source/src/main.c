/*
 * Copyright (c) 2023 Acme CPU
 *
 * Author: Rohan Dey (rohan@acmecpu.com)
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#if CONFIG_USB_DEVICE_STACK
#include <zephyr/usb/usb_device.h>
#endif

#if (CONFIG_APP_STORAGE)
#include "app_storage/app_storage.h"
#endif

#if (CONFIG_APP_UART)
#include "app_uart/app_uart.h"
#endif

#include "lib_stm32bl_host/lib_stm32bl_host.h"

static void main_print_message() {
	printk("*** Starting C20x application ***\n\n");
}

int main(void)
{
	main_print_message();

	int ret = 0;

	ret = app_storage_mount();

#if CONFIG_USB_DEVICE_STACK
	if (usb_enable(NULL)) {
		return -1;
	}
#endif
	LOG_INF("The device is put in USB mass storage mode.\n");
#if 1
	/* initialize the uart interface */
	ret = app_uart_init();
	if (ret != 0) {
		LOG_ERR("app_uart_init failed");
		return -1;
	}

	/* initialize the bootloader host library */
	struct lib_stm32bl_host_funcs funcs;
	funcs.ms_delay = app_uart_ms_delay;
	funcs.usart_close = app_uart_close;
	funcs.usart_open = app_uart_open;
	funcs.usart_recv = app_uart_read_bytes;
	funcs.usart_send = app_uart_write_bytes;

	lib_stm32bl_host_init(&funcs);

	while (1) {
		k_sleep(K_MSEC(500));
//		app_storage_file_copy_complete();
	}
#endif
	return 0;
}
