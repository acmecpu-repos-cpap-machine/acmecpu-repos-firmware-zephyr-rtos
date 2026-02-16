# Copyright (c) 2021 Acme CPU

if(CONFIG_TPS55340)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(tps55340.c)
endif()
