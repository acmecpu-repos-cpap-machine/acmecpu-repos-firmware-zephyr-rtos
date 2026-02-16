# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			app_settings.c
			#app_settings_display_data.c
			app_settings_value.c
			app_settings_utils.c
			app_settings_cmd_iface.c
			app_settings_multi.c
	)
	
target_sources(app PRIVATE ${app_sources})
