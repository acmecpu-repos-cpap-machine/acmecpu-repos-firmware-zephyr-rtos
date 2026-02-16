/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 24-Jun-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_AUDIO_APP_AUDIO_NEW_H_
#define SRC_INCLUDE_APP_AUDIO_APP_AUDIO_NEW_H_

/**
 * @brief   Starts recording audio 
 *          The recording is saved to a file
 * 
 * @param   record_file   file name with full path
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int app_audio_record(const char* record_file);

/**
 * @brief   Playback audio from file
 * 
 * @param   playback_file   file name with full path
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int app_audio_playback(const char* playback_file);

/**
 * @brief Initializes the audio hardware and applies default settings
 * 
 * @return  0       Success
 *          -ve     Fail
*/
int app_audio_init();

#endif /* SRC_INCLUDE_APP_AUDIO_APP_AUDIO_NEW_H_ */