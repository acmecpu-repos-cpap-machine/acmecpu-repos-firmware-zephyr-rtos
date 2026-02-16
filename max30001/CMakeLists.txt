# Copyright (c) 2022 Acme CPU

if(CONFIG_MAX30001)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(max30001.c max30001_comm.c)
endif()
