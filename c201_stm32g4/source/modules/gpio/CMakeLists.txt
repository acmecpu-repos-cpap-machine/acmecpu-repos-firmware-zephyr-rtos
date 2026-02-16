# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(gpio_pca95xx_int.c)
zephyr_include_directories(./)

#zephyr_library_sources_ifdef(CONFIG_GPIO_SHELL      gpio_shell.c)
#zephyr_library_sources_ifdef(CONFIG_USERSPACE   gpio_handlers.c)
