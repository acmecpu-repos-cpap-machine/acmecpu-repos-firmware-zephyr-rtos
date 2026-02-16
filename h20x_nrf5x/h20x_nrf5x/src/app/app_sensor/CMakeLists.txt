# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			app_sensor.c
			bsp_sensor.c
	)
target_sources(app PRIVATE ${app_sources})
