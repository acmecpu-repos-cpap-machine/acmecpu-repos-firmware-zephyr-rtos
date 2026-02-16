# Copyright (c) 2021 Acme CPU

if (CONFIG_BSP_MEMBRANE_SWITCH)

list(APPEND app_sources 
			bsp_membrane_switch.c
	)
target_sources(app PRIVATE ${app_sources})

endif()