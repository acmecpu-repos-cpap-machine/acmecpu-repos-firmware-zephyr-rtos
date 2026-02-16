# Copyright (c) 2023 Acme CPU

#if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204 OR CONFIG_BOARD_E206)

	set(COMPONENT_SRCS 	"comm_wifi.c"
						"app_http_server.c"
									)
									
	set(COMPONENT_ADD_INCLUDEDIRS 	.
									include)
	
#endif()

	idf_component_register(
							SRCS "${COMPONENT_SRCS}"
							
							INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
							
							REQUIRES 	nvs_flash
										host_cmds
										app_events
										esp_wifi
										
							PRIV_REQUIRES
										esp_http_server
										#lwip
										#esp_netif
							)