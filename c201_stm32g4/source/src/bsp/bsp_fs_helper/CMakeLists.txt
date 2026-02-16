# Copyright (c) 2021 Acme CPU

list(APPEND app_sources 
			bsp_fs_helper.c
			bsp_fs_log_mgmt.c
	)
target_sources(app PRIVATE ${app_sources})
