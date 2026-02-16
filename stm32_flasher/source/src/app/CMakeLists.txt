# Copyright (c) 2023 Acme CPU

add_subdirectory(app_shellcmd)
add_subdirectory(app_storage)
add_subdirectory(app_uart)

target_sources(app PRIVATE ${app_sources})
