/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_TS3USBCA4_TS3USBCA4_H_
#define MODULES_TS3USBCA4_TS3USBCA4_H_


typedef int (*ts3usbca4_select_chan_t)(const struct device *, uint8_t);

struct ts3usbca4_driver_api {
	ts3usbca4_select_chan_t select_channel;
};

#endif /* MODULES_TS3USBCA4_TS3USBCA4_H_ */
