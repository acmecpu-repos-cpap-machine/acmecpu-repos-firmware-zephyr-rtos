# Copyright (c) 2021 Acme CPU

if (CONFIG_APP_SHELLCMD)
list(APPEND app_sources 
			app_shellcmd.c
			app_shellcmd_battery.c
			app_shellcmd_sensor.c
			app_shellcmd_pid.c
			app_shellcmd_system.c
			app_shellcmd_heater.c
			app_shellcmd_sensor_print.c
	)
	if (CONFIG_BOARD_E206)		
		list(APPEND app_sources	app_shellcmd_update.c)
	endif()
	if (CONFIG_BOARD_C208T)		
		list(APPEND app_sources	app_shellcmd_update.c)
	endif()
	if (!CONFIG_BOARD_C208T)		
		list(APPEND app_sources	app_shellcmd_stepper.c)
	endif()
	if (CONFIG_APP_BLOWER)
		list(APPEND app_sources app_shellcmd_blower.c)
	endif()
	if (CONFIG_APP_CHARGER)
		list(APPEND app_sources app_shellcmd_charger.c)
	endif()
	if (CONFIG_APP_ANALOG)
		list(APPEND app_sources app_shellcmd_analog.c)
	endif()	
	if (CONFIG_APP_UCPD_EXT_CONTR)
		list(APPEND app_sources app_shellcmd_ucpd.c)
	endif()
	if (CONFIG_APP_TEST_CMDS AND CONFIG_APP_BLOWER)
		list(APPEND app_sources app_shellcmd_test.c)
	endif()
	if (CONFIG_APP_NET)
		list(APPEND app_sources app_shellcmd_net.c)
	endif()
target_sources(app PRIVATE ${app_sources})
endif()
