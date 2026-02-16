# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_SHELLCMD)
list(APPEND app_sources 
			app_shellcmd.c
			app_shellcmd_stm32bl.c
	)
	
target_sources(app PRIVATE ${app_sources})
endif()
