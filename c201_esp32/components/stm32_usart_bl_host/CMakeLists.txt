
if(CONFIG_BOARD_C201 OR CONFIG_BOARD_C204)
set(COMPONENT_SRCS 	"stm32_usart_bl_host.c")

set(COMPONENT_ADD_INCLUDEDIRS ". include")

endif()
set(COMPONENT_REQUIRES generic_linked_lists)

	register_component()
