# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_ANALOG)
	list(APPEND app_sources 
				app_analog.c
		)
		
	target_sources(app PRIVATE ${app_sources})
endif()