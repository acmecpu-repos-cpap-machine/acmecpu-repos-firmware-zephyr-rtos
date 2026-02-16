/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <soc.h>

#include <pinmux/stm32/pinmux_stm32.h>

/* pin assignments for STM32G474-DISCO board */
static const struct pin_config pinconf[] = {
#if DT_NODE_HAS_STATUS(DT_NODELABEL(usart3), okay)
	{STM32_PIN_PC10, STM32G4X_PINMUX_FUNC_PC10_USART3_TX},
	{STM32_PIN_PC11, STM32G4X_PINMUX_FUNC_PC11_USART3_RX},
#endif
};

static int pinmux_stm32_init(const struct device *port)
{
	ARG_UNUSED(port);

	stm32_setup_pins(pinconf, ARRAY_SIZE(pinconf));

	return 0;
}

SYS_INIT(pinmux_stm32_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_STM32_DEVICE_INITIALIZATION_PRIORITY);
