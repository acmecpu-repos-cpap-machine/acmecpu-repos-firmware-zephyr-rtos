# Copyright (c) 2024 Acme CPU

#if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204 OR CONFIG_BOARD_E206)

	set(COMPONENT_SRCS 	"app_net_file_download.c"
									)
									
	set(COMPONENT_ADD_INCLUDEDIRS 	.
									include)
	
#endif()

	idf_component_register(
							SRCS "${COMPONENT_SRCS}"
							INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
							REQUIRES 			app_events
							PRIV_REQUIRES		esp_http_client
												host_cmds
							)