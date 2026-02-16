# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			app_error_handler.c
	)
target_sources(app PRIVATE ${app_sources})
