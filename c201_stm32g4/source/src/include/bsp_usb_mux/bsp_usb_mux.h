/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_BSP_USB_MUX_BSP_USB_MUX_H_
#define SRC_INCLUDE_BSP_USB_MUX_BSP_USB_MUX_H_

/*
 * This function selects the USB1 port of T3USB3031 chip.
 * On the C201 board the USB1 port is connected to the
 * D+/D- lines of STM32
 * */
int bsp_usb_mux_select_usb1();

/*
 * This function selects the USB2 port of T3USB3031 chip.
 * On the C201 board the USB1 port is connected to the
 * D+/D- lines of BG95
 * */
int bsp_usb_mux_select_usb2();

/*
 * This function selects the MHL port of T3USB3031 chip.
 * On the C201 board the MHL port is connected to the
 * CP2102 D+/D- lines which bridges the ESP32 UART pins
 * */
int bsp_usb_mux_select_mhl();

#endif /* SRC_INCLUDE_BSP_USB_MUX_BSP_USB_MUX_H_ */
