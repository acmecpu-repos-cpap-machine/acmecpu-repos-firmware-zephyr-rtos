# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BSP_LEDS)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(bsp_leds.c)
endif()
