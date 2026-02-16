# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_PUSH_SWITCH)
	list(APPEND app_sources 
				app_push_switch.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()