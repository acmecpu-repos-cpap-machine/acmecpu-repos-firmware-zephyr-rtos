# SPDX-License-Identifier: Apache-2.0

if(CONFIG_STM32G4UCPD)
  add_compile_definitions(USE_FULL_LL_DRIVER)
  add_compile_definitions(USBPD_PORT_COUNT=1)
  add_compile_definitions(_SNK)
  add_compile_definitions(USBPDCORE_LIB_PD3_CONFIG_MINSNK)
  
  
  zephyr_include_directories(	./
  								Inc
  								STM32_USBPD_Library/Core/inc
  								STM32_USBPD_Library/Devices/STM32G4XX/inc
  								)
  								
  zephyr_library()
  
  zephyr_library_sources(		stm32g4_ucpd.c
  								#STM32_USBPD_Library/Core/src/usbpd_dpm_core.c
  								STM32_USBPD_Library/Core/src/usbpd_trace.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_cad_hw_if.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_hw_if_it.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_hw.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_phy_hw_if.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_phy.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_pwr_hw_if.c
  								STM32_USBPD_Library/Devices/STM32G4XX/src/usbpd_timersserver.c
  								Src/usbpd_dpm_core.c
  								Src/usbpd_dpm_user.c
  								Src/usbpd_pwr_if.c
  								Src/usbpd_pwr_user.c
  								#Src/usbpd_vdm_user.c
  								Src/usbpd.c
  								)
  								
  target_link_libraries(${ZEPHYR_CURRENT_LIBRARY} PUBLIC 
  								${CMAKE_CURRENT_SOURCE_DIR}/STM32_USBPD_Library/Core/lib/USBPDCORE_PD3_CONFIG_MINSNK_CM4_wc32.a
  						)
endif()
