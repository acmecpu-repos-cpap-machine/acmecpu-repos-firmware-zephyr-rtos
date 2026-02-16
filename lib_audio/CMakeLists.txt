# Copyright (c) 2023 Acme CPU

if (CONFIG_LIB_AUDIO)
	list(APPEND app_sources 
				lib_audio.c
				codec_apis.c
		)
		
	target_sources(app PRIVATE ${app_sources})
endif()