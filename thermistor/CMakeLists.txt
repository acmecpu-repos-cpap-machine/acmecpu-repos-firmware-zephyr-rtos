# Copyright (c) 2024 Acme CPU

if (CONFIG_NTC_THERMISTOR)
	zephyr_include_directories(./)
	zephyr_library()
	zephyr_library_sources(thermistor.c)
	zephyr_library_sources(thermistor_calc.c)
endif()
