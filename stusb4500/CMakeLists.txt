# Copyright (c) 2023 Acme CPU

if(CONFIG_STUSB4500)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(stusb4500.c stusb4500_util.c USB_PD_core.c)
endif()
