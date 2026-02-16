# Copyright (c) 2023 Acme CPU

if (CONFIG_LIB_STM32BL_HOST)
	list(APPEND lib_sources 
				lib_stm32bl_host.c
		)
	target_sources(app PRIVATE ${lib_sources})
endif()
