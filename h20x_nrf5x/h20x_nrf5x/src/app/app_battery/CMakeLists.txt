# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_BATTERY)
	list(APPEND app_sources 
				app_battery.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()