# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_UCPD)
	list(APPEND app_sources 
				app_ucpd.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()


if (CONFIG_APP_UCPD_EXT_CONTR)
	list(APPEND app_sources 
				app_ucpd_ext_contr.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()