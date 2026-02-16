# SPDX-License-Identifier: Apache-2.0

if(CONFIG_STSPIN220)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(stspin220.c)
endif()
