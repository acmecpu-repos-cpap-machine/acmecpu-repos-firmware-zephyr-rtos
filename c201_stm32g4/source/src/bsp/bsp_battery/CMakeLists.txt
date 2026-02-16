# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_BATTERY)
	list(APPEND app_sources 
				bsp_battery.c
		)
		
	if (CONFIG_APP_USBC_PD)
		list(APPEND app_sources bsp_ucpd.c)
	endif()
	
	target_sources(app PRIVATE ${app_sources})
endif()