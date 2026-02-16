# Copyright (c) 2021 Acme CPU

if(CONFIG_BSP_GPIO_DRIVER AND CONFIG_BLOWER_MOTOR_A2)

  zephyr_include_directories(./)
  zephyr_library()
  zephyr_library_sources(bsp_gpios.c)
  
#  if(CONFIG_BLOWER_MOTOR_A2)
	zephyr_library_sources(bsp_gpios.c bsp_fan.c)
#  endif()
endif()
