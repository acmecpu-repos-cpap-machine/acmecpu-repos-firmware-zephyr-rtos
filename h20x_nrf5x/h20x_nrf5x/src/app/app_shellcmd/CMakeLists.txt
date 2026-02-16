# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_SHELLCMD)
list(APPEND app_sources 
			app_shellcmd.c
			app_shellcmd_audio.c
			app_shellcmd_sensor.c
			app_shellcmd_battery.c
	)
	
target_sources(app PRIVATE ${app_sources})
endif()