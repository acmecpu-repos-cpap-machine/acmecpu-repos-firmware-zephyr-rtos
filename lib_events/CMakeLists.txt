# Copyright (c) 2022 Acme CPU

list(APPEND lib_sources 
			lib_events.c
	)
target_sources(app PRIVATE ${lib_sources})
