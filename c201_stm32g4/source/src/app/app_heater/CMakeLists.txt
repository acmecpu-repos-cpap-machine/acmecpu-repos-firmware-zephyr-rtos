# Copyright (c) 2021 Acme CPU
if (CONFIG_APP_HAS_HEATER)
	list(APPEND app_sources 
				app_heater.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()