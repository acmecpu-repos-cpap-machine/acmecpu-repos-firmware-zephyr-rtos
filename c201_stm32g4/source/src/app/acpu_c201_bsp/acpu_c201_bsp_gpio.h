/*
 * acpu_c201_bsp.h
 *
 *  Created on: Feb 23, 2021
 *      Author: Rohan Dey
 */

#ifndef SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_H_
#define SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_H_

#include <drivers/gpio.h>
#include <stdint.h>

typedef void (*acpu_c201_bsp_gpio_callback_handler_t)(gpio_port_pins_t pins);

int acpu_c201_bsp_gpio_add_callback(const char *port_name, gpio_port_pins_t pins,
		acpu_c201_bsp_gpio_callback_handler_t cb);
int acpu_c201_bsp_led_ind_control(uint16_t control);
int acpu_c201_bsp_sys_alive(void);
void acpu_c201_bsp_serial_debug_output(void);
int acpu_c201_bsp_gpio_init();

#endif /* SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_H_ */
