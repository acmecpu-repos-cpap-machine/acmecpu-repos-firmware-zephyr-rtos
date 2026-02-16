/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef APPLICATION_DRIVERS_ACPU_C201_BSP_H_
#define APPLICATION_DRIVERS_ACPU_C201_BSP_H_

extern struct k_sem ui_int_sync;

typedef int (*bsp_led_ind_ctrl_t)(const struct device *, uint16_t);
typedef int (*bsp_alive_t)(void);
typedef int (*bsp_gpio_int_cb_cfg_t)(void);
typedef void (*bsp_serial_debug_monitor_t)(void);

struct bsp_driver_api {
        bsp_led_ind_ctrl_t led_ind_ctrl;
        bsp_alive_t sys_alive;
        bsp_gpio_int_cb_cfg_t gpio_int_cb_cfg;
        bsp_serial_debug_monitor_t debug_output;
};

//#include <syscalls/ap_bsp_driver.h>
#endif /* APPLICATION_DRIVERS_ACPU_C201_BSP_H_ */
