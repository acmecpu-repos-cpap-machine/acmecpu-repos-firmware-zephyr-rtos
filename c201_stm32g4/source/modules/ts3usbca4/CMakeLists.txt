# SPDX-License-Identifier: Apache-2.0

if(CONFIG_TS3USBCA4)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(ts3usbca4.c)
endif()
