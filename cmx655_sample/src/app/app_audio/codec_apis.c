/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 31-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_audio);

#include "codec_apis.h"

#if CONFIG_CMX655D
#include "cmx655d.h"
#endif

#define CMX655D_NODE DT_NODELABEL(cmx655d)
#if DT_ON_BUS(CMX655D_NODE, i2c)
#define CMX655D_I2C_NODE DT_BUS(CMX655D_NODE)
#endif

static const struct device *codec_dev;
static const struct cmx655d_driver_api *api;

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the IRQ status value
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetIsr(void)
{   
    return (api->param_get(codec_dev, CMX655_ISR));
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the reference divider
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetRdiv(void)
{   int     iStatus;
    int     iData;
    
    iStatus = api->param_get(codec_dev, CMX655_RDIVHI);
    iData = iStatus << 8;
    iStatus = api->param_get(codec_dev, CMX655_RDIVLO);
    iData |= iStatus & 0xff;
    iStatus=iData;
    
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the feedback divider
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetNdiv(void)
{   int     iStatus;
    int     iData;
    
    iStatus = api->param_get(codec_dev, CMX655_NDIVHI);
    iData = iStatus << 8;
    iStatus = api->param_get(codec_dev, CMX655_NDIVLO);
    iData |= iStatus & 0xff;
    iStatus=iData;
    
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the loop filter resistor value 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetLFilt(void)
{   int     iStatus;
    
    iStatus = api->param_get(codec_dev, CMX655_PLLCTRL);
    iStatus = (iStatus & CMX655_PLLCTRL_LFILT_MASK) >> CMX655_PLLCTRL_LFILT_SHIFT;
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the charge pump current gain value 
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetCpi(void)
{   int     iStatus;
    
    iStatus = api->param_get(codec_dev, CMX655_PLLCTRL);
    iStatus = (iStatus & CMX655_PLLCTRL_CPI_MASK) >> CMX655_PLLCTRL_CPI_SHIFT;
    
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current PLLCTRL settings : -1 on error
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetPllCtrl(void)
{   int     iPllCtrl;
    
    iPllCtrl = api->param_get(codec_dev, CMX655_PLLCTRL);
    return(iPllCtrl);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current SYSCTRL settings : -1 on error
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetSysCtrl(void)
{   int     iSysCtrl;
    iSysCtrl = api->param_get(codec_dev, CMX655_SYSCTRL);
    return(iSysCtrl);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Deal with the SAI
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetSai(void)
{   
    int     iSai;
    iSai = api->param_get(codec_dev, CMX655_SAICTRL);
    return(iSai);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Deal with the SAIMUX
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetSaiM(void)
{   
    int     iSaiM;
    iSaiM = api->param_get(codec_dev, CMX655_SAIMUX);
    return(iSaiM);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Deal with the LEVEL register
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int     CMX655GetLevel(int* piLeft, int* piRight)
{
    int     iStatus;
    iStatus = api->param_get(codec_dev, CMX655_LEVEL);
    if(piLeft!=NULL)
    {   *piLeft = (iStatus & CMX655_LEVEL_GL_MASK) >> CMX655_LEVEL_GL_SHIFT;
    }
    if(piRight!=NULL)
    {   *piRight = (iStatus & CMX655_LEVEL_GR_MASK) >> CMX655_LEVEL_GR_SHIFT;
    }
    
    return(iStatus);
}

int CMX655SetLevel(int iLeft, int iRight)
{
    int     iStatus;
    int     iLevel;

    iLevel = (((iLeft  & CMX655_LEVEL_GL_VALUE) << CMX655_LEVEL_GL_SHIFT) |
              ((iRight & CMX655_LEVEL_GR_VALUE) << CMX655_LEVEL_GR_SHIFT));
    
    if((iStatus = api->param_set(codec_dev, CMX655_LEVEL, (uint8_t)iLevel)) < 0)
    {
        LOG_ERR("Problem writing LEVEL");
    }
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Deal with the filter registers
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetRvf()
{   int     iStatus;
    iStatus = api->param_get(codec_dev, CMX655_RVF);
    return(iStatus);
}

int CMX655GetPvf()
{   int     iStatus;
    iStatus = api->param_get(codec_dev, CMX655_PVF);
    return(iStatus);
}

int CMX655SetRvf(int iFilt)
{   
    int     iStatus;

    if((iStatus = api->param_set(codec_dev, CMX655_RVF, (uint8_t)iFilt)) < 0)
    {  
        LOG_ERR("Problem writing RVF\n");
    }
    return(iStatus);
}

int     CMX655SetPvf(int iFilt)
{   int     iStatus;

    if((iStatus = api->param_set(codec_dev, CMX655_PVF, (uint8_t)iFilt)) < 0)
    {  
        LOG_ERR("Problem writing PVF\n");
    }
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current playback volume setting
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetVolume(void)
{   int     iVolume;
    iVolume = api->param_get(codec_dev, CMX655_VOLUME);
    iVolume=((iVolume & CMX655_VOL_MASK) >> CMX655_VOL_SHIFT );
    return(iVolume);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Set the playback volume
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655SetVolume(int iVolume)
{
    int     iStatus;

    if ((iStatus = api->param_get(codec_dev, CMX655_VOLUME)) >= 0) {
        iStatus &= CMX655_VOL_SMOOTH_MASK;
        iStatus |= ((iVolume & CMX655_VOL_VALUE) << CMX655_VOL_SHIFT);
        if((iStatus = api->param_set(codec_dev, CMX655_VOLUME, (uint8_t)iStatus)) < 0) {
            LOG_ERR("Problem writing VOLUME\n");
        }
    }

    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current smoothing flag
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655GetVolSmooth(void)
{   int     iSmooth;
    iSmooth = api->param_get(codec_dev, CMX655_VOLUME);
    iSmooth = (iSmooth & CMX655_VOL_SMOOTH_MASK) >> CMX655_VOL_SMOOTH_SHIFT;
    return(iSmooth);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Set the current smoothing flag
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int CMX655SetVolSmooth(int iSmooth)
{
    int     iStatus;

    if((iStatus = api->param_get(codec_dev, CMX655_VOLUME)) < 0) {
        LOG_ERR("Problem reading VOLUME\n");
    } else {
        if(iSmooth) {
            iStatus |= CMX655_VOL_SMOOTH_MASK;
        } else {
            iStatus &= ~(CMX655_VOL_SMOOTH_MASK);
        }
        if((iStatus = api->param_set(codec_dev, CMX655_VOLUME, (uint8_t)iStatus)) < 0) {
            LOG_ERR("Problem writing VOLUME\n");
        }
    }
    return(iStatus);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current setup for the internal clock source
+       return CLKCTRL_PLL_RCLK, CLKCTRL_PLL_LRCLK, CLKCTRL_LPO, CLKCTRL_RCLK
+           or -1 on error
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int     CMX655GetClkSrc(void)

{   int     iClkSrc;
    iClkSrc = api->param_get(codec_dev, CMX655_CLKCTRL);
    iClkSrc = iClkSrc & (CMX655_CLKCTRL_CLKSEL | CMX655_CLKCTRL_PLLSEL | CMX655_CLKCTRL_PLLREF);
    switch (iClkSrc)
    {   case (CMX655_CLKCTRL_CLKSEL | CMX655_CLKCTRL_PLLSEL | CMX655_CLKCTRL_PLLREF):
            iClkSrc = CMX655_CLKCTRL_PLL_LRCLK;
            break;
        case (CMX655_CLKCTRL_CLKSEL | CMX655_CLKCTRL_PLLSEL):
            iClkSrc = CMX655_CLKCTRL_PLL_RCLK;
            break;
        case (CMX655_CLKCTRL_CLKSEL):
        case (CMX655_CLKCTRL_CLKSEL | CMX655_CLKCTRL_PLLREF):
            iClkSrc = CMX655_CLKCTRL_LPO;
            break;
        default:
            iClkSrc = CMX655_CLKCTRL_RCLK;
            break;
    }

    return(iClkSrc);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Get the current sample rate
+       return 0,1,2 or 3 for the sample rate -1 on an error
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int     CMX655GetSampleRate(void)

{   int     iSampleRate;
    iSampleRate = api->param_get(codec_dev, CMX655_CLKCTRL);
    iSampleRate = (iSampleRate >> CMX655_CLKCTRL_SR_SHIFT) & CMX655_CLKCTRL_SR_VALUE;
    return(iSampleRate);
}

/*
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+   Set the noise gate attenuation value
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
int     CMX655SetNoiseGateAtten(int piAtten)
{   
    int     iStatus;
    
    // if((iStatus=CMX655Read(NGLSTAT,2,abAtten))<0)
    if((iStatus = api->param_set(codec_dev, CMX655_NGCTRL, (uint8_t)piAtten)) < 0)
    {   LOG_ERR("Problem writing CMX655_NGCTRL\n");
    }
    return(iStatus);
}

void codec_print_regs()
{
    uint8_t rd=0;
    LOG_INF("-------------------------------");
    LOG_INF("\t CMX655D REGISTERS");
    LOG_INF("-------------------------------");
    rd = api->param_get(codec_dev, CMX655_CLKCTRL);
    LOG_INF("CMX655_CLKCTRL = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_RDIVHI);
    LOG_INF("CMX655_RDIVHI = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_RDIVLO);
    LOG_INF("CMX655_RDIVLO = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_NDIVHI);
    LOG_INF("CMX655_NDIVHI = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_NDIVLO);
    LOG_INF("CMX655_NDIVLO = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_PLLCTRL);
    LOG_INF("CMX655_PLLCTRL = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_SAICTRL);
    LOG_INF("CMX655_SAICTRL = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_SYSCTRL);
    LOG_INF("CMX655_SYSCTRL = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_LEVEL);
    LOG_INF("CMX655_LEVEL = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_RVF);
    LOG_INF("CMX655_RVF = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_PVF);
    LOG_INF("CMX655_PVF = 0x%x", rd);
    rd = api->param_get(codec_dev, CMX655_VOLUME);
    LOG_INF("CMX655_VOLUME = 0x%x", rd);
    LOG_INF("-------------------------------");
}

int codec_prepare()
{
	// set system clock
	int ret = api->prepare(codec_dev);
    if (ret < 0) {
        LOG_ERR("code prepare failed");
        return ret;
    }
	return ret;
}

int codec_startclk()
{
	// set system clock
	int ret = api->startclk(codec_dev);
    if (ret < 0) {
        LOG_ERR("codec startclk failed");
        return ret;
    }
	return ret;
}

int codec_stopclk()
{
	// set system clock
	int ret = api->stopclk(codec_dev);
    if (ret < 0) {
        LOG_ERR("codec stopclk failed");
        return ret;
    }
	return ret;
}

int codec_pa_disable()
{
	uint8_t mask = (CMX655_SYSCTRL_PAMP);
	int ret = api->chan_en_dis(codec_dev, mask, 0);
    if (ret < 0) {
        LOG_ERR("chan_en_dis failed");
        return ret;
    }

    return (ret);
}

int codec_pa_enable()
{
	uint8_t mask = (CMX655_SYSCTRL_PAMP);
	int ret = api->chan_en_dis(codec_dev, mask, CMX655_SYSCTRL_PAMP);
    if (ret < 0) {
        LOG_ERR("chan_en_dis failed");
        return ret;
    }

    return (ret);
}

int codec_mic_pa_enable()
{
    // cmx655d_update_bits(drv_data, CMX655_SYSCTRL, CMX655_SYSCTRL_SAI, CMX655_SYSCTRL_SAI);

	// enable sai, mic and power amp
	uint8_t chan_cfg = (CMX655_SYSCTRL_SAI | CMX655_SYSCTRL_MICL | CMX655_SYSCTRL_PAMP);
	// uint8_t chan_cfg = (CMX655_SYSCTRL_MICR);
	int ret = api->chan_en_dis(codec_dev, chan_cfg, chan_cfg);
    if (ret < 0) {
        LOG_ERR("chan_en_dis failed, fmt = %d", chan_cfg);
        return ret;
    }
    return ret;
}

int codec_enable_streams()
{
    int ret = 0;

#if (CMX655D_EXT_OCS_CLK)
    api->param_set(codec_dev, CMX655_SAICTRL, 0x98);
    api->param_set(codec_dev, CMX655_SAIMUX, 0x08);
    // api->param_set(codec_dev, CMX655_LEVEL, 0xCC);
    api->param_set(codec_dev, CMX655_PREAMP, 0x01);
    api->param_set(codec_dev, CMX655_VOLUME, 0x7F);
    api->param_set(codec_dev, CMX655_SYSCTRL, 0x28);
#elif (CMX655D_PLL_CLK)
    api->param_set(codec_dev, CMX655_SAICTRL, 0x38);
    api->param_set(codec_dev, CMX655_LEVEL, 0xCC);
    api->param_set(codec_dev, CMX655_RVF, 0x04);
    api->param_set(codec_dev, CMX655_PVF, 0x04);
    api->param_set(codec_dev, CMX655_VOLUME, 0xDC);
    api->param_set(codec_dev, CMX655_SYSCTRL, 0x2A);
#endif
    return ret;    
}

int codec_clock_setup(const struct device *dev, int n_chan, int s_rate)
{
 	int ret = 0;
    /* store the device and api pointers for further use */
    codec_dev = dev;
    api = codec_dev->api;
    uint8_t clkCtrl;

#if (CMX655D_EXT_OCS_CLK)
    codec_stopclk();
    switch (s_rate)
    {
    case 8000:
        clkCtrl = 0x00;
        break;
    case 16000:
        clkCtrl = 0x20;
    break;
    case 32000:
        clkCtrl = 0x40;
    break;
    case 48000:
        clkCtrl = 0x60;
    break;
    default:
        clkCtrl = 0x00; // 8k
        break;
    }
    api->param_set(codec_dev, CMX655_CLKCTRL, clkCtrl);
    ret = codec_startclk();
#elif (CMX655D_PLL_CLK)
    uint16_t NDiv;
    uint8_t RDiv=1, PllCtrl = 3;
    uint8_t NDiv_hi, NDiv_lo;
    switch (s_rate)
    {
    case 8000:
        clkCtrl = 0x1C;
        NDiv = 3072;
        break;
    case 16000:
        clkCtrl = 0x3C;
        NDiv = 1536;
    break;
    case 32000:
        clkCtrl = 0x5C;
        NDiv = 768;
    break;
    case 48000:
        clkCtrl = 0x7C;
        NDiv = 512;
    break;
    default:
        // 16k
        clkCtrl = 0x3C;
        break;
    }
    api->param_set(codec_dev, CMX655_CLKCTRL, clkCtrl);
    api->param_set(codec_dev, CMX655_RDIVLO, 0x01);
    
    NDiv_hi = NDiv >> 8;
    NDiv_lo = NDiv & 0xFF;
    api->param_set(codec_dev, CMX655_NDIVLO, NDiv_lo);
    api->param_set(codec_dev, CMX655_NDIVHI, NDiv_hi);

    api->param_set(codec_dev, CMX655_PLLCTRL, 0x03);
    api->startclk(codec_dev);
#endif
	return ret;
}

const struct device * codec_device_get()
{
    // return DEVICE_DT_GET(CMX655D_I2C_NODE);
    return DEVICE_DT_GET(DT_NODELABEL(cmx655d));
}