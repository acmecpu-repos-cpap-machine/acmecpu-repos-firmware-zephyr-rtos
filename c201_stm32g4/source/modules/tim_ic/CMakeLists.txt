# SPDX-License-Identifier: Apache-2.0

#zephyr_library()
zephyr_include_directories(./)
zephyr_library_sources_ifdef(CONFIG_TIM_IC_STM32	tim_ic_stm32.c)

#zephyr_library_sources_ifdef(CONFIG_USERSPACE   pwm_handlers.c)
#zephyr_library_sources_ifdef(CONFIG_PWM_SHELL   pwm_shell.c)
