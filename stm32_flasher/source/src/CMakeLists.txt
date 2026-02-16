# Copyright (c) 2023 Acme CPU

add_subdirectory(app)
add_subdirectory(lib)

list(APPEND app_sources 
			main.c
	)
	
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE include)
target_include_directories(app PRIVATE lib)
