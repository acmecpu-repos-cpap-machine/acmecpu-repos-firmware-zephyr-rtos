# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TPS22810)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(tps22810.c)
endif()
