
set(COMPONENT_SRCS 	"console_cmds.c"
					"cmd_system.c"
					)
					
if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204)					
	list(APPEND COMPONENT_SRCS
					"cmd_stm32_bl.c"
					"stm32_test_fw/stm32g4_test_fw.c"
	)
endif()					
	
set(COMPONENT_ADD_INCLUDEDIRS ". include")

set(COMPONENT_REQUIRES console spi_flash stm32_usart_bl_host driver)
	
register_component()
