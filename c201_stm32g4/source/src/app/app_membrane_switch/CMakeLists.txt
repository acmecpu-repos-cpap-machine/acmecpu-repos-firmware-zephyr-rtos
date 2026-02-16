# Copyright (c) 2021 Acme CPU

#if (CONFIG_APP_MEMBRANE_SWITCH)
if (0)

list(APPEND app_sources 
			app_membrane_switch.c
	)
target_sources(app PRIVATE ${app_sources})

endif()
