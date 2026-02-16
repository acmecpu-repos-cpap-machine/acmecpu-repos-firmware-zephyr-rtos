# SPDX-License-Identifier: Apache-2.0

if(CONFIG_PWR_CTRL_DRV)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(power_control_driver.c)
endif()
