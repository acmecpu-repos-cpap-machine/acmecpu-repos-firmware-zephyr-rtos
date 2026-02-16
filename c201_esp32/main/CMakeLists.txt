
#if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204)
	set(COMPONENT_SRCS 	"main.c"
						"c201_esp32_application.c"
								)
								
	set(COMPONENT_ADD_INCLUDEDIRS ". include")

	set(COMPONENT_REQUIRES 	host_cmds 
							comm_ble 
							app_events 
							stm32_usart_bl_host 
							console_cmds
							comm_wifi
							app_net
							app_update
							)
	register_component()
#endif()

#if(CONFIG_BOARD_H205C)
#	set(COMPONENT_SRCS 	"main.c"
#						"c201_esp32_application.c"
#								)
#								
#	set(COMPONENT_ADD_INCLUDEDIRS ". include")
#
#	set(COMPONENT_REQUIRES 	host_cmds 
##							comm_ble 
#							app_events 
##							stm32_usart_bl_host 
#							console_cmds
#							)
#	register_component()
#endif()