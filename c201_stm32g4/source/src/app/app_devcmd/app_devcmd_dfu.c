/*
 * Copyright (c) 2021 Acme CPU
 */
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shell/shell.h>
#include <sys/reboot.h>

#include <logging/log.h>
LOG_MODULE_DECLARE(app_devcmd);

#include "app_devcmd/app_devcmd.h"
#include "app_devcmd_packet.h"
#include "app_firmware_update/bsp_firmware_update.h"
#include "lib_events/lib_events.h"

#define CMD_DFU_START			"dfu start"

static int cmd_dfu_start(const struct shell *shell, size_t argc, char **argv) {
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: dfu start");

	/* Put device into bootloader mode. Steps:
	 * 1. TODO Report a system level event for DFU
	 * 2. TODO Wait and check if all threads have stopped
	 * 3. Send response packet
	 * 4. Report reboot event and wait for 'n' secs
	 * 5. Configure device to go into bootloader
	 * 6. Reboot the system
	 * */

	/* TODO 1. Report a system level event for DFU */

	/* TODO 2. Wait and check if all threads have stopped */


	/* 3. Send response packet */
	int ret = 0;
	uint8_t status = DEVCMD_STATUS_OK;
//	if (!ret) status = DEVCMD_STATUS_OK;

	/* make the response packet */
	struct devcmd_packet_t *resp = (struct devcmd_packet_t*) calloc(1, sizeof(struct devcmd_packet_t));
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		return -1;
	}
	app_devcmd_make_packet(resp, DEVCMD_PACKET_SINGLE, 1,
						strlen(CMD_DFU_START),
						CMD_DFU_START, status,
						0, NULL);
	/* make a serialized buffer to send */
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, DEVCMD_PACKET_SIZE_MAX+1);
	if (resp == NULL) {
		LOG_ERR("%s calloc failed!", (__func__));
		free(resp);
		return -1;
	}
	uint32_t sdata_len=0;
	ret = app_devcmd_serialize_packet(serialized_buffer, (DEVCMD_PACKET_SIZE_MAX+1), resp, &sdata_len);

	/* transmit the serialized buffer */
	ret = app_devcmd_transmit_data(serialized_buffer, sdata_len);

	free(resp);
	free(serialized_buffer);

	/* 4. Report reboot event and wait for 'n' secs */
	lib_events_report_event(LIB_EVENT_POWER_OFF);
	k_sleep(K_MSEC(1000));

	/* 5. Configure device to go into bootloader */
	bsp_fwupdate_config_bootloader();

	/* 6. Reboot the system */
	sys_reboot(SYS_REBOOT_WARM);

	return ret;
}


/* blower */
SHELL_STATIC_SUBCMD_SET_CREATE(dfu_subcmds,
		SHELL_CMD(start, NULL, "Put processor in bootloader mode for firmware upgrade", cmd_dfu_start),
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(dfu, &dfu_subcmds, "DFU commands", NULL);
