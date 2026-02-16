# Copyright (c) 2023 Acme CPU

if(CONFIG_APP_UART)
	list(APPEND app_sources 
				app_uart.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()