# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_DEVCMD)
list(APPEND app_sources 
			app_devcmd_packet.c
			app_devcmd.c
			app_devcmd_blower.c
			app_devcmd_battery.c
			app_devcmd_dfu.c
			app_devcmd_stepper.c
	)
	
target_sources(app PRIVATE ${app_sources})
endif()