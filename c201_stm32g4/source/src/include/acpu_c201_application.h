/*
 * Copyright (c) 2021 Acme CPU
 */



#ifndef SRC_INCLUDE_ACPU_C201_APPLICATION_H_
#define SRC_INCLUDE_ACPU_C201_APPLICATION_H_

typedef enum {
	/* Default state while  power on */
	C201_APP_STATE_BOOTING = 0,

	/* State when the application is initializing */
	C201_APP_STATE_INITIALIZING,

	/* State when application has been initialized,
	 * all threads have been started and are waiting for next action */
	C201_APP_STATE_INITIALIZED,

	/* Initialization failed, the application cannot proceed further */
	C201_APP_STATE_INIT_FAILED,

	/* The main application is reading the saved configuration and
	 * applying the configuration to the relevant modules */
	C201_APP_STATE_CONFIGURING,

	/* The saved configuration has been applied successfully */
	C201_APP_STATE_CONFIGURED,

	/* Configuration failed, the application cannot proceed further */
	C201_APP_STATE_CONFIG_FAILED,

	/* System is running and blower is ON */
	C201_APP_STATE_RUNNING_BLOWER_ON,

	/* System is running and blower is OFF */
	C201_APP_STATE_RUNNING_BLOWER_OFF
} ACPU_C201_APP_STATES;

void app_events_poll();
int app_check_and_connect_to_network();
void app_common_events_register();
int acpu_c201_app_init();

#endif /* SRC_INCLUDE_ACPU_C201_APPLICATION_H_ */
