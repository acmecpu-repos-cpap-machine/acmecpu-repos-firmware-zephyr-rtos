/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_TPS55340_TPS55340_H_
#define MODULES_TPS55340_TPS55340_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <zephyr/device.h>
#if (CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
#define TPS55340_OUTPUT_VOLTAGE_MIN			(8.21)
#define TPS55340_OUTPUT_VOLTAGE_MAX			(24.0)
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
#define TPS55340_OUTPUT_VOLTAGE_MIN			(8.21)
#define TPS55340_OUTPUT_VOLTAGE_MAX			(20.39)
#endif
#define TPS55340_ADC_DAC_REF_VOLTAGE		(3.3)

#if (CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
/* Following values are obtained from the input-output curve of C201 TPS55340 schematic */
#define TPS55340_BOOST_CONVERTER_SLOPE		(3.691)
#define TPS55340_BOOST_CONVERTER_Y_INT		(8.21)
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
/* Following values are obtained from the input-output curve of C201 TPS55340 schematic */
#define TPS55340_BOOST_CONVERTER_SLOPE		(3.691)
#define TPS55340_BOOST_CONVERTER_Y_INT		(8.21)
#endif

#if (CONFIG_BOARD_C205 || CONFIG_BOARD_E206 || CONFIG_BOARD_E206W)
/* R1 = 470k, R2 = 75k */
#define TPS55340_ADC_VOLT_DIVIDER_R1		(470*1000)
#define TPS55340_ADC_VOLT_DIVIDER_R2		(75*1000)
#elif (CONFIG_BOARD_C204_CORE || CONFIG_BOARD_STM32G473_ACME_CPU_C201_OLED || CONFIG_BOARD_STM32G473_ACME_CPU_C201)
/* R1 = 390k, R2 = 75k */
#define TPS55340_ADC_VOLT_DIVIDER_R1		(390*1000)
#define TPS55340_ADC_VOLT_DIVIDER_R2		(75*1000)
#endif

/* API type defines */
typedef int (*tps55340_enable_t)(const struct device *);
typedef int (*tps55340_disable_t)(const struct device *);
typedef int (*tps55340_output_voltage_set_t)(const struct device *, float);
typedef int (*tps55340_output_voltage_get_t)(const struct device *, int32_t *);
typedef int (*tps55340_is_running_t)(const struct device *);

struct tps55340_driver_api {
	tps55340_enable_t enable;
	tps55340_disable_t disable;
	tps55340_output_voltage_set_t tps55340_output_voltage_set;
	tps55340_output_voltage_get_t tps55340_output_voltage_get;
	tps55340_is_running_t tps55340_is_running;
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_TPS55340_TPS55340_H_ */
