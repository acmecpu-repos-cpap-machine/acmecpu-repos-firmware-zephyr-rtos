# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ACPU_C201_BSP_DRIVER)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(acpu_c201_bsp.c acpu_c201_bsp_blower.c)
endif()
