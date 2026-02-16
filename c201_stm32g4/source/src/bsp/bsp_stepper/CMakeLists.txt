# Copyright (c) 2021 Acme CPU

if (NOT CONFIG_BOARD_C208T)
	list(APPEND app_sources 
				bsp_stepper.c
		)
target_sources(app PRIVATE ${app_sources})
endif()
