# Copyright (c) 2023 Acme CPU

# Add our modules directory to Zephyr Modules
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/modules)

cmake_minimum_required(VERSION 3.20.1)

set(PROJ_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/)
set(DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/dts)

set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(stm32_flasher)

add_subdirectory(src)
