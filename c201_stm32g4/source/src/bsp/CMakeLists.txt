# Copyright (c) 2021 Acme CPU

add_subdirectory(bsp_battery)
add_subdirectory(bsp_blower)
add_subdirectory(bsp_display)
#add_subdirectory(bsp_fs_helper)
add_subdirectory(bsp_led)
add_subdirectory(bsp_membrane_switch)
add_subdirectory(bsp_sensor)
#add_subdirectory(bsp_usb_mux)
add_subdirectory(bsp_stepper)
add_subdirectory(bsp_buzzer)
#add_subdirectory(bsp_aux_power)
add_subdirectory(bsp_time)
add_subdirectory(bsp_firmware_update)

list(APPEND app_sources 
			bsp_init.c
	)
	
target_sources(app PRIVATE ${app_sources})

