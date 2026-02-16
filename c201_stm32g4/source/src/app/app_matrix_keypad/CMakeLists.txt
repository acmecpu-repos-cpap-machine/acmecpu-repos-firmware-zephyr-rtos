# Copyright (c) 2023 Acme CPU

if (CONFIG_APP_MATRIX_KEYPAD)
	list(APPEND app_sources 
				app_matrix_keypad.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()