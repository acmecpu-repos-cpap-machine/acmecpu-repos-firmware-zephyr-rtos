# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_NET)
	list(APPEND app_sources 
				app_net.c
				app_net_data_util.c
		)
	
	if (CONFIG_APP_HAS_HTML_GEN)
		list(APPEND app_sources
				app_net_html_gen.c
				app_net_html_menu.c
		)
	endif()
	
	target_sources(app PRIVATE ${app_sources})
endif()