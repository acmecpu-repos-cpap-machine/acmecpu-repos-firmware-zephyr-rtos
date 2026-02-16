# Copyright (c) 2022 Acme CPU

if ((NOT CONFIG_BLOWER_MOTOR_A2) AND CONFIG_APP_BLDC_DRV)

	list(APPEND app_sources 
				app_bldc_drv.c
				app_bldc_drv_cmds.c
		)
	target_sources(app PRIVATE ${app_sources})

endif()
