# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			bsp_blower.c
	)
target_sources(app PRIVATE ${app_sources})
