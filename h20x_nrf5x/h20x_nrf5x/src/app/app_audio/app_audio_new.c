/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 23-Jun-2023
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#define ONLY_PLAYBACK	0

#include <zephyr/kernel.h>
#include <stdio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_audio);

#include "lib_audio/lib_audio.h"

int m_audio_state = -1;

static void audio_cb(LIB_AUDIO_EVENT event)
{
	m_audio_state = event;
}

int app_audio_record(const char* record_file)
{
    LOG_INF("Recording to %s ...", record_file);
    int ret = lib_audio_record(record_file);
    if (ret < 0) {
        LOG_ERR("could not record, %d", ret);
    }
    return ret;
}

int app_audio_playback(const char* playback_file)
{
	LOG_INF("Playing from %s ...", playback_file);
	int ret = lib_audio_playback(playback_file);
    if (ret < 0) {
        LOG_ERR("could not record, %d", ret);
    }
    return ret;
}

int app_audio_init()
{
    int ret = 0;
	/* init audio layer */
	ret = lib_audio_init((LIB_AUDIO_RECORD | LIB_AUDIO_PLAYBACK), audio_cb);
	ret = lib_audio_start();
	
	/* allow threads to start */
	k_sleep(K_MSEC(100));

    return ret;
}
