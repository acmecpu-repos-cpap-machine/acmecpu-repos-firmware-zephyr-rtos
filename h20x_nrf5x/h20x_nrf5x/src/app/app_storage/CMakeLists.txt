# Copyright (c) 2022 Acme CPU

list(APPEND app_sources 
			app_storage.c
	)
	
target_sources(app PRIVATE ${app_sources})