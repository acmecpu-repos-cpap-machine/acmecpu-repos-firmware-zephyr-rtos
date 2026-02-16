# Copyright (c) 2021 Acme CPU

if(CONFIG_C20X_SCREENS_TFT)
	FILE(GLOB app_sources img/oled_0_96_128x64/*.c)
	
	list(APPEND app_sources
				c20x_screen_manager.c
				c20x_screen_splash.c
				c20x_screen_dashboard.c
				c20x_screen_statusbar.c
				c20x_screen_settings.c 
#				c20x_screen_switch.c
#				c20x_screen_spinbox.c
				c20x_screen_roller.c
#				c20x_screen_datetime_roller.c
				c20x_screen_dynamic_roller.c
				c20x_screen_common_styles.c
				c20x_screen_alert_msg.c
				c20x_screen_settings_extra_func.c
				c20x_screen_power_key.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()

if(CONFIG_C20X_SCREENS_OLED)
	FILE(GLOB app_sources img/oled_0_96_128x64/*.c)
	
	list(APPEND app_sources
				c20x_screen_manager.c
				c20x_screen_splash.c
				c20x_screen_dashboard.c
				c20x_screen_statusbar.c
				c20x_screen_settings.c 
#				c20x_screen_switch.c
#				c20x_screen_spinbox.c
				c20x_screen_roller.c
#				c20x_screen_datetime_roller.c
				c20x_screen_dynamic_roller.c
				c20x_screen_common_styles.c
				c20x_screen_alert_msg.c
				c20x_screen_settings_extra_func.c
				c20x_screen_power_key.c
#				img/oled_0_96_128x64/batt_25.c
#				img/oled_0_96_128x64/batt_50.c
#				img/oled_0_96_128x64/batt_75.c
#				img/oled_0_96_128x64/batt_100.c
#				img/oled_0_96_128x64/*.c
		)
	target_sources(app PRIVATE ${app_sources})
endif()