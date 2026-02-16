# Copyright (c) 2024 Acme CPU

if (CONFIG_APP_HAS_FLUID_LEVEL)
	list(APPEND app_sources 
				app_fluid_level.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()