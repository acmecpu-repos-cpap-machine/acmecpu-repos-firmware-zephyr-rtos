# Copyright (c) 2022 Acme CPU
if (CONFIG_LIB_PUSH_SWITCH)
    list(APPEND lib_sources 
                lib_push_switch.c
        )
    target_sources(app PRIVATE ${lib_sources})
endif()