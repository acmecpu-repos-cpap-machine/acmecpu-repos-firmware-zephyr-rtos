/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_DRV10975_DRV10975_H_
#define MODULES_DRV10975_DRV10975_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <device.h>

/* BIT masks */
#define DRV10975_BIT_OVERRIDE	(1 << 7)

/* API type defines */
typedef int (*drv10975_configure_t)(const struct device*);
typedef int (*drv10975_enter_closed_loop_t)(const struct device*);
typedef int (*drv10975_speed_ctrl_ana_t)(const struct device *, const uint16_t);
typedef int (*drv10975_speed_ctrl_pwm_t)(const struct device *, const uint16_t);
typedef int (*drv10975_speed_ctrl_i2c_t)(const struct device *, const uint16_t, const uint8_t);
typedef int (*drv10975_dir_ctrl_t)(const struct device*, const uint8_t);
typedef int (*drv10975_status_get_t)(const struct device *);
typedef int (*drv10975_speed_get_t)(const struct device *, float *);
typedef int (*drv10975_motor_period_get_t)(const struct device *, float *);
typedef int (*drv10975_motor_kt_get_t)(const struct device *, float *);
typedef int (*drv10975_motor_current_get_t)(const struct device *, float *);
typedef int (*drv10975_initial_position_get_t)(const struct device *);
typedef int (*drv10975_supply_voltage_get_t)(const struct device *, float *);
typedef int (*drv10975_speed_cmd_get_t)(const struct device *);
typedef int (*drv10975_speed_cmd_buffer_get_t)(const struct device *);
typedef int (*drv10975_fault_code_get_t)(const struct device *);
typedef int (*drv10975_eeprom_val_get_t)(const struct device *, uint8_t*);

struct drv10975_driver_api {
	drv10975_configure_t configure;
	drv10975_enter_closed_loop_t enter_closed_loop;
	drv10975_speed_ctrl_ana_t speed_ctrl_ana;
	drv10975_speed_ctrl_pwm_t speed_ctrl_pwm;
	drv10975_speed_ctrl_i2c_t speed_ctrl_i2c;
	drv10975_dir_ctrl_t dir_ctrl;
	drv10975_status_get_t status_get;
	drv10975_speed_get_t speed_get;
	drv10975_motor_period_get_t motor_period_get;
	drv10975_motor_kt_get_t motor_kt_get;
	drv10975_motor_current_get_t motor_current_get;
	drv10975_initial_position_get_t initial_position_get;
	drv10975_supply_voltage_get_t supply_voltage_get;
	drv10975_speed_cmd_get_t speed_cmd_get;
	drv10975_speed_cmd_buffer_get_t speed_cmd_buffer_get;
	drv10975_fault_code_get_t fault_code_get;
	drv10975_eeprom_val_get_t eeprom_val_get;
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_DRV10975_DRV10975_H_ */
