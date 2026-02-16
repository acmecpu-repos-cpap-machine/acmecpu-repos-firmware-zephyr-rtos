# Copyright (c) 2021 Acme CPU

if(CONFIG_BUZZER_GENERIC)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(buzzer_generic.c)
endif()
