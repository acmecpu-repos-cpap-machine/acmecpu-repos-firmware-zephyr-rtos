# Copyright (c) 2022 Acme CPU

if (CONFIG_BMA253)
	zephyr_include_directories(./)
	zephyr_library()
	zephyr_library_sources(bma253.c)
	zephyr_library_sources_ifdef(CONFIG_BMA280_TRIGGER bma253_trigger.c)
endif()