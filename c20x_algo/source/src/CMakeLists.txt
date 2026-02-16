list(APPEND app_sources 
			c20x_algo.cpp
			PressureFlowRate.cpp
			Breath.cpp
	)

target_sources(app PUBLIC ${app_sources})

target_include_directories(app PUBLIC .)

