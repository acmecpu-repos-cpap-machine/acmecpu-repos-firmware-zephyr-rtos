# Copyright (c) 2022 Acme CPU

if(CONFIG_APP_UART_M2M)

	add_subdirectory(app_wifi_bt)
	add_subdirectory(app_bldc_drv)
	
	list(APPEND app_sources 
				app_uart_m2m_com.c
#				app_uart_m2m_com_frame.c
				app_uart_m2m_callback.c
		)
	target_sources(app PRIVATE ${app_sources})

endif()
