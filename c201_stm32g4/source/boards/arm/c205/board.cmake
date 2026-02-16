# SPDX-License-Identifier: Apache-2.0

#board_runner_args(jlink "--device=STM32G473RE" "--speed=4000")

#include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
#include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

board_runner_args(pyocd "--target=stm32g473qetx")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

