# Copyright (c) 2021 Acme CPU

if(CONFIG_TPS61169)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(tps61169.c)
endif()
