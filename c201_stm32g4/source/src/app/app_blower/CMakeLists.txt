# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_BLOWER)
	list(APPEND app_sources 
				app_blower.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()