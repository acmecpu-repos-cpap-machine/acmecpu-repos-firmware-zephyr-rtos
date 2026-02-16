/*
 * Copyright (c) 2021 Acme CPU
 */

#define DT_DRV_COMPAT st_stm32g4ucpd

#include <errno.h>
#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/adc.h>
#include "stm32g4_ucpd.h"
#define LOG_LEVEL CONFIG_STM32G4UCPD_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(stm32g4ucpd);

#include "usbpd.h"
#include "usbpd_hw_if.h"
#include "usbpd_dpm_user.h"

#define LOW		0
#define HIGH	1

/* VBUS ADC related definitions */
#define VOLTAGE_FB_ADC_RESOLUTION		12
#define ADC_GAIN						ADC_GAIN_1
#define ADC_REFERENCE					ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME			ADC_ACQ_TIME_DEFAULT

struct vbus_ucpd_sense g_vbus_sense;

/** Configuration data */
struct stm32g4ucpd_config {
	/* VBUS ADC channel definition */
	const char *vbus_adc_name;
	uint8_t vbus_adc_ch;

	/* UCPD Interrupts */
	int irq_no;
	int irq_priority;
};

struct stm32g4ucpd_data {
	struct k_sem lock;
};


/* Init function */
static int stm32g4ucpd_init(const struct device *dev) {
	const struct stm32g4ucpd_config *config = dev->config;
	struct stm32g4ucpd_data *data = dev->data;
	int ret = 0;

	k_sem_init(&data->lock, 1, 1);

//	IRQ_CONNECT(config->irq_no, config->irq_priority, UCPD1_IRQHandler, dev, 0);
//	irq_enable(config->irq_no);

	IRQ_CONNECT(UCPD1_IRQn, 4, USBPD_PORT0_IRQHandler, NULL, 0);
	irq_enable(UCPD1_IRQn);

	/* Initialize VBUS ADC data */
	g_vbus_sense.dev = device_get_binding(config->vbus_adc_name);
	if (g_vbus_sense.dev == NULL) {
		LOG_ERR("Could not get %s device", config->vbus_adc_name);
		return -ENODEV;
	}

	g_vbus_sense.chcfg.gain 			= ADC_GAIN;
	g_vbus_sense.chcfg.reference        = ADC_REFERENCE;
	g_vbus_sense.chcfg.acquisition_time = ADC_ACQUISITION_TIME;
	g_vbus_sense.chcfg.channel_id       = config->vbus_adc_ch;
	g_vbus_sense.chcfg.differential 	= 0;

	g_vbus_sense.seq.options = NULL;
	g_vbus_sense.seq.channels = BIT(config->vbus_adc_ch);// bit mask of channels to read
	g_vbus_sense.seq.buffer = NULL;		// to be assigned in runtime
	g_vbus_sense.seq.buffer_size = 0;	// to be assigned in runtime
	g_vbus_sense.seq.resolution = VOLTAGE_FB_ADC_RESOLUTION; // desired resolution
	g_vbus_sense.seq.oversampling = 0;	// don't oversample
	g_vbus_sense.seq.calibrate = 0;		// don't calibrate


	ret = MX_USBPD_Init();
	if (ret != USBPD_OK) {
		LOG_ERR("MX_USBPD_Init failed!");
		return -1;
	}

	return 0;
}

void callback_handlers_set(const struct device *dev,
		stm32g4_ucpd_post_notif_msg_handler_t PtrPost,
		stm32g4_ucpd_fmt_send_notif_handler_t PtrFormatSend,
		stm32g4_ucpd_save_info_handler_t PtrSaveInfo) {
	USBPD_DPM_SetNotification_APP(PtrPost, PtrFormatSend, PtrSaveInfo);
}

int source_capability_get_request(const struct device *dev) {
	return USBPD_DPM_RequestGetSourceCapability(0);
}

int power_profile_request(const struct device *dev, uint8_t index_src_pdo, uint16_t requested_voltage) {
	return USBPD_DPM_RequestMessageRequest(0, index_src_pdo, requested_voltage);
}

static const struct stm32g4ucpd_driver_api stm32g4ucpd_drv_api_funcs = {
		.callbacks_set = callback_handlers_set,
		.src_capability_req = source_capability_get_request,
		.power_profile_req = power_profile_request,
};


#define DEVICE_INSTANCE(inst)													\
																				\
const static struct stm32g4ucpd_config stm32g4ucpd##inst##_cfg = {				\
	.vbus_adc_name = DT_IO_CHANNELS_LABEL_BY_NAME(DT_DRV_INST(inst),vbus),		\
	.vbus_adc_ch = DT_IO_CHANNELS_INPUT_BY_NAME(DT_DRV_INST(inst), vbus),		\
	.irq_no = DT_IRQ_BY_NAME(DT_DRV_INST(inst), global, irq),					\
	.irq_priority = DT_IRQ_BY_NAME(DT_DRV_INST(inst), global, priority)			\
};																				\
																				\
static struct stm32g4ucpd_data stm32g4ucpd##inst##_drvdata;						\
																				\
DEVICE_DEFINE(UCPD, DT_INST_LABEL(inst),                        				\
		stm32g4ucpd_init, NULL,                          						\
		&stm32g4ucpd##inst##_drvdata,                                     		\
		&stm32g4ucpd##inst##_cfg,                                      			\
		POST_KERNEL, CONFIG_STM32G4UCPD_INIT_PRIORITY,          				\
		&stm32g4ucpd_drv_api_funcs);                               				\

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);

#if 0
DEVICE_DT_INST_DEFINE(inst,														\
		stm32g4ucpd_init,														\
		device_pm_control_nop,													\
		&stm32g4ucpd##inst##_drvdata,											\
		&stm32g4ucpd##inst##_cfg,												\
		POST_KERNEL, CONFIG_STM32G4UCPD_INIT_PRIORITY,							\
		&stm32g4ucpd_drv_api_funcs);
#endif
