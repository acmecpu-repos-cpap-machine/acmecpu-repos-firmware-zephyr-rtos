#set(COMPONENT_SRCS 	"comm_ble_blower_device_profile.c"
#					"comm_ble_device_info_service.c"
#					"comm_ble_battery_service.c"
#					"comm_ble_blower_service.c"
#					"comm_ble_dfu_service.c"
#					"comm_ble_stepper_service.c"
#					"comm_ble_settings_service.c"
#					"comm_ble_sensor_service.c"
#								)
#set(COMPONENT_ADD_INCLUDEDIRS ". include")

#set(COMPONENT_REQUIRES bt nvs_flash host_cmds app_events stm32_usart_bl_host)

#register_component()


if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204 OR CONFIG_BOARD_E206)

	set(COMPONENT_SRCS 	"comm_ble_blower_device_profile.c"
						"comm_ble_device_info_service.c"
						"comm_ble_battery_service.c"
						"comm_ble_blower_service.c"
						"comm_ble_dfu_service.c"
						"comm_ble_stepper_service.c"
						"comm_ble_settings_service.c"
						"comm_ble_sensor_service.c"
									)
									
	set(COMPONENT_ADD_INCLUDEDIRS 	.
									include)
	
endif()

	idf_component_register(
							SRCS "${COMPONENT_SRCS}"
							INCLUDE_DIRS "${COMPONENT_ADD_INCLUDEDIRS}"
							REQUIRES bt nvs_flash host_cmds app_events stm32_usart_bl_host
#							REQUIRED_IDF_TARGETS esp32
							)