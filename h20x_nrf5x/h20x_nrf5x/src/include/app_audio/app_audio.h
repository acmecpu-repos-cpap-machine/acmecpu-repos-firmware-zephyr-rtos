/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 25-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_
#define SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_

typedef enum {
    AUDIO_STATE_STOP = 0,
    AUDIO_STATE_RECORD,
    AUDIO_STATE_PLAYBACK,
} AUDIO_STATES;

int app_audio_record_start(const char* fname, uint32_t fsize_max);
int app_audio_playback_start(const char* fname);
int app_audio_stop();
int app_audio_init();
int app_audio_volume_set(uint32_t volume);
int app_audio_printfile(const char* fname);

#endif /* SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_ */