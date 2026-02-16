# Copyright (c) 2021 Acme CPU

if(CONFIG_MICROCRYSTAL_RV3028)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(microcrystal_rv3028.c)
endif()
