# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			bsp_buzzer.c
	)
target_sources(app PRIVATE ${app_sources})
