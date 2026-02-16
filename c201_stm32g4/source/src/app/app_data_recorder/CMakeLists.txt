# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_DATA_RECORDER)
	list(APPEND app_sources 
				app_data_recorder.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()
