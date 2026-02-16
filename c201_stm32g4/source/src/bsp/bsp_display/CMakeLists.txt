# Copyright (c) 2021 Acme CPU

if(CONFIG_APP_DISPLAY)
	list(APPEND app_sources 
				bsp_display.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()
