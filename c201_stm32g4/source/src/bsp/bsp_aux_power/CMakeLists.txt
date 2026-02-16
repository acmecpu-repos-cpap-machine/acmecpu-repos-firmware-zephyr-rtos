# Copyright (c) 2021 Acme CPU

list(APPEND app_sources
			bsp_aux_power.c 
			load_switch_tps22810.c
	)
target_sources(app PRIVATE ${app_sources})
