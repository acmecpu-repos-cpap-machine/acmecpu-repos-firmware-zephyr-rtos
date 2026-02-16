/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 25-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_
#define SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_

#include <stdint.h>

typedef enum {
    AUDIO_STATE_STOP = 0,
    AUDIO_STATE_RECORD,
    AUDIO_STATE_PLAYBACK,
} AUDIO_STATES;


// WAV header spec information:
//https://web.archive.org/web/20140327141505/https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
//http://www.topherlee.com/software/pcm-tut-wavformat.html
struct __attribute__((__packed__)) wav_header {
    // RIFF Header
    char riff_header[4]; // Contains "RIFF"
    int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
    char wave_header[4]; // Contains "WAVE"
    
    // Format Header
    char fmt_header[4]; // Contains "fmt " (includes trailing space)
    int fmt_chunk_size; // Should be 16 for PCM
    short audio_format; // Should be 1 for PCM. 3 for IEEE Float
    short num_channels;
    int sample_rate;
    int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
    short sample_alignment; // num_channels * Bytes Per Sample
    short bit_depth; // Number of bits per sample
    
    // Data
    char data_header[4]; // Contains "data"
    int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
    // uint8_t bytes[]; // Remainder of wave file is bytes
};



int app_audio_record_start(const char* fname, uint32_t fsize_max);
int app_audio_playback_start(const char* fname);
int app_audio_stop();
int app_audio_init();
int app_audio_volume_set(uint32_t volume);
int app_audio_printfile(const char* fname);

#endif /* SRC_INCLUDE_APP_AUDIO_APP_AUDIO_H_ */