/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 30-May-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_LIB_LIB_AUDIO_LIB_AUDIO_H_
#define SRC_LIB_LIB_AUDIO_LIB_AUDIO_H_

#include <stdint.h>

#define LIB_AUDIO_RECORD            0b00000001
#define LIB_AUDIO_PLAYBACK          0b00000010
#define LIB_AUDIO_PLAYBACK_LOUT     0b00000100

typedef enum {
    LIB_AUDIO_EVENT_INIT_DONE = 0,
    LIB_AUDIO_EVENT_RECORD_STARTED,
    LIB_AUDIO_EVENT_RECORD_COMPLETE,
    LIB_AUDIO_EVENT_RECORD_ERROR,
    LIB_AUDIO_EVENT_PLAYBACK_STARTED,
    LIB_AUDIO_EVENT_PLAYBACK_COMPLETE,
    LIB_AUDIO_EVENT_PLAYBACK_ERROR,

    LIB_AUDIO_EVENT_MAX
} LIB_AUDIO_EVENT;

typedef void (*lib_audio_cb)(LIB_AUDIO_EVENT event);

/**
 * @brief Initializes the audio hardware and applies default settings
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_init(uint8_t rec_pb, lib_audio_cb user_cb);


/**
 * @brief   Starts the audio processing thread. This does not start recording or playback but
 *          does necessary initialization to be able to record or playback
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_start();

/**
 * @brief   Starts recording audio till a particular time, after which the recording stops
 *          The recording is saved to a file
 * 
 * @param   fname   file name with full path
 * @param   secs    number of seconds to record
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_record_till_time(const char* fname, uint32_t secs);

/**
 * @brief   Starts recording audio 
 *          The recording is saved to a file
 * 
 * @param   fname   file name with full path
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_record(const char* fname);

/**
 * @brief   Playback audio from file
 * 
 * @param   fname   file name with full path
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_playback(const char* fname);

/**
 * @brief   Stop recording or playback
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_stop();

/**
 * @brief   Change playback volume
 * 
 * @param   volume   0 to 100
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_playback_volume_change(uint32_t volume);

/**
 * @brief   Change recording gain
 * 
 * @param   gain   0 to 100
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int lib_audio_record_gain_change(uint32_t gain);

#endif /* SRC_LIB_LIB_AUDIO_LIB_AUDIO_H_ */