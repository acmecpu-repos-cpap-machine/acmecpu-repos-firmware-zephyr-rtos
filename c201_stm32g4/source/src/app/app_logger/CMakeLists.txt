# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_LOGGER)
list(APPEND app_sources 
			app_logger.c
	)
	
	list(APPEND app_sources
				#app_logger_backend_sdcard.c
				#app_logger_backend_storage.c
		)

target_sources(app PRIVATE ${app_sources})
endif()
