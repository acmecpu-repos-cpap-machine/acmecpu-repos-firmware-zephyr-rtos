# Copyright (c) 2024 Acme CPU
if (CONFIG_LIB_M2M_FRAME)
    list(APPEND lib_sources 
                lib_m2m_frame.c
        )
    target_sources(app PRIVATE ${lib_sources})
endif()
