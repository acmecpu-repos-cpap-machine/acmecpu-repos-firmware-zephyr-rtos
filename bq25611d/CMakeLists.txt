# Copyright (c) 2021 Acme CPU

if(CONFIG_BQ25611D)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(bq25611d.c)
endif()
