# Copyright (c) 2021 Acme CPU

if (CONFIG_LIB_PID_CONTROL)

list(APPEND lib_sources 
			PID.c
	)
target_sources(app PRIVATE ${lib_sources})

endif()