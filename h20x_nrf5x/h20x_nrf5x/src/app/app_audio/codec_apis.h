/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef SRC_INCLUDE_APP_AUDIO_CODEC_APIS_H_
#define SRC_INCLUDE_APP_AUDIO_CODEC_APIS_H_


int     CMX655OpenGpio(void);
void    CMX655HardReset(void);

void    CMX655SetCsLow(void);
void    CMX655SetCsSpi(void);
void    CMX655ResetZ(void);
void    CMX655ResetHi(void);

int     CMX655Clock(int);

int     CMX655GetIsr(void);

int     CMX655SetNdiv(int);
int     CMX655GetNdiv(void);
int     CMX655SetRdiv(int);
int     CMX655GetRdiv(void);

int     CMX655GetLFilt(void);
int     CMX655SetLFilt(int);
int     CMX655GetCpi(void);
int     CMX655SetCpi(int);
int     CMX655GetPllCtrl(void);
int     CMX655SetPllCtrl(int);
 
int     CMX655GetSysCtrl(void);
int     CMX655SetSysCtrl(int);
int     CMX655SetBitsSysCtrl(int);
int     CMX655ClrBitsSysCtrl(int);

int     CMX655SetPlayPreamp(int);

int     CMX655GetPreampGain(void);
int     CMX655SetPreampGain(int);
int     CMX655GetNoiseGateAtten(int *);
int     CMX655GetAlcAtten(void);

int     CMX655GetSai(void);
int     CMX655SetSai(int);
int     CMX655GetLevel(int *,int *);
int     CMX655SetLevel(int,int);
int     CMX655GetRvf(void);
int     CMX655GetPvf(void);
int     CMX655SetRvf(int);
int     CMX655SetPvf(int);

int     CMX655GetSaiM(void);
int     CMX655SetSaiM(int);

int     CMX655GetVolume(void);
int     CMX655SetVolume(int);
int     CMX655GetVolSmooth(void);
int     CMX655SetVolSmooth(int);

int     CMX655GetClkSrc(void);
int     CMX655SetClkSrc(int);
int     CMX655GetSampleRate(void);
int     CMX655SetSampleRate(int);

int     CMX655Command(int);

void codec_print_regs();
int codec_prepare();
int codec_startclk();
int codec_stopclk();
int codec_pa_disable();
int codec_pa_enable();
int codec_mic_pa_enable();
int codec_enable_streams();
int codec_init(const struct device *dev, int n_chan, int s_rate);
int codec_clock_setup(const struct device *dev, int n_chan, int s_rate);

#endif /* SRC_INCLUDE_APP_AUDIO_CODEC_APIS_H_ */
