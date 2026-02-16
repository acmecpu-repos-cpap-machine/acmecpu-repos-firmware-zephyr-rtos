# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_UTILS)
		list(APPEND app_sources 
					app_utils.c
			)

	if (CONFIG_APP_MENU_SORT_UTIL)
		list(APPEND app_sources app_menu_util.c)
	endif()
	
	target_sources(app PRIVATE ${app_sources})
endif()