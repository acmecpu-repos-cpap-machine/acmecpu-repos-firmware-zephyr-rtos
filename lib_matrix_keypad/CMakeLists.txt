# Copyright (c) 2023 Acme CPU
if (CONFIG_LIB_MATRIX_KEYPAD)
    list(APPEND lib_sources 
                lib_matrix_keypad.c
        )
    target_sources(app PRIVATE ${lib_sources})
endif()