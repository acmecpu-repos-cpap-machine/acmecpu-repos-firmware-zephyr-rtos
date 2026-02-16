# Copyright (c) 2021 Acme CPU

if(CONFIG_BQ25890)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(bq25890.c)
endif()
