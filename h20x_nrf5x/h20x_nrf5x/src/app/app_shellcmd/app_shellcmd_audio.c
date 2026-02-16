/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 1-Nov-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */


// #include <zephyr.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_shellcmd);
#include "app_shellcmd/app_shellcmd.h"

// #include "app_audio/app_audio.h"
// #include "app_audio/app_audio_new.h"
#include "lib_audio/lib_audio.h"
#include "app_storage/app_storage.h"

static int audio_record(const struct shell *shell, size_t argc, char **argv)
{

	if (argc != 3) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: audio %s %s %s", (argv[0]), (argv[1]), (argv[2]));
    char fname1[255];
    snprintf(fname1, sizeof(fname1), "%s/%s", app_storage_mp_get(), argv[1]);

    uint32_t fsize_max = strtol(argv[2], NULL, 10);
    // if (app_audio_record_start(fname1, fsize_max) < 0) {
	if (lib_audio_record(fname1) < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }

	return 0;
}

static int audio_playback(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: audio %s %s", (argv[0]), (argv[1]));

    char fname1[255];
    snprintf(fname1, sizeof(fname1), "%s/%s", app_storage_mp_get(), argv[1]);

    // if (app_audio_playback_start(fname1) < 0) {
	if (lib_audio_playback(fname1) < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }
	return 0;
}

static int audio_stop(const struct shell *shell, size_t argc, char **argv)
{

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

    // if (app_audio_stop() < 0) {
	if (lib_audio_stop() < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }

	return 0;
}

static int playback_vol(const struct shell *shell, size_t argc, char **argv)
{

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: audio %s %s", (argv[0]), (argv[1]));

	uint32_t volume = strtol(argv[1], NULL, 10);
    // if (app_audio_volume_set(volume) < 0) {
	if (lib_audio_playback_volume_change(volume) < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }
	return 0;
}

static int record_gain(const struct shell *shell, size_t argc, char **argv)
{

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_INF("received cmd: audio %s %s", (argv[0]), (argv[1]));

	uint32_t gain = strtol(argv[1], NULL, 10);
	if (lib_audio_record_gain_change(gain) < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }
	return 0;
}
#if 0
static int audio_printfile(const struct shell *shell, size_t argc, char **argv) {

	if (argc != 2) {
		LOG_ERR("incorrect number of arguments");
		return -EINVAL;
	}

	/* Log messages to be printed on the console */
	LOG_DBG("received cmd: audio %s %s", (argv[0]), (argv[1]));

    char fname1[255];
    snprintf(fname1, sizeof(fname1), "%s/%s", app_storage_mp_get(), argv[1]);

    if (app_audio_printfile(fname1) < 0) {
        shell_print(shell, MSG_FAIL);
    } else {
        shell_print(shell, MSG_PASS);    
    }
	return 0;
}
#endif

/* audio */
SHELL_STATIC_SUBCMD_SET_CREATE(audio_subcmds,
		SHELL_CMD_ARG(record, NULL, "record", audio_record, 3, 0),
		SHELL_CMD_ARG(play, NULL, "playback from file", audio_playback, 2, 0),
        SHELL_CMD_ARG(stop, NULL, "stop recording or playback", audio_stop, 0, 0),
		SHELL_CMD_ARG(volume, NULL, "set playback volume", playback_vol, 2, 0),
		SHELL_CMD_ARG(gain, NULL, "set recording gain", record_gain, 2, 0),
#if 0
		SHELL_CMD_ARG(printfile, NULL, "print recorded file", audio_printfile, 2, 0),
#endif	
		SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(audio, &audio_subcmds, "Audio commands", NULL);
