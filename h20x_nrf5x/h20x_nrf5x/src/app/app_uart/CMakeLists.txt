# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_UART)
	list(APPEND app_sources 
				app_uart.c
		)
		
	target_sources(app PRIVATE ${app_sources})
endif()