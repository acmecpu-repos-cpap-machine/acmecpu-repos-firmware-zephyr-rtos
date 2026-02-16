# Copyright (c) 2024 Acme CPU
# Author: Rohan Dey (rohan@acmecpu.com)

if (CONFIG_APP_DFU)
	list(APPEND app_sources 
				app_dfu.c
				app_file_download.c
				app_dfu_stm32bl.c
				app_dfu_stm32bl_uart.c
				app_dfu_netproc_update.c
		)
		
	target_sources(app PRIVATE ${app_sources})
endif()