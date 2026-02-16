/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Acme CPU
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_tim_ic

#include <errno.h>
#include <soc.h>
#include <drivers/pwm.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <devicetree.h>
#include <stm32_ll_rcc.h>
#include <stm32g4xx_ll_tim.h>
#include <drivers/clock_control/stm32_clock_control.h>
//#include <pinmux/stm32/pinmux_stm32.h>
#include <pinmux/pinmux_stm32.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(tim_ic, CONFIG_TIM_IC_LOG_LEVEL);

#define TIMER_MAX_CH 4u

/** TIM IC data. */
struct tim_ic_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;

	uint32_t first_capture_val[TIMER_MAX_CH];
	uint32_t second_capture_val[TIMER_MAX_CH];
};

/** TIM IC configuration. */
struct tim_ic_stm32_config {
	/** Timer instance. */
	TIM_TypeDef *timer;
	/** Prescaler. */
	uint32_t prescaler;
	/** Clock configuration. */
	struct stm32_pclken pclken;
	/** pinctrl configurations. */
	const struct soc_gpio_pinctrl *pinctrl;
	/** Number of pinctrl configurations. */
	size_t pinctrl_len;
	void (*irq_config_func)(const struct device *);
};

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2,
	LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4
};

/** Channel to input capture set function mapping. */
static void (*const tim_ic_set_ie[TIMER_MAX_CH])(TIM_TypeDef*) = {
	LL_TIM_EnableIT_CC1, LL_TIM_EnableIT_CC2, LL_TIM_EnableIT_CC3, LL_TIM_EnableIT_CC4
};

static void (*const tim_ic_clear_ie[TIMER_MAX_CH])(TIM_TypeDef*) = {
	LL_TIM_DisableIT_CC1, LL_TIM_DisableIT_CC2, LL_TIM_DisableIT_CC3, LL_TIM_DisableIT_CC4
};

static uint32_t (*const tim_ic_af_cc[TIMER_MAX_CH])(TIM_TypeDef *TIMx) = {
	LL_TIM_IsActiveFlag_CC1, LL_TIM_IsActiveFlag_CC2, LL_TIM_IsActiveFlag_CC3, LL_TIM_IsActiveFlag_CC4
};

static uint32_t (*const tim_ic_af_ovr[TIMER_MAX_CH])(TIM_TypeDef *TIMx) = {
	LL_TIM_IsActiveFlag_CC1OVR, LL_TIM_IsActiveFlag_CC2OVR, LL_TIM_IsActiveFlag_CC3OVR, LL_TIM_IsActiveFlag_CC4OVR
};

static void (*const tim_ic_cf_cc[TIMER_MAX_CH])(TIM_TypeDef *TIMx) = {
	LL_TIM_ClearFlag_CC1, LL_TIM_ClearFlag_CC2, LL_TIM_ClearFlag_CC3, LL_TIM_ClearFlag_CC4
};

static void (*const tim_ic_cf_ovr[TIMER_MAX_CH])(TIM_TypeDef *TIMx) = {
	LL_TIM_ClearFlag_CC1OVR, LL_TIM_ClearFlag_CC2OVR, LL_TIM_ClearFlag_CC3OVR, LL_TIM_ClearFlag_CC4OVR
};

static uint32_t (*const tim_ic_get_captured_val[TIMER_MAX_CH])(TIM_TypeDef *TIMx) = {
	LL_TIM_IC_GetCaptureCH1, LL_TIM_IC_GetCaptureCH2, LL_TIM_IC_GetCaptureCH3, LL_TIM_IC_GetCaptureCH4
};

static pwm_capture_callback_handler_t tim_ic_user_cb[TIMER_MAX_CH];
static void *isr_cb_user_data[TIMER_MAX_CH];
static uint8_t m_capture_index[TIMER_MAX_CH] = {0};

#if 0
/** Channel to compare set function mapping. */
static void (*const set_timer_compare[TIMER_MAX_CH])(TIM_TypeDef *,
						     uint32_t) = {
	LL_TIM_OC_SetCompareCH1, LL_TIM_OC_SetCompareCH2,
	LL_TIM_OC_SetCompareCH3, LL_TIM_OC_SetCompareCH4,
#if TIMER_HAS_6CH
	LL_TIM_OC_SetCompareCH5, LL_TIM_OC_SetCompareCH6
#endif
};

/**
 * Obtain LL polarity from PWM flags.
 *
 * @param flags PWM flags.
 *
 * @return LL polarity.
 */
static uint32_t get_polarity(pwm_flags_t flags)
{
	if ((flags & PWM_POLARITY_MASK) == PWM_POLARITY_NORMAL) {
		return LL_TIM_OCPOLARITY_HIGH;
	}

	return LL_TIM_OCPOLARITY_LOW;
}
#endif

/**
 * Obtain timer clock speed.
 *
 * @param pclken  Timer clock control subsystem.
 * @param tim_clk Where computed timer clock will be stored.
 *
 * @return 0 on success, error code otherwise.
 */
static int get_tim_clk(const struct stm32_pclken *pclken, uint32_t *tim_clk)
{
	int r;
	const struct device *clk;
	uint32_t bus_clk, apb_psc;

	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	__ASSERT_NO_MSG(clk);

	r = clock_control_get_rate(clk, (clock_control_subsys_t *)pclken,
				   &bus_clk);
	if (r < 0) {
		return r;
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X)
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE1;
	} else {
		apb_psc = CONFIG_CLOCK_STM32_D2PPRE2;
	}
#else
	if (pclken->bus == STM32_CLOCK_BUS_APB1) {
		apb_psc = STM32_APB1_PRESCALER;	//CONFIG_CLOCK_STM32_APB1_PRESCALER;
	}
#if !defined(CONFIG_SOC_SERIES_STM32F0X) && !defined(CONFIG_SOC_SERIES_STM32G0X)
	else {
		apb_psc = STM32_APB2_PRESCALER;	//CONFIG_CLOCK_STM32_APB2_PRESCALER;
	}
#endif
#endif

#if defined(RCC_DCKCFGR_TIMPRE) || defined(RCC_DCKCFGR1_TIMPRE) || \
	defined(RCC_CFGR_TIMPRE)
	/*
	 * There are certain series (some F4, F7 and H7) that have the TIMPRE
	 * bit to control the clock frequency of all the timers connected to
	 * APB1 and APB2 domains.
	 *
	 * Up to a certain threshold value of APB{1,2} prescaler, timer clock
	 * equals to HCLK. This threshold value depends on TIMPRE setting
	 * (2 if TIMPRE=0, 4 if TIMPRE=1). Above threshold, timer clock is set
	 * to a multiple of the APB domain clock PCLK{1,2} (2 if TIMPRE=0, 4 if
	 * TIMPRE=1).
	 */

	if (LL_RCC_GetTIMPrescaler() == LL_RCC_TIM_PRESCALER_TWICE) {
		/* TIMPRE = 0 */
		if (apb_psc <= 2u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 2u;
		}
	} else {
		/* TIMPRE = 1 */
		if (apb_psc <= 4u) {
			LL_RCC_ClocksTypeDef clocks;

			LL_RCC_GetSystemClocksFreq(&clocks);
			*tim_clk = clocks.HCLK_Frequency;
		} else {
			*tim_clk = bus_clk * 4u;
		}
	}
#else
	/*
	 * If the APB prescaler equals 1, the timer clock frequencies
	 * are set to the same frequency as that of the APB domain.
	 * Otherwise, they are set to twice (×2) the frequency of the
	 * APB domain.
	 */
	if (apb_psc == 1u) {
		*tim_clk = bus_clk;
	} else {
		*tim_clk = bus_clk * 2u;
	}
#endif

	return 0;
}

static int is_timer_cc_instance(const struct device *dev, uint32_t pwm) {
	const struct tim_ic_stm32_config *cfg = dev->config;
	switch (pwm) {
	case 1:
		if (!IS_TIM_CC1_INSTANCE(cfg->timer)) {
			LOG_ERR("Invalid channel (#%d) for timer %s", pwm, dev->name);
			return -EINVAL;
		}
		break;
	case 2:
		if (!IS_TIM_CC2_INSTANCE(cfg->timer)) {
			LOG_ERR("Invalid channel (#%d) for timer %s", pwm, dev->name);
			return -EINVAL;
		}
		break;
	case 3:
		if (!IS_TIM_CC3_INSTANCE(cfg->timer)) {
			LOG_ERR("Invalid channel (#%d) for timer %s", pwm, dev->name);
			return -EINVAL;
		}
		break;
	case 4:
		if (!IS_TIM_CC4_INSTANCE(cfg->timer)) {
			LOG_ERR("Invalid channel (#%d) for timer %s", pwm, dev->name);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Invalid channel (#%d) for timer %s", pwm, dev->name);
		return -EINVAL;
	}
	return 0;
}

static int tim_ic_stm32_pin_enable_capture(const struct device *dev, uint32_t pwm) {
	const struct tim_ic_stm32_config *cfg = dev->config;

	int ret = is_timer_cc_instance(dev, pwm);
	if (ret != 0) {
		LOG_ERR("Timer is not an instance of input capture");
		return ret;
	}

	/* Start the input capture */

	/* Clear the status register */
	LL_TIM_ClearFlag_UPDATE(cfg->timer);

	/* Clear the Capture/Compare interrupt flag (CCxF) */
	tim_ic_cf_cc[pwm - 1](cfg->timer);

	/* Enable the capture/compare interrupt (CCxIE) */
	tim_ic_set_ie[pwm - 1](cfg->timer);		//LL_TIM_EnableIT_CCX(cfg->timer);

	return 0;
}

static int tim_ic_stm32_pin_disable_capture(const struct device *dev, uint32_t pwm) {
	const struct tim_ic_stm32_config *cfg = dev->config;

	int ret = is_timer_cc_instance(dev, pwm);
	if (ret != 0) {
		LOG_ERR("Timer is not an instance of input capture");
		return ret;
	}

	/* Stop the input capture */

	/* Clear the status register */
	LL_TIM_ClearFlag_UPDATE(cfg->timer);

	/* Clear the Capture/Compare interrupt flag (CCxF) */
	tim_ic_cf_cc[pwm - 1](cfg->timer);

	/* Disable capture/compare interrupt (CCxIE) */
	tim_ic_clear_ie[pwm -1 ](cfg->timer);

	return 0;
}

static int tim_ic_stm32_pin_config_capture(const struct device *dev,\
								uint32_t pwm, pwm_flags_t flags,\
								pwm_capture_callback_handler_t cb,\
							    void *user_data)
{
	const struct tim_ic_stm32_config *cfg = dev->config;
	uint32_t channel;

	int ret = is_timer_cc_instance(dev, pwm);
	if (ret != 0) {
		LOG_ERR("Timer is not an instance of input capture");
		return ret;
	}

	channel = ch2ll[pwm - 1u];

/*
  uint32_t ICPolarity;     !< Specifies the active edge of the input signal.
                           This parameter can be a value of @ref TIM_LL_EC_IC_POLARITY.
                           This feature can be modified afterwards using unitary function @ref LL_TIM_IC_SetPolarity().
  uint32_t ICActiveInput;  !< Specifies the input.
                           This parameter can be a value of @ref TIM_LL_EC_ACTIVEINPUT.
                           This feature can be modified afterwards using unitary function @ref LL_TIM_IC_SetActiveInput().
  uint32_t ICPrescaler;    !< Specifies the Input Capture Prescaler.
                           This parameter can be a value of @ref TIM_LL_EC_ICPSC.
                           This feature can be modified afterwards using unitary function @ref LL_TIM_IC_SetPrescaler().
  uint32_t ICFilter;       !< Specifies the input capture filter.
                           This parameter can be a value of @ref TIM_LL_EC_IC_FILTER.
                           This feature can be modified afterwards using unitary function @ref LL_TIM_IC_SetFilter().

 */
	LL_TIM_IC_InitTypeDef ic_init;
	LL_TIM_IC_StructInit(&ic_init);

	ic_init.ICPrescaler = LL_TIM_ICPSC_DIV1;		/*!< No prescaler, capture is done each time an edge is detected on the capture input */
	ic_init.ICPolarity = LL_TIM_IC_POLARITY_RISING;	/*!< The circuit is sensitive to TIxFP1 rising edge, TIxFP1 is not inverted */
	ic_init.ICFilter = LL_TIM_IC_FILTER_FDIV1;//L_TIM_IC_FILTER_FDIV1_N8;	/*!< fSAMPLING=fCK_INT, N=8 */;
	ic_init.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;	/*!< ICx is mapped on TIx */

	if (LL_TIM_IC_Init(cfg->timer, channel, &ic_init) != SUCCESS)
	{
		LOG_ERR("Could not initialize timer channel input capture");
		return -EIO;
	}

	LL_TIM_EnableARRPreload(cfg->timer);
//	LL_TIM_OC_EnablePreload(cfg->timer, channel);
	LL_TIM_SetAutoReload(cfg->timer, 0xFFFF);
	LL_TIM_GenerateEvent_UPDATE(cfg->timer);

	tim_ic_user_cb[pwm - 1] = cb;
	isr_cb_user_data[pwm - 1] = user_data;

#if 0
	if (IS_TIM_DMA_INSTANCE(cfg->timer))
	{
		LL_TIM_EnableDMAReq_CC1(cfg->timer);
	}
#endif

	ret = tim_ic_stm32_pin_enable_capture(dev, pwm);
	if (ret != 0) {
		LOG_ERR("Could not enable input capture on %s, %d", dev->name, ret);
		return ret;
	}

	return 0;
}

static int tim_ic_stm32_get_cycles_per_sec(const struct device *dev,
					uint32_t pwm,
					uint64_t *cycles)
{
	struct tim_ic_stm32_data *data = dev->data;
	const struct tim_ic_stm32_config *cfg = dev->config;

	*cycles = (uint64_t)(data->tim_clk / (cfg->prescaler + 1));

	return 0;
}

static const struct pwm_driver_api tim_ic_stm32_driver_api = {
	.get_cycles_per_sec = tim_ic_stm32_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.pin_configure_capture = tim_ic_stm32_pin_config_capture,
	.pin_enable_capture = tim_ic_stm32_pin_enable_capture,
	.pin_disable_capture = tim_ic_stm32_pin_disable_capture,
#endif
};

static int tim_ic_stm32_init(const struct device *dev)
{
	int r;
	struct tim_ic_stm32_data *data = dev->data;
	const struct tim_ic_stm32_config *cfg = dev->config;
	const struct device *clk;
	LL_TIM_InitTypeDef init;

	cfg->irq_config_func(dev);

	/* enable clock and store its speed */
	clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	__ASSERT_NO_MSG(clk);

	r = clock_control_on(clk, (clock_control_subsys_t *)&cfg->pclken);
	if (r < 0) {
		LOG_ERR("Could not initialize clock (%d)", r);
		return r;
	}

	r = get_tim_clk(&cfg->pclken, &data->tim_clk);
	if (r < 0) {
		LOG_ERR("Could not obtain timer clock (%d)", r);
		return r;
	}

	/* configure pinmux */
	r = stm32_dt_pinctrl_configure(cfg->pinctrl,
				       cfg->pinctrl_len,
				       (uint32_t)cfg->timer);
	if (r < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", r);
		return r;
	}

	/* initialize timer */
	LL_TIM_StructInit(&init);

	init.Prescaler = cfg->prescaler;
	init.CounterMode = LL_TIM_COUNTERMODE_UP;
	init.Autoreload = 0xFFFFu;
	init.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;

	if (LL_TIM_Init(cfg->timer, &init) != SUCCESS) {
		LOG_ERR("Could not initialize timer");
		return -EIO;
	}
	LL_TIM_EnableCounter(cfg->timer);

	return 0;
}

void stm32_tim_isr(void *arg)
{
	const struct device *dev = (struct device *) arg;
	const struct tim_ic_stm32_config *cfg = dev->config;
	struct tim_ic_stm32_data *data = dev->data;
	//TIM_TypeDef *timer = cfg->timer;

	uint8_t max_i = 0;

	/* FIXME: if else logic and max_i value needs to be fixed */
	if (IS_TIM_CC1_INSTANCE(cfg->timer))
	{
		max_i = 1;
	}
	else if (IS_TIM_CC2_INSTANCE(cfg->timer))
	{
		max_i = 2;
	}
	else if (IS_TIM_CC3_INSTANCE(cfg->timer))
	{
		max_i = 3;
	}
	else if (IS_TIM_CC4_INSTANCE(cfg->timer))
	{
		max_i = 4;
	}

	for (uint8_t i = 0; i < max_i; i++)
	{
		if(tim_ic_af_cc[i])
		{
			/* Indicate whether Capture/Compare n over-capture interrupt flag (CCxOF) is set */
			if(tim_ic_af_ovr[i](cfg->timer))
			{
				/* Clear the Capture/Compare n over-capture interrupt flag (CCxOF). */
				tim_ic_cf_ovr[i](cfg->timer);
			}
			/* Clear the Capture/Compare n interrupt flag (CCxF) */
			tim_ic_cf_cc[i](cfg->timer);

			/* Capture first edge */
			if (m_capture_index[i] == 0)
			{
				/* Get captured value for input channel n */
				data->first_capture_val[i] = tim_ic_get_captured_val[i](cfg->timer);

				m_capture_index[i] = 1;
			}
			/* Capture second edge */
			else if (m_capture_index[i] == 1)
			{
				/* Get captured value for input channel n */
				data->second_capture_val[i] = tim_ic_get_captured_val[i](cfg->timer);

				/* Capture period computation */
				uint32_t capture_period_cycles = 0;
				if (data->second_capture_val[i] > data->first_capture_val[i])
				{
					capture_period_cycles = data->second_capture_val[i] - data->first_capture_val[i];
				}
				else if(data->second_capture_val[i] < data->first_capture_val[i])
				{
					/* 0xFFFF is max value of timers in 16-bit mode */
					capture_period_cycles = ((0xFFFF - data->first_capture_val[i]) + data->second_capture_val[i]) + 1;
				}
				else
				{
					/* If captures are equal we have reached the limit of frequency measures */
					m_capture_index[i] = 0;
					return;
				}

				m_capture_index[i] = 0;

				/* Call the user callback */
				if (tim_ic_user_cb[i] != NULL) {
					tim_ic_user_cb[i](dev, (i + 1), capture_period_cycles, 0, 0, isr_cb_user_data[i]);
				}
			}
		}
	}
	return;
}

#define STM32_TIM_IRQ_CONNECT_AND_ENABLE(idx)				\
\
	do {								\
		IRQ_CONNECT(DT_IRQN(DT_PARENT(DT_DRV_INST(idx))), DT_IRQ(DT_PARENT(DT_DRV_INST(idx)), priority),						\
					stm32_tim_isr, DEVICE_GET(tim_ic_stm32_##idx), 0);			\
					irq_enable(DT_IRQN(DT_PARENT(DT_DRV_INST(idx))));									\
		} while (0);															\
	return

#define STM32_TIM_IRQ_HANDLER_DECL(idx)				\
		static void tim_stm32_irq_config_func_##idx(const struct device *dev)

#define STM32_TIM_IRQ_HANDLER_FUNCTION(idx)				\
		.irq_config_func = tim_stm32_irq_config_func_##idx,

#define STM32_TIM_IRQ_HANDLER(idx)												\
		static void tim_stm32_irq_config_func_##idx(const struct device *dev)	\
		{																		\
			STM32_TIM_IRQ_CONNECT_AND_ENABLE(idx);								\
		}

#define DT_INST_CLK(index, inst)                                               	\
	{                                                                      		\
		.bus = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bus),     			\
		.enr = DT_CLOCKS_CELL(DT_PARENT(DT_DRV_INST(index)), bits)     			\
	}

#define TIM_IC_DEVICE_INIT(index)                                               \
\
	STM32_TIM_IRQ_HANDLER_DECL(index);\
\
	static struct tim_ic_stm32_data tim_ic_stm32_data_##index;                  \
\
	static const struct soc_gpio_pinctrl tim_ic_pins_##index[] =	       		\
		ST_STM32_DT_INST_PINCTRL(index, 0);			       						\
\
	static const struct tim_ic_stm32_config tim_ic_stm32_config_##index = {     \
		.timer = (TIM_TypeDef *)DT_REG_ADDR(                           			\
			DT_PARENT(DT_DRV_INST(index))),                        				\
		.prescaler = DT_INST_PROP(index, st_prescaler),                			\
		.pclken = DT_INST_CLK(index, timer),                           			\
		.pinctrl = tim_ic_pins_##index,                                   		\
		.pinctrl_len = ARRAY_SIZE(tim_ic_pins_##index),                   		\
		STM32_TIM_IRQ_HANDLER_FUNCTION(index)\
	};                                                                     		\
\
	DEVICE_DEFINE(tim_ic_stm32_##index, DT_INST_LABEL(index),           		\
		tim_ic_stm32_init, NULL, &tim_ic_stm32_data_##index,          			\
		&tim_ic_stm32_config_##index, POST_KERNEL,            					\
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,                						\
		&tim_ic_stm32_driver_api);\
\
	STM32_TIM_IRQ_HANDLER(index)

#ifdef CONFIG_PWM_CAPTURE
	DT_INST_FOREACH_STATUS_OKAY(TIM_IC_DEVICE_INIT)
#endif /* CONFIG_PWM_CAPTURE */
