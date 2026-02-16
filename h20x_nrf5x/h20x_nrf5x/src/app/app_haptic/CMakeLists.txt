# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_HAPTIC_FB)
	list(APPEND app_sources 
				app_haptic.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()