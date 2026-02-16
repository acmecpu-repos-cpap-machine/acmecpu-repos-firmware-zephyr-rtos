# Copyright (c) 2021 Acme CPU

# Add our modules directory to Zephyr Modules
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules)

cmake_minimum_required(VERSION 3.20.1)

set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dts)
#set(DTC_OVERLAY_FILE "boards/stm32g473_acme_cpu_c201.overlay")
#set(DTC_OVERLAY_FILE "boards/stm32g473_acme_cpu_c201_oled.overlay")
#set(DTC_OVERLAY_FILE "boards/c204_core.overlay")

#set(DTC_OVERLAY_FILE "boards/stm32g473_acme_cpu_c201.overlay application/ui_ux/ui_menu.config")

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
#set(BOARD stm32g473_acme_cpu_c201)
#set(BOARD stm32g473_acme_cpu_c201_oled)
#set(BOARD c204_core)
#set(BOARD c205)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(acme_cpu_c201)

#FILE(GLOB app_sources 
#			src/*.c
#			src/application/acpu_c201_bsp/*.c)
#target_sources(app PRIVATE ${app_sources})

#target_include_directories(app PRIVATE src/application/acpu_c201_bsp)

add_subdirectory(src)
