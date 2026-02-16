# Copyright (c) 2021 Acme CPU

cmake_minimum_required(VERSION 3.20.0)

# Add our modules directory to Zephyr Modules
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules)

# Add our boards directory to Zephyr Boards
#list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/boards)

set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dts)

# H205C-MAIN board secure domain
set(DTC_OVERLAY_FILE "boards/h205c_nrf5340_cpuapp.overlay")
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(BOARD h205c_nrf5340_cpuapp)

# H205C-MAIN board non-secure domain
#set(DTC_OVERLAY_FILE "boards/h205c_nrf5340_cpuapp_ns.overlay")
#set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
#set(BOARD h205c_nrf5340_cpuapp_ns)

# nRF5340-DK board
#set(DTC_OVERLAY_FILE "boards/nrf5340dk_nrf5340_cpuapp.overlay")
#set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})
#set(BOARD nrf5340dk_nrf5340_cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(h20x_nrf5x)

#target_sources(app PRIVATE src/main.c)
add_subdirectory(src)