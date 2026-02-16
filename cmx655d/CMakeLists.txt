# Copyright (c) 2022 Acme CPU

if(CONFIG_CMX655D)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(cmx655d.c)
endif()
