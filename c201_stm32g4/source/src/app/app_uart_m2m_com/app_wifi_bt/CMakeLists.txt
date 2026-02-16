# Copyright (c) 2022 Acme CPU

if (CONFIG_APP_WIFI_BT)

	list(APPEND app_sources 
				app_wifi_bt.c
				app_wifi_bt_cmds.c
		)
	target_sources(app PRIVATE ${app_sources})

endif()
