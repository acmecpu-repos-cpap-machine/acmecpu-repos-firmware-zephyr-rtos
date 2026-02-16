/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 21-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#ifndef MODULES_CMX655D_CMX655D_H_
#define MODULES_CMX655D_CMX655D_H_

#define CMX655_ISR      (0x00)
#define     CMX655_ISR_MICR        (1 << 0)
#define     CMX655_ISR_MICL        (1 << 1)
#define     CMX655_ISR_AMPOC       (1 << 2)
#define     CMX655_ISR_AMPCLIP     (1 << 3)
#define     CMX655_ISR_CLKRDY      (1 << 4)
#define     CMX655_ISR_THERM       (1 << 5)
#define     CMX655_ISR_VOL         (1 << 6)
#define     CMX655_ISR_CAL         (1 << 7)

#define CMX655_ISM      (0x01)
#define     CMX655_ISM_MICR        (1 << 0)
#define     CMX655_ISM_MICL        (1 << 1)
#define     CMX655_ISM_AMPOC       (1 << 2)
#define     CMX655_ISM_AMPCLIP     (1 << 3)
#define     CMX655_ISM_CLKRDY      (1 << 4)
#define     CMX655_ISM_THERM       (1 << 5)
#define     CMX655_ISM_VOL         (1 << 6)
#define     CMX655_ISM_CAL         (1 << 7)

#define CMX655_ISE      (0x02)

#define CMX655_CLKCTRL  (0x03)
#define     CMX655_CLKCTRL_PREDIV_SHIFT    (0)
#define     CMX655_CLKCTRL_PREDIV_VALUE    (0x3)
#define     CMX655_CLKCTRL_PREDIV_MASK     (CMX655_CLKCTRL_PREDIV_VALUE << \
                                                CMX655_CLKCTRL_PREDIV_SHIFT)

#define     CMX655_CLKCTRL_CLKSEL_SHIFT    (2)
#define     CMX655_CLKCTRL_CLKSEL          (1 << CMX655_CLKCTRL_CLKSEL_SHIFT)
#define     CMX655_CLKCTRL_PLLSEL_SHIFT    (3)
#define     CMX655_CLKCTRL_PLLSEL          (1 << CMX655_CLKCTRL_PLLSEL_SHIFT)
#define     CMX655_CLKCTRL_PLLREF_SHIFT    (4)
#define     CMX655_CLKCTRL_PLLREF          (1 << CMX655_CLKCTRL_PLLREF_SHIFT)

#define     CMX655_CLKCTRL_CLRSRC_SHIFT    (2)
#define     CMX655_CLKCTRL_CLRSRC_VALUE    (0x7)
#define     CMX655_CLKCTRL_CLRSRC_MASK     (CMX655_CLKCTRL_CLRSRC_VALUE << \
                                                CMX655_CLKCTRL_CLRSRC_SHIFT)
#define     CMX655_CLKCTRL_CLRSRC_RCLK     (0 << CMX655_CLKCTRL_CLRSRC_SHIFT) 
#define     CMX655_CLKCTRL_CLRSRC_LPO      (1 << CMX655_CLKCTRL_CLRSRC_SHIFT) 
#define     CMX655_CLKCTRL_CLRSRC_LRCLK    (7 << CMX655_CLKCTRL_CLRSRC_SHIFT) 
#define     CMX655_CLKCTRL_SR_SHIFT        (5)
#define     CMX655_CLKCTRL_SR_VALUE        (0x3)
#define     CMX655_CLKCTRL_SR_MASK         (CMX655_CLKCTRL_SR_VALUE << \
                                                CMX655_CLKCTRL_SR_SHIFT)
#define     CMX655_CLKCTRL_SR_8K            (0 << CMX655_CLKCTRL_SR_SHIFT)
#define     CMX655_CLKCTRL_SR_16K           (1 << CMX655_CLKCTRL_SR_SHIFT)
#define     CMX655_CLKCTRL_SR_32K           (2 << CMX655_CLKCTRL_SR_SHIFT)
#define     CMX655_CLKCTRL_SR_48K           (3 << CMX655_CLKCTRL_SR_SHIFT)

#define     CMX655_CLKCTRL_RCLK        (1)
#define     CMX655_CLKCTRL_LPO         (2)
#define     CMX655_CLKCTRL_PLL_LRCLK   (3)
#define     CMX655_CLKCTRL_PLL_RCLK    (4)


#define CMX655_RDIVHI   (0x04)
#define CMX655_RDIVLO   (0x05)
#define CMX655_NDIVHI   (0x06)
#define CMX655_NDIVLO   (0x07)

#define CMX655_PLLCTRL  (0x08)
#define     CMX655_PLLCTRL_CPI_SHIFT       (0)
#define     CMX655_PLLCTRL_LFILT_SHIFT     (4)
#define     CMX655_PLLCTRL_LFILT_VALUE     (0xF)
#define     CMX655_PLLCTRL_LFILT_MASK      (CMX655_PLLCTRL_LFILT_VALUE << CMX655_PLLCTRL_LFILT_SHIFT)
#define     CMX655_PLLCTRL_CPI_SHIFT       (0)
#define     CMX655_PLLCTRL_CPI_VALUE       (0xF)
#define     CMX655_PLLCTRL_CPI_MASK        (CMX655_PLLCTRL_CPI_VALUE << CMX655_PLLCTRL_CPI_SHIFT)


#define CMX655_SAICTRL  (0x09)
#define     CMX655_SAI_PCM         (1 << 0)
#define     CMX655_SAI_BINV        (1 << 2)
#define     CMX655_SAI_POL         (1 << 3)
#define     CMX655_SAI_DLY         (1 << 4)
#define     CMX655_SAI_MONO        (1 << 5)
#define     CMX655_SAI_WL          (1 << 6)
#define     CMX655_SAI_MSTR        (1 << 7)

#define CMX655_SAIMUX   (0x0a)

#define CMX655_RVF      (0x0c)
#define     CMX655_VF_DCBLOCK_SHIFT     (2)
#define     CMX655_VF_DCBLOCK           (1 << CMX655_VF_DCBLOCK_SHIFT)
#define     CMX655_VF_LPFEN_SHIFT       (3)
#define     CMX655_VF_LPFEN             (1 << CMX655_VF_LPFEN_SHIFT)
#define CMX655_LDCTRL   (0x0d)
#define CMX655_RDCTRL   (0x0e)
#define CMX655_LEVEL    (0x0f)
#define     CMX655_LEVEL_GR_SHIFT      (0)
#define     CMX655_LEVEL_GR_VALUE      (0x0f)
#define     CMX655_LEVEL_GR_MASK       (CMX655_LEVEL_GR_VALUE << CMX655_LEVEL_GR_SHIFT)
#define     CMX655_LEVEL_GL_SHIFT      (4)
#define     CMX655_LEVEL_GL_VALUE      (0x0f)
#define     CMX655_LEVEL_GL_MASK       (CMX655_LEVEL_GL_VALUE << CMX655_LEVEL_GL_SHIFT)
#define CMX655_NGCTRL   (0x1c)
#define     CMX655_NGCTRL_EN_SHIFT      (7)
#define     CMX655_NGCTRL_EN            (1 << CMX655_NGCTRL_EN_SHIFT)
#define CMX655_NGTIME   (0x1d)
#define CMX655_NGLSTAT  (0x1e)
#define CMX655_NGRSTAT  (0x1f)
#define CMX655_PVF      (0x28)
#define CMX655_PREAMP   (0x29)
#define CMX655_VOLUME   (0x2a)
#define     CMX655_VOL_MUTE        (0x00)
#define     CMX655_VOL_SHIFT       (0x00)
#define     CMX655_VOL_VALUE       (0x7f)
#define     CMX655_VOL_MASK        (CMX655_VOL_VALUE << CMX655_VOL_SHIFT)
#define     CMX655_VOL_SMOOTH_SHIFT    (7)
#define     CMX655_VOL_SMOOTH_MASK     (1 << CMX655_VOL_SMOOTH_SHIFT)
#define CMX655_ALCCTRL  (0x2b)
#define CMX655_ALCTIME  (0x2c)
#define CMX655_ALCGAIN  (0x2d)
#define CMX655_ALCSTAT  (0x2e)
#define CMX655_DST      (0x2f)
#define CMX655_CPR      (0x30)
#define CMX655_SYSCTRL  (0x32)
#define     CMX655_SYSCTRL_MICR    (1 << 0)
#define     CMX655_SYSCTRL_MICL    (1 << 1)
#define     CMX655_SYSCTRL_PAMP    (1 << 3)
#define     CMX655_SYSCTRL_LOUT    (1 << 4)
#define     CMX655_SYSCTRL_SAI     (1 << 5)

#define CMX655_COMMAND  (0x33)
#define     CMX655_CMD_CLOCK_STOP  (0x00)
#define     CMX655_CMD_CLOCK_START (0x01)
#define     CMX655_CMD_SOFT_RESET  (0xff)

#define CMX655_RATES (  SNDRV_PCM_RATE_8000 |\
                        SNDRV_PCM_RATE_16000 |\
                        SNDRV_PCM_RATE_32000 |\
                        SNDRV_PCM_RATE_48000 )

#define CMX655_FMTS ( SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE )

// clock id's when calling set sysclk
// Auto = Use RCLK when in DAI master mode. Use LRCLK in Slave mode.
// DO NOT use CMX655_SYSCLK_LRCLK when in DAI master mode
#define CMX655_SYSCLK_AUTO  (0)
#define CMX655_SYSCLK_RCLK  (1)
#define CMX655_SYSCLK_LRCLK (2)
#define CMX655_SYSCLK_LPO   (3)
#define CMX655_SYSCLK_MIN   (CMX655_SYSCLK_AUTO)
#define CMX655_SYSCLK_MAX   (CMX655_SYSCLK_LPO)


/** File reference: soc-dai.h
 *  https://docs.huihoo.com/doxygen/linux/kernel/3.7/soc-dai_8h.html
 * */
#define SND_SOC_DAIFMT_MASTER_MASK      0xf000
#define SND_SOC_DAIFMT_FORMAT_MASK      0x000f
#define SND_SOC_DAIFMT_INV_MASK         0x0f00
#define SND_SOC_DAIFMT_CBM_CFM          (1 << 12) /* codec clk & FRM master */
#define SND_SOC_DAIFMT_CBS_CFS          (4 << 12) /* codec clk & FRM slave */
#define SND_SOC_DAIFMT_I2S              1 /* I2S mode */
#define SND_SOC_DAIFMT_LEFT_J           3 /* Left Justified mode */
#define	SND_SOC_DAIFMT_NB_NF            (1 << 8) /* normal bit clock + frame */
#define	SND_SOC_DAIFMT_NB_IF            (2 << 8) /* normal BCLK + inv FRM */
#define	SND_SOC_DAIFMT_IB_NF            (3 << 8) /* invert BCLK + nor FRM */
#define	SND_SOC_DAIFMT_IB_IF            (4 << 8) /* invert BCLK + FRM */

#define OLD_DRV 0

struct cmx655d_hw_params {
    int sRate;
    int channels;
    int clock_mode; /* LRCLK = 0, RCLK = 1*/
};

/* API type defines */
typedef int (*startup_t)(const struct device *);

typedef int (*hw_params_set_t)(const struct device *, struct cmx655d_hw_params *);
typedef int (*main_clock_config_t)(const struct device *, struct cmx655d_hw_params *);
typedef int (*fmt_set_t)(const struct device *, uint32_t);
typedef int (*sysclk_set_t)(const struct device *, int iClkId);
typedef int (*chan_en_dis_t)(const struct device *, uint8_t, uint8_t);
typedef int (*prepare_t)(const struct device *);
typedef void (*shutdown_t)(const struct device *);
typedef int (*startclk_t)(const struct device *);
typedef int (*stopclk_t)(const struct device *);
typedef int (*param_get_t)(const struct device *, uint8_t/*in reg*/);
typedef int (*param_set_t)(const struct device *, uint8_t/*in reg*/, uint8_t val);


struct cmx655d_driver_api {
    startup_t startup;

    fmt_set_t fmt_set;
    sysclk_set_t sysclk_set;
    chan_en_dis_t chan_en_dis;
    
    prepare_t prepare;
    hw_params_set_t hw_params_set;
    shutdown_t shutdown;

    startclk_t startclk;
    stopclk_t stopclk;

    param_get_t param_get;
    param_set_t param_set;

    main_clock_config_t main_clock_config;
};

#endif /* MODULES_CMX655D_CMX655D_H_ */
