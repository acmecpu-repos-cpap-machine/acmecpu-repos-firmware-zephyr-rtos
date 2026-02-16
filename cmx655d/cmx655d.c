/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 21-Oct-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

#define DT_DRV_COMPAT cml_cmx655d
// #define CONFIG_CMX655D_INTERRUPT 1

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_CMX655D_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cmx655d);

#include "cmx655d.h"

#ifdef CONFIG_CMX655D_INTERRUPT
static bool m_irq_stat = false;
#endif

/*
 * Structure to hold info on cmx655 setup
 */
struct cmx655d_dai_data {
    int iSysClk;
    unsigned int uiEnabledStreams;
    bool bBestClkRunning; // Clear if prepare needs to setup the clock
    int iClkSrc;    
};

/** Configuration data */
struct cmx655d_config {
    /** The master I2C device's name and address */
	const char *i2c_master_name;
	uint16_t i2c_addr;
    struct i2c_dt_spec i2c_master;

#ifdef CONFIG_CMX655D_INTERRUPT
	/* Interrupt pin definition */
	// const char *int_gpio_port;
	// gpio_pin_t int_gpio_pin;
	// gpio_flags_t int_gpio_flags;
    struct gpio_dt_spec int_gpio;
#endif

	/* Reset pin definition */
	// const char *reset_gpio_port;
	// gpio_pin_t reset_gpio_pin;
	// gpio_flags_t reset_gpio_flags;
    struct gpio_dt_spec reset_gpio;
};

struct cmx655d_data {
	const struct device *i2c_master;
    uint16_t i2c_addr;
    struct k_sem lock;

#if CONFIG_CMX655D_INTERRUPT
	/* Self-reference to the driver instance */
	const struct device *instance;
	struct gpio_callback gpio_callback;
	struct k_work interrupt_worker;
#endif

    struct cmx655d_dai_data dai_data;
    struct cmx655d_hw_params hwParams;
};

static int cmx655d_read_regs(const struct cmx655d_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
	// return i2c_burst_read(dev->i2c_master, dev->i2c_addr, reg, data, length);
    return i2c_burst_read_dt(&config->i2c_master, reg, data, length);
}
static int seq = 1;
static int cmx655d_write_regs(const struct cmx655d_config *config, uint8_t reg, uint8_t *data, uint16_t length)
{
    // int ret = i2c_burst_write(dev->i2c_master, dev->i2c_addr, reg, data, length);
    int ret = i2c_burst_write_dt(&config->i2c_master, reg, data, length);
    if (ret == 0) {
        LOG_INF("write [%d] [0x%x] 0x%x", seq++, reg, *data);
    }
	return ret;
}

static int cmx655d_update_bits(const struct cmx655d_config *config, uint8_t reg, uint8_t mask, uint8_t val)
{
    int ret=0;
    uint8_t rw_val=0;

    /* check bits */
    // ret = i2c_burst_read(dev->i2c_master, dev->i2c_addr, reg, &rw_val, 1);
    ret = i2c_burst_read_dt(&config->i2c_master, reg, &rw_val, 1);
    if (ret == 0) {
        if ((rw_val & mask) == val) {
            // no need to update
            ret = 0;
        } else {
            // update bits
            rw_val = (rw_val & ~mask) | (val & mask);
	        // ret = i2c_burst_write(dev->i2c_master, dev->i2c_addr, reg, &rw_val, 1);
            ret = i2c_burst_write_dt(&config->i2c_master, reg, &rw_val, 1);
            if (ret == 0) {
                LOG_INF("write [%d] [0x%x] 0x%x", seq++, reg, rw_val);
            }
        }
    }
    return ret;
}

#ifdef CONFIG_CMX655D_INTERRUPT
static void cmx655d_interrupt_worker(struct k_work *work) {
	struct cmx655d_data *const drv_data = CONTAINER_OF(work, struct cmx655d_data, interrupt_worker);
	// int ret = 0;

	k_sem_take(&drv_data->lock, K_FOREVER);

    m_irq_stat = true;

	/* Read and store charger status */
	// ret = read_charger_status(drv_data->instance, &drv_data->charger_status);
	// if (ret != 0) {
	// 	LOG_ERR("Failed to read charger status (%d)", ret);
	// 	goto err;
	// }

	/* Call the higher level callback */
	// if (drv_data->handler != NULL) {
	// 	drv_data->handler(drv_data->instance, &drv_data->charger_status);
	// }

// err:
	k_sem_give(&drv_data->lock);
}

static void cmx655d_interrupt_callback(const struct device *dev, struct gpio_callback *cb, gpio_port_pins_t pins) {
	struct cmx655d_data *const drv_data = CONTAINER_OF(cb, struct cmx655d_data, gpio_callback);

	ARG_UNUSED(pins);

	/* Cannot read CMX655D registers from ISR context, queue worker */
	k_work_submit(&drv_data->interrupt_worker);
}
#endif /* CONFIG_CMX655D_INTERRUPT */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Define some functions used by this module to control the CMX655
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */
/* 
 * Start CMX655 internal clock and wait for clock ready bit
 */
static int cmx655StartSysClk(const struct cmx655d_config *config)
{
    int iRet;
    int i;
    uint8_t uiVal;
    // Dummy read to clear status bits
    // uiVal = SOC_COM_READ(psComponent, CMX655_ISR);
    iRet = cmx655d_read_regs(config, CMX655_ISR, &uiVal, 1);

    // Start clock
    // iRet = snd_soc_component_write(psComponent, CMX655_COMMAND, 
    //                                             CMX655_CMD_CLOCK_START);
    uint8_t rw_val = CMX655_CMD_CLOCK_START;
    iRet = cmx655d_write_regs(config, CMX655_COMMAND, &rw_val, 1);
    if (iRet < 0) {
        LOG_ERR("Failed to write start clock command %d\n", iRet);
        return iRet;
    }

    // Wait for status bit
    for (i=0; i< 100; i++)
    {
        // uiVal = SOC_COM_READ(psComponent, CMX655_ISR);
        cmx655d_read_regs(config, CMX655_ISR, &uiVal, 1);
        if (uiVal & CMX655_ISR_CLKRDY)
        {
            break;
        } 
    }
    if (i == 100)
    {
        // Clock did not start
        iRet = -EIO;
    } 
    return iRet;
}

static int cmx655StopSysClk(const struct cmx655d_config *config)
{
    // return snd_soc_component_write(psComponent, CMX655_COMMAND, 
    //                                 CMX655_CMD_CLOCK_STOP);
    uint8_t rw_val = CMX655_CMD_CLOCK_STOP;
    return cmx655d_write_regs(config, CMX655_COMMAND, &rw_val, 1);
}

/*
 *  Get the clock setup the system clock based on clock Id, DAI master mode 
 *  and sample rate
 *      iClkId      - Clock source setting as defined in cmx655.h
 *      iMasterMode - Non-zero if the CMX655 is the DAI master
 *      iSRSetting  - Setting for sample rate 0 to 3
 *      piClkSrc    - pointer for storing clock source (PLLREF, PLLSEL and 
 *                          CLKSEL bits)
 *      piRDiv      - pointer for storing PLL's RDIV value (13 bits)
 *      piNDiv      - pointer for storing PLL's NDIV value (13 bits)
 *      piPllCtrl   - pointer for storing PLLCTRL register value (8 bits)
 */
static int cmx655GetSysClkConfig(int iClkId,
                                    uint8_t iMasterMode,
                                    int iSRSetting,
                                    int *piClkSrc,
                                    int *piRDiv,
                                    int *piNDiv,
                                    int *piPllCtrl)
{
    // Do auto selection
    if (iClkId == CMX655_SYSCLK_AUTO)
    {
        if (iMasterMode != 0)
        {
            iClkId = CMX655_SYSCLK_RCLK;
        }
        else
        {
            iClkId = CMX655_SYSCLK_LRCLK;
        }
    }
    // Set default values
    *piRDiv = 0;
    *piNDiv = 0;
    *piPllCtrl = 0;
    switch (iClkId) 
    {
        case (CMX655_SYSCLK_RCLK):
            *piClkSrc = CMX655_CLKCTRL_CLRSRC_RCLK;
            break;
        case (CMX655_SYSCLK_LPO):
            *piClkSrc = CMX655_CLKCTRL_CLRSRC_LPO;
            break;
        case (CMX655_SYSCLK_LRCLK):
            *piClkSrc = CMX655_CLKCTRL_CLRSRC_LRCLK;
            *piRDiv = 1;
            switch (iSRSetting)
            {
                case (CMX655_CLKCTRL_SR_8K):
                {
                    *piNDiv = 3072;
                    *piPllCtrl = 3; //(0 << CMX655_PLLCTRL_LFILT_SHIFT) || (3 << CMX655_PLLCTRL_CPI_SHIFT);
                    LOG_DBG("CMX655_CLKCTRL_SR_8K, iNDiv = %d, iPllCtrl = %d", *piNDiv, *piPllCtrl);
                    break;
                }
                case (CMX655_CLKCTRL_SR_16K):
                {
                    *piNDiv = 1536;
                    *piPllCtrl = 3; //(0 << CMX655_PLLCTRL_LFILT_SHIFT) || (3 << CMX655_PLLCTRL_CPI_SHIFT);
                    LOG_DBG("CMX655_CLKCTRL_SR_16K, iNDiv = %d, iPllCtrl = %d", *piNDiv, *piPllCtrl);
                    break;
                }
                case (CMX655_CLKCTRL_SR_32K):
                {
                    *piNDiv = 768;
                    *piPllCtrl = 195; //(12<< CMX655_PLLCTRL_LFILT_SHIFT) || (3 << CMX655_PLLCTRL_CPI_SHIFT);
                    LOG_DBG("CMX655_CLKCTRL_SR_32K, iNDiv = %d, iPllCtrl = %d", *piNDiv, *piPllCtrl);
                    break;
                }
                case (CMX655_CLKCTRL_SR_48K):
                {
                    *piNDiv = 512;
                    *piPllCtrl = (12<< CMX655_PLLCTRL_LFILT_SHIFT) || (3 << CMX655_PLLCTRL_CPI_SHIFT);
                    LOG_DBG("CMX655_CLKCTRL_SR_48K, iNDiv = %d, iPllCtrl = %d", *piNDiv, *piPllCtrl);
                    break;
                }
                default:
                    return -EINVAL;
            }
            break;
        default:
            return -EINVAL;
    }
    return 0;
};

/*
 *  Setup the clock and sample rate. The clock needs to be setup at the same
 *  time as the sample rate encase we are using the serial port as the clock 
 *  source.
 *  If the clock source the serial port then the PLL settings are dependent on 
 *  the sample rate.
 */
static int cmx655SetupRate(struct cmx655d_data *drv_data, const struct cmx655d_config *config, 
                            struct cmx655d_hw_params *psHwParams)
{
    int iRet;
    // struct cmx655Data *psCmx655Data =
    //                               snd_soc_component_get_drvdata(psComponent);
    // struct cmx655DaiData *psCmx655DaiData = &psCmx655Data->sDaiData;
    // int iSRate = params_rate(psHwParams);
    int iSRate = psHwParams->sRate;
    uint8_t iMasterMode;
    int iSRateSetting;
    int iClkSrc;
    int iRDiv;
    int iNDiv;
    int iPllCtrl;
    uint8_t iSysCtrl;
    uint8_t iVol;

#if OLD_DRV
    // iMasterMode = SOC_COM_READ(psComponent, CMX655_SAICTRL);
    iRet = cmx655d_read_regs(config, CMX655_SAICTRL, &iMasterMode, 1);
    if (iRet < 0) {
        LOG_ERR("cmx655d_read_regs failed %d", CMX655_SAICTRL);
        return iRet;
    }
    iMasterMode = iMasterMode & CMX655_SAI_MSTR;
#else
    iMasterMode = psHwParams->clock_mode;
#endif

    // Workout clock settings
    // Start with sample rate
    switch (iSRate) 
    {
        case 8000:
            iSRateSetting = CMX655_CLKCTRL_SR_8K;
            break; 
        case 16000:
            iSRateSetting = CMX655_CLKCTRL_SR_16K;
            break; 
        case 32000:
            iSRateSetting = CMX655_CLKCTRL_SR_32K;
            break; 
        case 48000:
            iSRateSetting = CMX655_CLKCTRL_SR_48K;
            break; 
        default:
            LOG_ERR("Unsupported rate %d\n", iSRate);
            return -EINVAL; 
    }
    
    iRet = cmx655GetSysClkConfig(drv_data->dai_data.iSysClk,
                                 iMasterMode, iSRateSetting,
                                 &iClkSrc, &iRDiv, &iNDiv, &iPllCtrl);
    if (iRet < 0)
    {
        LOG_ERR("Failed to get system clock settings %i\n", iRet);
    }
    // Check if we are using the LRCLK as the source.
    if (iClkSrc == CMX655_CLKCTRL_CLRSRC_LRCLK)
    {
        LOG_DBG("Using LRCLK as clk source. Using LPO for setup then switch over to LRCLK later");
        // Store correct clock source for later use
        // psCmx655DaiData->iClkSrc = iClkSrc;
        // psCmx655DaiData->bBestClkRunning = false; // Need more setup later
        drv_data->dai_data.iClkSrc = iClkSrc;
        drv_data->dai_data.bBestClkRunning = false; // Need more setup later
        iClkSrc = CMX655_CLKCTRL_CLRSRC_LPO;
    }
    else
    {
        // psCmx655DaiData->bBestClkRunning = true;
        drv_data->dai_data.bBestClkRunning = false; // Need more setup later
    }
    // Test to see if the clock source and sample rate are correct.
    // If so we can skip the setup
    // if (snd_soc_component_test_bits(psComponent, CMX655_CLKCTRL,
    //                  CMX655_CLKCTRL_CLRSRC_MASK | CMX655_CLKCTRL_SR_MASK,
    //                                     iClkSrc | iSRateSetting) == 0)
    // {
    //     dev_dbg(psComponent->dev,
    //                 "Rate Setup correct skipping setup\n");
    //     return 0;

    // }
    
    // Turn all inputs and outputs off before disabling clock
    // iSysCtrl = SOC_COM_READ(psComponent, CMX655_SYSCTRL);
    // snd_soc_component_update_bits(psComponent, CMX655_SYSCTRL,
    //                                 CMX655_SYSCTRL_MICR |
    //                                 CMX655_SYSCTRL_MICL |
    //                                 CMX655_SYSCTRL_PAMP |
    //                                 CMX655_SYSCTRL_LOUT,    0);
#if OLD_DRV
    iRet = cmx655d_read_regs(config, CMX655_SYSCTRL, &iSysCtrl, 1);
    iRet = cmx655d_update_bits(config, CMX655_SYSCTRL,
                                    CMX655_SYSCTRL_MICR |
                                    CMX655_SYSCTRL_MICL |
                                    CMX655_SYSCTRL_PAMP |
                                    CMX655_SYSCTRL_LOUT,    0);
    if (iRet < 0) {
        LOG_ERR("cmx655d_update_bits failed %d", CMX655_SYSCTRL);
        return iRet;
    }

    cmx655StopSysClk(config);
    // Set new sample rate and clock source
    // snd_soc_component_update_bits(psComponent, CMX655_CLKCTRL, 
    //                  CMX655_CLKCTRL_CLRSRC_MASK | CMX655_CLKCTRL_SR_MASK,
    //                                     iClkSrc | iSRateSetting);
    cmx655d_update_bits(config, CMX655_CLKCTRL,
                        CMX655_CLKCTRL_CLRSRC_MASK | CMX655_CLKCTRL_SR_MASK,
                                    iClkSrc | iSRateSetting);
#endif
    cmx655d_update_bits(config, CMX655_CLKCTRL,
                            CMX655_CLKCTRL_CLRSRC_MASK | CMX655_CLKCTRL_SR_MASK,
                            CMX655_CLKCTRL_CLRSRC_LRCLK | iSRateSetting);

    // Set new RDIV
    // snd_soc_component_update_bits(psComponent, CMX655_RDIVHI,
    //                                     0x1F, iRDiv >> 8);
    // snd_soc_component_update_bits(psComponent, CMX655_RDIVLO,
    //                                     0xFF, iRDiv & 0xFF);
    cmx655d_update_bits(config, CMX655_RDIVHI,
                                        0x1F, iRDiv >> 8);
    cmx655d_update_bits(config, CMX655_RDIVLO,
                                        0xFF, iRDiv & 0xFF);

    // Set new NDIV
    // snd_soc_component_update_bits(psComponent, CMX655_NDIVHI,
    //                                     0x1F, iNDiv >> 8);
    // snd_soc_component_update_bits(psComponent, CMX655_NDIVLO,
    //                                     0xFF, iNDiv & 0xFF);
    cmx655d_update_bits(config, CMX655_NDIVHI,
                                        0x1F, iNDiv >> 8);
    cmx655d_update_bits(config, CMX655_NDIVLO,
                                        0xFF, iNDiv & 0xFF);

    // Set new PLLCTRL
    // snd_soc_component_update_bits(psComponent, CMX655_PLLCTRL,
    //                                     0xFF, iPllCtrl & 0xFF);
    LOG_DBG("iPllCtrl = %d", iPllCtrl);
    cmx655d_update_bits(config, CMX655_PLLCTRL,
                                        0xFF, iPllCtrl & 0xFF);

    // Now we can re-start the clock
    if ((iRet=cmx655StartSysClk(config))<0)
    {
        LOG_WRN("System clock failed to start %i\n", iRet);
        iRet = 0;
        return iRet;
    }
#if OLD_DRV
    // Turn anything on that we turned off
    if ((iSysCtrl & (CMX655_SYSCTRL_MICR | CMX655_SYSCTRL_MICL)) > 0)
    {   // Turn on mic(s)
        // snd_soc_component_update_bits(psComponent, CMX655_SYSCTRL,
        //                                 CMX655_SYSCTRL_MICR |
        //                                 CMX655_SYSCTRL_MICL,
        //                                 iSysCtrl);
        cmx655d_update_bits(config, CMX655_SYSCTRL,
                                        CMX655_SYSCTRL_MICR |
                                        CMX655_SYSCTRL_MICL,
                                        iSysCtrl);

        // Wait for filters to settle
        // if (snd_soc_component_test_bits(psComponent, CMX655_RVF, 
        //                                 CMX655_VF_DCBLOCK,
        //                                 CMX655_VF_DCBLOCK) == 0)
        // {   // DC blocking filter off, Shorter wait
        //     usleep_range(3500,4000); 
        // }
        // else
        // {
            // This allows time for Mics and DC blocking filter to settle
            // msleep(320);
        // }
        k_sleep(K_MSEC(320));
    }
    if ((iSysCtrl & (CMX655_SYSCTRL_PAMP | CMX655_SYSCTRL_LOUT)) > 0)
    {   // Turn output(s) on
        // Store volume
        // iVol = SOC_COM_READ(psComponent, CMX655_VOLUME);
        cmx655d_read_regs(config, CMX655_VOLUME, &iVol, 1);
        // Lower volume with smooth on
        // snd_soc_component_write(psComponent, CMX655_VOLUME, 0x80);
        // snd_soc_component_update_bits(psComponent, CMX655_SYSCTRL,
        //                                 CMX655_SYSCTRL_PAMP |
        //                                 CMX655_SYSCTRL_LOUT,
        //                                 iSysCtrl);
        uint8_t rw_val = 0x80;
        cmx655d_write_regs(config, CMX655_VOLUME, &rw_val, 1);
        cmx655d_update_bits(config, CMX655_SYSCTRL,
                                        CMX655_SYSCTRL_PAMP |
                                        CMX655_SYSCTRL_LOUT,
                                        iSysCtrl);
        // Restore volume
        // snd_soc_component_write(psComponent, CMX655_VOLUME, iVol);
        cmx655d_write_regs(config, CMX655_VOLUME, &iVol, 1);
    } 
#endif
    return 0;
};
static int cmx655d_sai_fmt_set(const struct device *dev, uint32_t uiFmt)
{
    const struct cmx655d_config *config = dev->config;
	// struct cmx655d_data *drv_data = dev->data;
    int ret = 0;

    uint8_t uiRegVal = 0;
    // Set master bit
    switch (uiFmt & SND_SOC_DAIFMT_MASTER_MASK)
    {
        case SND_SOC_DAIFMT_CBM_CFM:
            uiRegVal = uiRegVal | CMX655_SAI_MSTR;
            break;
        case SND_SOC_DAIFMT_CBS_CFS:
            // Could or in 0 but no need
            break;
        default:
            LOG_ERR("Unsupported digital audio interface master mode\n");
            return -EINVAL;
    }
    // Set data format
    switch (uiFmt & SND_SOC_DAIFMT_FORMAT_MASK)
    {
        case SND_SOC_DAIFMT_I2S:
            uiRegVal = uiRegVal | CMX655_SAI_DLY | CMX655_SAI_POL;
            break;
        case SND_SOC_DAIFMT_LEFT_J:
            // Could or in 0 but no need
            break;
         default:
            LOG_ERR("Unsupported digital audio interface data format\n");
            return -EINVAL;
    }
    // Change invert bits if required
    switch (uiFmt & SND_SOC_DAIFMT_INV_MASK)
    {
        case SND_SOC_DAIFMT_NB_NF:
            // No inverts do nothing
            break;
        case SND_SOC_DAIFMT_NB_IF:
            uiRegVal = uiRegVal ^ CMX655_SAI_POL;
            break;
        case SND_SOC_DAIFMT_IB_NF:
            uiRegVal = uiRegVal | CMX655_SAI_BINV;
            break;
        case SND_SOC_DAIFMT_IB_IF:
            uiRegVal = (uiRegVal | CMX655_SAI_BINV) ^ CMX655_SAI_POL;
            break;
        default:
            LOG_ERR("Unknown digital audio interface polarity\n");
            return -EINVAL;
    }

    // Write value to codec
    // snd_soc_component_write(psComponent, CMX655_SAICTRL, uiRegVal);
    // uiRegVal = 0x18;
    // enable mono
    uiRegVal = uiRegVal | CMX655_SAI_MONO;
    ret = cmx655d_write_regs(config, CMX655_SAICTRL, &uiRegVal, 1);
    if (ret < 0) {
        LOG_ERR("cmx655d_write_regs failed %d", CMX655_SAICTRL);
        return ret;
    }

    return ret;
}

/*
 * Save and check requested ClkId is valid.
 * Clock is setup as part of hw params
 */
static int cmx655d_sai_sysclk_set(const struct device *dev, int iClkId)
{
    // struct cmx655Data *psCmx655Data =
    //                         snd_soc_component_get_drvdata(psDai->component); 
    // struct cmx655DaiData *psCmx655DaiData = &psCmx655Data->sDaiData;
    // const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;
   
    switch (iClkId)
    {
        case CMX655_SYSCLK_MIN ... CMX655_SYSCLK_MAX:
            break;
        default:
            return -EINVAL;
    }

    // psCmx655DaiData->iSysClk = iClkId;
    drv_data->dai_data.iSysClk = iClkId;
    
    return 0;
};

static int cmx655d_chan_enable_disable(const struct device *dev, uint8_t mask, uint8_t val)
{
    const struct cmx655d_config *config = dev->config;
    // struct cmx655d_data *drv_data = dev->data;

    int ret = cmx655d_update_bits(config, CMX655_SYSCTRL,
                                  mask,
                                  val);

    return ret;
}

static int cmx655d_startclk(const struct device *dev)
{
    const struct cmx655d_config *config = dev->config;
    struct cmx655d_data *drv_data = dev->data;
    int iRet = 0;

    if ((iRet = cmx655StartSysClk(config)) < 0)
    {
        LOG_WRN("Failed to restart clock\n");
        iRet = 0;
        // This will happen if the CPU driver does not start the LRCLK 
        // until the last point.
        // For now we will assume the clock will start
    }

    drv_data->dai_data.bBestClkRunning = true;
    return iRet;
}

static int cmx655d_stopclk(const struct device *dev)
{
    const struct cmx655d_config *config = dev->config;
    struct cmx655d_data *drv_data = dev->data;
    int iRet = 0;
    iRet = cmx655StopSysClk(config);
    if (iRet == 0)
        drv_data->dai_data.bBestClkRunning = false;
    return iRet;
}

/*
 * Callback to prepare, if running from the LRCLK we will need to 
 * swap to it here. 
 * Cannot do it in hw_params as the CPU's port was not setup
 */
static int cmx655d_prepare(const struct device *dev)
{
    int iRet = 0;
    // struct snd_soc_component *psComponent = psDai->component;
    // struct cmx655Data *psCmx655Data =
    //                               snd_soc_component_get_drvdata(psComponent);
    // struct cmx655DaiData *psCmx655DaiData = &psCmx655Data->sDaiData;

    const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;

    // if (!psCmx655DaiData->bBestClkRunning)
    if (!drv_data->dai_data.bBestClkRunning)
    {   // Stop the clock change over to the correct one an start it again
        if ((iRet = cmx655StopSysClk(config)) < 0)
        {   LOG_ERR("Failed to stop clock %d\n", iRet);
            goto GetOut;
        }
        // iRet = snd_soc_component_update_bits(psComponent, CMX655_CLKCTRL,
        //                                     CMX655_CLKCTRL_CLRSRC_MASK,
        //                                         psCmx655DaiData->iClkSrc);
        iRet = cmx655d_update_bits(config, CMX655_CLKCTRL,
                                            CMX655_CLKCTRL_CLRSRC_MASK,
                                                drv_data->dai_data.iClkSrc);
        if (iRet < 0)
        {   LOG_ERR("Failed to set new clock setup %d\n", iRet);
            goto GetOut;
        }
        if ((iRet = cmx655StartSysClk(config)) < 0)
        {   LOG_WRN("Failed to restart clock\n");
            iRet = 0;
            // This will happen if the CPU driver does not start the LRCLK 
            // until the last point.
            // For now we will assume the clock will start
        }
        drv_data->dai_data.bBestClkRunning = true;
    }
GetOut:
    return iRet;
};
static int cmx655d_hw_params_set(const struct device *dev, struct cmx655d_hw_params *psHwParams)
{
    const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;

    int iRet=0;
    // struct snd_soc_component *psComponent = psDai->component;
    // struct i2c_client *psI2c = to_i2c_client(psComponent->dev);
    // struct cmx655Data *psCmx655Data =
    //                               snd_soc_component_get_drvdata(psComponent);
    // struct cmx655DaiData *psCmx655DaiData = &psCmx655Data->sDaiData;
    // unsigned int uiEnabledStreams = psCmx655DaiData->uiEnabledStreams;
    unsigned int uiEnabledStreams = drv_data->dai_data.uiEnabledStreams;
     
    if (drv_data->dai_data.bBestClkRunning)
    {   // Will get here if the clock is in use so don't go stopping it
        LOG_INF("Clock running. Skipping setup\n");
    }
    else
    {
        // Setup clock and sample rate
        if ((iRet = cmx655SetupRate(drv_data, config, psHwParams)) < 0)
        {   
            LOG_ERR("Failed to set rates %d\n", iRet);
            return iRet;
        }
    }
    // Set mono bit based on channel count
    // if (params_channels(psHwParams) == 1)
    if (psHwParams->channels == 1)
    {
        LOG_INF("Switching into mono mode\n");
        // snd_soc_component_update_bits(psComponent, CMX655_SAICTRL,
        //                                 CMX655_SAI_MONO, CMX655_SAI_MONO);
        cmx655d_update_bits(config, CMX655_SAICTRL,
                                        CMX655_SAI_MONO, CMX655_SAI_MONO);
    }else
    {
        // snd_soc_component_update_bits(psComponent, CMX655_SAICTRL,
        //                                 CMX655_SAI_MONO, 0); 
        cmx655d_update_bits(config, CMX655_SAICTRL,
                                        CMX655_SAI_MONO, 0);
    }
     
    // TODO: implement later
    // if (psI2c->irq)
    // {
    //     psCmx655Data->uiOcCnt = 0; // Reset overcurrent count
    // }
    if (uiEnabledStreams == 0){
        LOG_INF("First stream to enable, enabling SAI\n");
        // If first stream to be enabled
        // Enable SAI (serial audio interface) port
        // We need it running before the platform starts.
        // to avoid I2S sync errors
        // snd_soc_component_update_bits(psComponent, CMX655_SYSCTRL, 
        //                                CMX655_SYSCTRL_SAI, CMX655_SYSCTRL_SAI);
        cmx655d_update_bits(config, CMX655_SYSCTRL,
                                        CMX655_SYSCTRL_SAI, CMX655_SYSCTRL_SAI);
    }
    else
    {
        LOG_INF("Not first stream to enable, skipping SAI enable\n");
    }
    
    // Inc enabled streams by 1 
    drv_data->dai_data.uiEnabledStreams = uiEnabledStreams + 1;

    return iRet;
}
/*
 * Shutdown DAI link
 */
static void cmx655d_sai_shutdown(const struct device *dev)
{
    const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;
    // struct snd_soc_component *psComponent = psDai->component;
    // struct cmx655Data *psCmx655Data =
    //                               snd_soc_component_get_drvdata(psComponent);
    // struct cmx655DaiData *psCmx655DaiData = &psCmx655Data->sDaiData;
    unsigned int uiEnabledStreams = drv_data->dai_data.uiEnabledStreams;
    if (uiEnabledStreams == 0)
    {
        // Protect against shutdown getting called without a start.
        // This was seen with audacity
        LOG_INF("Shutdown called when SAI not running\n");
        return;
    }

    // Reduce enabled streams by 1 
    uiEnabledStreams = uiEnabledStreams - 1;
    drv_data->dai_data.uiEnabledStreams = uiEnabledStreams;
    if (uiEnabledStreams == 0) 
    {
        LOG_INF("Last stream to disable, disabling SAI\n");
        // If no streams left
        // Disable SAI port
        // snd_soc_component_update_bits(psComponent, CMX655_SYSCTRL, 
        //                                CMX655_SYSCTRL_SAI, 0);
        cmx655d_update_bits(config, CMX655_SYSCTRL,
                                        CMX655_SYSCTRL_SAI, 0);

        // Setup the clock again next time arounf
        drv_data->dai_data.bBestClkRunning = false;
    }
    else
    {
        LOG_INF("Not last stream to disable, skipping SAI disable\n");
    }
}

static int cmx655d_param_get(const struct device *dev, uint8_t reg)
{
    const struct cmx655d_config *config = dev->config;
	// struct cmx655d_data *drv_data = dev->data;
    uint8_t val = 0;
    cmx655d_read_regs(config, reg, &val, 1);
    return val;
}

static int cmx655d_param_set(const struct device *dev, uint8_t reg, uint8_t val)
{
    int ret = 0;
    const struct cmx655d_config *config = dev->config;
	// struct cmx655d_data *drv_data = dev->data;
    ret = cmx655d_write_regs(config, reg, &val, 1);
    return ret;
}

static int cmx655d_main_clock_config(const struct device *dev, struct cmx655d_hw_params *psHwParams)
{
    int ret = 0;
    const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;

    ret = cmx655SetupRate(drv_data, config, psHwParams);
    return ret;
}

static int cmx655d_startup(const struct device *dev)
{
    const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;
    int ret = 0;
    
    uint8_t rw_val = 0x00;
    /* Configure the main clock and system sample rate and then start the main clock. */
    /*  CLKCTRL settings
    *   SR = 16ksps (001)
    *   PLLREF = DIVCLK (0)
    *   PLLSEL = Open-loop VCO (LPOSC) (0)
    *   CLKSEL = VCOCLK (1)
    *   PREDIV = Divide by 1 (00)
    */
    rw_val = 0x24;
    ret = cmx655d_write_regs(config, CMX655_CLKCTRL, &rw_val, 1);
    if (ret < 0) {
        LOG_ERR("cmx655d_write_regs failed %d", CMX655_CLKCTRL);
        return ret;
    }
#ifdef CONFIG_CMX655D_INTERRUPT
    /*  ISM settings
    *   Mask all interrupts except CLKRDY
    */
    rw_val = 0x10;
    ret = cmx655d_write_regs(config, CMX655_ISM, &rw_val, 1);
    if (ret < 0) {
        LOG_ERR("cmx655d_write_regs failed %d", CMX655_ISM);
        return ret;
    }
#endif
    /*  COMMAND settings
    *   Start the main clock
    */
    rw_val = 0x01;
    ret = cmx655d_write_regs(config, CMX655_COMMAND, &rw_val, 1);
    if (ret < 0) {
        LOG_ERR("cmx655d_write_regs failed %d", CMX655_COMMAND);
        return ret;
    }

    /* Wait for the IRQN line to go low and then read the ISR register to confirm the main clock has gone active. */
#ifdef CONFIG_CMX655D_INTERRUPT
    // const struct device *intr_gpio_dev = device_get_binding(config->int_gpio_port);
    while(!m_irq_stat)  k_sleep(K_MSEC(1));
    int pin_stat = gpio_pin_get(config->int_gpio.port, config->int_gpio.pin);
    if (pin_stat == 1) {    /* low is 1 for active low pin */
        /*  ISR status
        */
        ret = cmx655d_read_regs(config, CMX655_ISR, &rw_val, 1);
        if (ret < 0) {
            LOG_ERR("cmx655d_read_regs failed %d", CMX655_ISR);
            return ret;
        }
        if (rw_val & CMX655_ISR_CLKRDY) {
            LOG_INF("Main clock ready");
            k_sem_take(&drv_data->lock, K_FOREVER);
            m_irq_stat = true;
            k_sem_give(&drv_data->lock);
        } else {
            LOG_ERR("Main clock did not start, exit");
            return -1;
        }
    } else {
        LOG_ERR("Main clock did not start, exit");
        return -1;
    }
#else
    int i;
    for (i=0; i< 100; i++) {
        ret = cmx655d_read_regs(config, CMX655_ISR, &rw_val, 1);
        if (ret < 0) {
            LOG_ERR("cmx655d_read_regs failed %d", CMX655_ISR);
            return ret;
        }
        if (rw_val & CMX655_ISR_CLKRDY)
            break;
        k_sleep(K_MSEC(1));
    }
    if (i == 100) {
        LOG_ERR("main clock did not start");
        ret = -EIO;
    }
#endif
    
    return ret;
}

static int cmx655d_init(const struct device *dev)
{
	const struct cmx655d_config *config = dev->config;
	struct cmx655d_data *drv_data = dev->data;
	int ret = 0;

	/* Get I2C master device instance */
	// drv_data->i2c_master = device_get_binding((char *)config->i2c_master_name);
	// if (!drv_data->i2c_master) {
	// 	return -EINVAL;
	// }
    // drv_data->i2c_addr = config->i2c_addr;

   	if (!device_is_ready(config->i2c_master.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}
    
    k_sem_init(&drv_data->lock, 1, 1);

    /* Set-up value following the codec reset that will happen in a bit */
    drv_data->dai_data.uiEnabledStreams = 0;
    drv_data->dai_data.bBestClkRunning = false;
    
    /* Configure interrupt pin */
#ifdef CONFIG_CMX655D_INTERRUPT
    /* Store self-reference for interrupt handling */
	drv_data->instance = dev;

	/* Prepare interrupt worker */
	k_work_init(&drv_data->interrupt_worker, cmx655d_interrupt_worker);

	/* Configure interrupt GPIO pin */
	if (!device_is_ready(config->int_gpio.port)) {
		LOG_ERR("INT device is not ready");
		return -ENODEV;
    }
	// const struct device *intr_gpio_dev = device_get_binding(config->int_gpio_port);
	else {
		ret = gpio_pin_configure(config->int_gpio.port, config->int_gpio.pin, 
                                (GPIO_INPUT | config->int_gpio.dt_flags));
		ret = gpio_pin_interrupt_configure(config->int_gpio.port, config->int_gpio.pin, 
                                (GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt pin %d (%d)", config->int_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&drv_data->gpio_callback, cmx655d_interrupt_callback, BIT(config->int_gpio.pin));
		gpio_add_callback(config->int_gpio.port, &drv_data->gpio_callback);
	}
#endif

    /* reset the codec */
	// const struct device *dev_rst = device_get_binding(config->reset_gpio_port);
	if (device_is_ready(config->reset_gpio.port)) {
	    ret = gpio_pin_configure(config->reset_gpio.port, config->reset_gpio.pin, 
                (GPIO_OUTPUT | GPIO_PUSH_PULL | config->reset_gpio.dt_flags));
        /* Hold reset line */
	    ret = gpio_pin_set(config->reset_gpio.port, config->reset_gpio.pin, 1);
        /* Time of reset pulse must be greater than 1us sleep for 10us to 1ms, speed is not critical here */
        k_sleep(K_MSEC(100));
        /* release reset line */
        ret = gpio_pin_set(config->reset_gpio.port, config->reset_gpio.pin, 0);
        k_sleep(K_MSEC(10));
    } else {
        /* do soft reset */
        uint8_t rw_val = CMX655_CMD_SOFT_RESET;
        ret = cmx655d_write_regs(config, CMX655_COMMAND, &rw_val, 1);
        if (ret < 0) {
            LOG_ERR("cmx655d_write_regs failed %d", CMX655_COMMAND);
            return ret;
        }
    }

    ret = cmx655d_startup(dev);

    return ret;
}

static const struct cmx655d_driver_api driver_api = {
    .startup = cmx655d_startup,

    .fmt_set = cmx655d_sai_fmt_set,
    .sysclk_set = cmx655d_sai_sysclk_set,
    .chan_en_dis = cmx655d_chan_enable_disable,

    .prepare = cmx655d_prepare,
    .hw_params_set = cmx655d_hw_params_set,
    .shutdown = cmx655d_sai_shutdown,

    .startclk = cmx655d_startclk,
    .stopclk = cmx655d_stopclk,

    .param_get = cmx655d_param_get,
    .param_set = cmx655d_param_set,

    .main_clock_config = cmx655d_main_clock_config,
};

#define DEVICE_INSTANCE(inst) \
\
const static struct cmx655d_config cmx655d_##inst##_cfg = { \
	/*.i2c_master_name = DT_INST_BUS_LABEL(inst), \
	.i2c_addr = DT_INST_REG_ADDR(inst), */\
    .i2c_master = I2C_DT_SPEC_INST_GET(inst),		       \
	IF_ENABLED(CONFIG_CMX655D_INTERRUPT, (			\
	    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, interrupt_gpios), (	\
            /*.int_gpio_port = DT_INST_GPIO_LABEL(inst, interrupt_gpios),	\
            .int_gpio_pin = DT_INST_GPIO_PIN(inst, interrupt_gpios),	\
            .int_gpio_flags = DT_INST_GPIO_FLAGS(inst, interrupt_gpios),*/	\
            .int_gpio = GPIO_DT_SPEC_INST_GET(inst, interrupt_gpios), \
	))))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, reset_gpios), (	\
        /*.reset_gpio_port = DT_INST_GPIO_LABEL(inst, reset_gpios),	\
        .reset_gpio_pin = DT_INST_GPIO_PIN(inst, reset_gpios),	\
        .reset_gpio_flags = DT_INST_GPIO_FLAGS(inst, reset_gpios),*/	\
        .reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios), \
    ))								\
};\
\
static struct cmx655d_data cmx655d_##inst##_drvdata = { \
}; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		cmx655d_init,									\
		device_pm_control_nop,							\
		&cmx655d_##inst##_drvdata,						\
		&cmx655d_##inst##_cfg,							\
		APPLICATION, CONFIG_CMX655D_INIT_PRIORITY,		\
		&driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
