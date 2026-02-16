# Copyright (c) 2024 Acme CPU

set(SRCS_LIB_M2M_FRAME 			"lib_m2m_frame/lib_m2m_frame.c")
									
set(COMPONENT_ADD_INCLUDEDIRS 	.)
set(INCLUDEDIRS_LIB_M2M_FRAME 	lib_m2m_frame)

idf_component_register(
							SRCS "${SRCS_LIB_M2M_FRAME}"
							INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
							INCLUDE_DIRS "${INCLUDEDIRS_LIB_M2M_FRAME}"
							
#							REQUIRES 			app_events
#							PRIV_REQUIRES		esp_http_client
#												host_cmds
							)