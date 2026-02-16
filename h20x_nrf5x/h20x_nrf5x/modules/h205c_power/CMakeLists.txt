# Copyright (c) 2022 Acme CPU

if(CONFIG_H205C_POWER)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(h205c_power.c)
endif()
