# Copyright (c) 2021 Acme CPU

if(CONFIG_STM32_OPTION_BYTES_DRIVER)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(stm32_option_bytes.c)
endif()
