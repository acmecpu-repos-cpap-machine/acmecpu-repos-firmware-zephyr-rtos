/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_BSP_BLOWER_BSP_BLOWER_H_
#define SRC_INCLUDE_BSP_BLOWER_BSP_BLOWER_H_

#include <stdint.h>

#define BLOWER_ON	(1)
#define BLOWER_OFF	(0)

#define BSP_BLOWER_OK	(0)
#define BSP_BLOWER_FAIL	(1)
#define BSP_BLOWER_UNKNOWN	(2)

#define  MC_NO_FAULTS  (uint16_t)(0x0000u)     /**< @brief No error.*/
#define  MC_FOC_DURATION  (uint16_t)(0x0001u)  /**< @brief Error: FOC rate to high.*/
#define  MC_OVER_VOLT  (uint16_t)(0x0002u)     /**< @brief Error: Software over voltage.*/
#define  MC_UNDER_VOLT  (uint16_t)(0x0004u)    /**< @brief Error: Software under voltage.*/
#define  MC_OVER_TEMP  (uint16_t)(0x0008u)     /**< @brief Error: Software over temperature.*/
#define  MC_START_UP  (uint16_t)(0x0010u)      /**< @brief Error: Startup failed.*/
#define  MC_SPEED_FDBK  (uint16_t)(0x0020u)    /**< @brief Error: Speed feedback.*/
#define  MC_BREAK_IN  (uint16_t)(0x0040u)      /**< @brief Error: Emergency input (Over current).*/
#define  MC_SW_ERROR  (uint16_t)(0x0080u)      /**< @brief Software Error.*/


typedef enum {
	BLOWER_NOT_RUNNING = BLOWER_OFF,
	BLOWER_RUNNING = BLOWER_ON,
} BSP_BLOWER_RUNNING_STATUS;

int bsp_blower_init();

int bsp_blower_reset();

int bsp_blower_on();

int bsp_blower_off();

int bsp_blower_oper_voltage_set(float blower_voltage);

int bsp_blower_oper_voltage_get(int32_t *blower_voltage);

int bsp_blower_is_running();

int bsp_blower_duty_cycle_set(uint8_t duty_percent);

int bsp_blower_speed_inc(uint8_t percent_inc);

int bsp_blower_speed_dec(uint8_t percent_dec);

int bsp_blower_speed_set(int32_t speed_rpm);

int bsp_blower_speed_get(uint8_t *speed_percent, uint32_t *speed_hz, int32_t *speed_rpm);

int bsp_blower_runstat_get(uint16_t *fault, int32_t *speed_rpm);

int bsp_blower_fault_ack();

int bsp_blower_spdRamp_set(int32_t speed_rpm, uint32_t ramp_ms);

int bsp_blower_power_get(uint16_t *pow_avg_w, uint16_t *pow_inst_w);

#endif /* SRC_INCLUDE_BSP_BLOWER_BSP_BLOWER_H_ */


