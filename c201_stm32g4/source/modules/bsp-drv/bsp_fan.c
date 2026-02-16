/*
 * Copyright (c) 2021 Acme CPU
 *
 */

#include <kernel.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/pwm.h>
#include <drivers/dma.h>
#include <sys/ring_buffer.h>
#include <stdint.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bsp_gpio_driver);

#include "bsp_fan.h"

#define DT_DRV_COMPAT	bsp_fan

struct bsp_fan_cfg {
	const char *speed_ctrl_dev_name;
	uint32_t speed_ctrl_ch;
	uint32_t speed_ctrl_period;
	uint32_t speed_ctrl_flags;
	const char *speed_fb_dev_name;
	uint32_t speed_fb_ch;
	//uint32_t speed_fb_period;
	uint32_t speed_fb_flags;
};

struct bsp_fan_data {
	const struct device *speed_ctrl_dev;
	const struct device *speed_fb_dev;
	struct k_sem lock;
};

#define SPEED_FB_RING_BUF_SIZE	32
struct ring_buf speed_fb_ring_buffer;
uint16_t speed_fb_data[SPEED_FB_RING_BUF_SIZE];
uint16_t tim_ic_data;

#define FREQ_AVG	0
#define CYCLES_AVG	1
#define RAW_VAL		0

static uint64_t m_speed_fb_cycles; /** Timer clock (Hz). */
static uint32_t m_blower_freq = 0;
static uint32_t m_avg_count = 0;
static uint32_t m_period_cycles = 0;

uint32_t bsp_fan_freq_get(const struct device *dev) {
	struct bsp_fan_data *drv_data = dev->data;

	k_sem_take(&drv_data->lock, K_FOREVER);
#if FREQ_AVG
	if (m_avg_count > 0) {
		m_blower_freq = (m_blower_freq / m_avg_count);
	}
	m_avg_count = 0;
#elif CYCLES_AVG
	if (m_avg_count > 0) {
		m_period_cycles = (m_period_cycles / m_avg_count);
		m_blower_freq = (m_speed_fb_cycles / m_period_cycles);
	} else {
		m_blower_freq = 0;
	}
	m_avg_count = 0;
#elif RAW_VAL
#endif
	k_sem_give(&drv_data->lock);

	return m_blower_freq;
}

void bsp_tim_ic_cb(const struct device *dev, uint32_t pwm, uint32_t period_cycles, uint32_t pulse_cycles, int status,
		void *user_data) {

	const struct device *dev_fan = (const struct device *)user_data;
	struct bsp_fan_data *drv_data = dev_fan->data;

	k_sem_take(&drv_data->lock, K_FOREVER);
#if FREQ_AVG
	m_blower_freq += (m_speed_fb_cycles / period_cycles);
	m_avg_count++;
#elif CYCLES_AVG
	m_period_cycles += period_cycles;
	m_avg_count++;
#elif RAW_VAL
	m_blower_freq = (m_speed_fb_cycles / period_cycles);
#endif
	k_sem_give(&drv_data->lock);

	/*
	 * 	Storing time stamp data in a ring buffer
	 */
//	ring_buf_put(&speed_fb_ring_buffer, (uint8_t*) user_data, sizeof tim_ic_data);
	/*
	 * An alternative way of storing time stamp data in a buffer
	 * It might be easier to check out the functionality with this section
	 *
	 static uint8_t i = 0;
	 *(speed_fb_data + i) = *(uint16_t *)user_data;
	 i++;

	 if(i >= SPEED_FB_RING_BUF_SIZE)
	 {
	 i = 0;
	 }
	 */
	return;
}

int bsp_fan_speed_ctrl_pwm(const struct device *dev, uint8_t pwm_percent) {
	const struct bsp_fan_cfg *cfg = dev->config;
	struct bsp_fan_data *drv_data = dev->data;
	drv_data->speed_ctrl_dev = device_get_binding(cfg->speed_ctrl_dev_name);
	drv_data->speed_fb_dev = device_get_binding(cfg->speed_fb_dev_name);
	int ret = 0;

	uint32_t pwm_period = cfg->speed_ctrl_period;
	float pwm_pulse_mul = ((float)pwm_percent / 100);
	uint32_t pwm_pulse = (cfg->speed_ctrl_period * pwm_pulse_mul);
	pwm_flags_t pwm_flags = cfg->speed_ctrl_flags;

	ret = pwm_pin_set_cycles(drv_data->speed_ctrl_dev, cfg->speed_ctrl_ch,
			pwm_period, pwm_pulse, pwm_flags);
	if (ret != 0) {
		printk("bsp_fan_init: pwm_pin_set_cycles for %s, channel %d failed\n",
				drv_data->speed_ctrl_dev->name, cfg->speed_ctrl_ch);
		return -1;
	}
	return ret;
}

int bsp_fan_init(const struct device *dev) {
	const struct bsp_fan_cfg *cfg = dev->config;
	struct bsp_fan_data *drv_data = dev->data;
	drv_data->speed_ctrl_dev = device_get_binding(cfg->speed_ctrl_dev_name);
	drv_data->speed_fb_dev = device_get_binding(cfg->speed_fb_dev_name);
	int ret = 0;

	k_sem_init(&drv_data->lock, 1, 1);

//	ring_buf_init(&speed_fb_ring_buffer, SPEED_FB_RING_BUF_SIZE, speed_fb_data);

	if (drv_data->speed_fb_dev != NULL) {
		ret = pwm_pin_configure_capture(drv_data->speed_fb_dev,
				cfg->speed_fb_ch, cfg->speed_ctrl_flags, bsp_tim_ic_cb,
				(void*) dev);
		if (ret != 0) {
			printk(
					"bsp_fan_init: pwm_pin_configure_capture for %s, channel %d failed\n",
					drv_data->speed_fb_dev->name, cfg->speed_fb_ch);
			return -1;
		}

		ret = pwm_get_cycles_per_sec(drv_data->speed_fb_dev, cfg->speed_fb_ch,
				&m_speed_fb_cycles);
		if (ret != 0) {
			printk("bsp_fan_init: pwm_get_cycles_per_sec failed, %d\n", ret);
			return -1;
		}

		/*
		 const struct device *speed_fb_dma = device_get_binding("DMA_1");
		 const struct dma_driver_api *dma_api = speed_fb_dma->api;
		 struct dma_config config = {

		 };
		 dma_api->config(speed_fb_dma, 3, &config);
		 */
	}
	return 0;
}



static const struct bsp_fan_api bsp_fan_drv_api = {
		.enable = NULL,
		.disable = NULL,
		.speed_ctrl_pwm = bsp_fan_speed_ctrl_pwm,
		.speed_ctrl_i2c = NULL,
		.speed_feedback = NULL,
		.fan_freq_get = bsp_fan_freq_get,
};

#define DRV_INST(inst)\
	static const struct bsp_fan_cfg fan_##inst##_cfg = {\
        .speed_ctrl_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst, speed_ctrl),    \
		.speed_ctrl_ch = DT_INST_PWMS_CHANNEL_BY_NAME(inst, speed_ctrl),\
		.speed_ctrl_period = DT_INST_PWMS_PERIOD_BY_NAME(inst, speed_ctrl),\
		.speed_ctrl_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst, speed_ctrl),\
        .speed_fb_dev_name = DT_INST_PWMS_LABEL_BY_NAME(inst,speed_feedback),   \
		.speed_fb_ch = DT_INST_PWMS_CHANNEL_BY_NAME(inst, speed_feedback),\
		.speed_fb_flags = DT_INST_PWMS_FLAGS_BY_NAME(inst, speed_feedback),\
	};\
\
	static struct bsp_fan_data fan_##inst##_data = {\
		\
	};\
\
	DEVICE_DEFINE(fan_##inst, DT_INST_LABEL(inst), bsp_fan_init, NULL,\
					&fan_##inst##_data, &fan_##inst##_cfg,\
					APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,\
					&bsp_fan_drv_api);

#if DT_HAS_COMPAT_STATUS_OKAY(bsp_fan)
DT_INST_FOREACH_STATUS_OKAY(DRV_INST)
#endif
