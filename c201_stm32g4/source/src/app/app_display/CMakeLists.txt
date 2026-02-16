# Copyright (c) 2021 Acme CPU

if(CONFIG_APP_DISPLAY)

	add_subdirectory(c20x_screens)

	list(APPEND app_sources 
				app_display.c
				app_display_menu.c
		)
	target_sources(app PRIVATE ${app_sources})

	target_include_directories(app PRIVATE c20x_screens)

endif()