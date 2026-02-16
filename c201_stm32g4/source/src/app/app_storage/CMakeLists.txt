# Copyright (c) 2021 Acme CPU
if(CONFIG_APP_STORAGE)
	list(APPEND app_sources 
				app_storage.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()