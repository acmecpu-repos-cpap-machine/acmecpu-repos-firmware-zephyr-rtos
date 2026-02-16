/*
 * acpu_c201_bsp_gpio_config.h
 *
 *  Created on: 23-Feb-2021
 *      Author: Rohan Dey
 */

#ifndef SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_CONFIG_H_
#define SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_CONFIG_H_

/*
 * Maximum number of callback functions required for the application from GPIO interrupts
 *   membrane switch
 *   battery charger
 *   bg95
 *   ...
 */
#define ACPU_GPIO_MAX_CB			3

#define ACPU_GPIO_LABEL_DIO_1			"IO_1"
#define ACPU_GPIO_LABEL_DIO_2			"IO_2"
#define ACPU_GPIO_LABEL_DIO_3			"IO_3"
#define ACPU_GPIO_LABEL_UI_INT			"UI INT"
#define ACPU_GPIO_LABEL_SENS_INT		"SENSOR INT"
#define ACPU_GPIO_LABEL_TAMP_DET		"TAMP DET"


#endif /* SRC_APPLICATION_ACPU_C201_BSP_ACPU_C201_BSP_GPIO_CONFIG_H_ */
