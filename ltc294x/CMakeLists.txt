# Copyright (c) 2021 Acme CPU

if(CONFIG_LTC294X)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(ltc294x.c)
endif()
