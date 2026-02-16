# Copyright (c) 2024 Acme CPU

if (CONFIG_BMI270)
	list(APPEND app_sources 
				app_imu.c
				bmi2.c
				bmi270_legacy.c
				common.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()