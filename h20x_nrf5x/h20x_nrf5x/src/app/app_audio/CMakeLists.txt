# Copyright (c) 2022 Acme CPU

list(APPEND app_sources 
#			app_audio.c
#			codec_apis.c
#			raw.c
			app_audio_new.c
	)
	
target_sources(app PRIVATE ${app_sources})