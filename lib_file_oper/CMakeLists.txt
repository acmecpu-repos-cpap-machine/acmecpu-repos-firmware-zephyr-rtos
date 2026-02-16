# Copyright (c) 2023 Acme CPU

if (CONFIG_LIB_FILE_OPER)
	list(APPEND lib_sources 
				lib_file_oper.c
				lib_file_oper_helper.c
				lib_file_oper_log_mgmt.c
		)
	target_sources(app PRIVATE ${lib_sources})
endif()