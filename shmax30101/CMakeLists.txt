# Copyright (c) 2022 Acme CPU

if(CONFIG_SHMAX30101)
  zephyr_include_directories(./ ./Utilities)
  zephyr_library()
  zephyr_library_sources(shmax30101.c SSInterface.c SSMAX30101Comm.c)
  zephyr_library_sources(Utilities/queue.c Utilities/utils.c Utilities/CRC8.c)
endif()
