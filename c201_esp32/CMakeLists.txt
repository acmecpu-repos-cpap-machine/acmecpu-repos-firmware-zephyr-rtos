# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# The PROJECT_VER must be changed each time a new functionality is developed
# The OTA update will not work if the version is not changed.
# see implementation of function cb_fwapp_get_handler() in host_cmds_fw_update.c
set(PROJECT_VER "0.0.0.12")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(c201_esp32)
