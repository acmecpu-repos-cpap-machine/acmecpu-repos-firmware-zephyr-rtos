# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_LED_NOTIFICATION)
	list(APPEND app_sources 
				app_led.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()