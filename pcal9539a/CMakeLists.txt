# Copyright (c) 2021 Acme CPU

if(CONFIG_GPIO_PCAL9539A)
  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(gpio_pcal9539a.c)
endif()
