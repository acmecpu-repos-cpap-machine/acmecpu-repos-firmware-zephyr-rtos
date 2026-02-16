# Copyright (c) 2021 Acme CPU

if (NOT CONFIG_BOARD_NRF5340DK_NRF5340_CPUAPP)
    add_subdirectory(app_utils)
    add_subdirectory(app_audio)
    add_subdirectory(app_storage)
endif()
add_subdirectory(app_shellcmd)
add_subdirectory(app_sensor)
add_subdirectory(app_analog)
add_subdirectory(app_uart)
add_subdirectory(app_haptic)
add_subdirectory(app_battery)
add_subdirectory(app_push_switch)

#list(APPEND app_sources 
#			main.c
#	)
	
#target_sources(app PRIVATE ${app_sources})

#target_include_directories(app PRIVATE include)
#target_include_directories(app PRIVATE lib)