# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DRV10975)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(drv10975.c)
endif()
