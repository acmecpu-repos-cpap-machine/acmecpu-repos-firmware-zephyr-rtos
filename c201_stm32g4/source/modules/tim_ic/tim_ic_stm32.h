/*
 * Copyright (c) 2021 Acme CPU
 *
 */

#ifndef MODULES_TIM_IC_TIM_IC_STM32_H_
#define MODULES_TIM_IC_TIM_IC_STM32_H_

#if 0
/** TIM IC data. */
struct tim_ic_stm32_data {
	/** Timer clock (Hz). */
	uint32_t tim_clk;
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

#endif

#endif /* MODULES_TIM_IC_TIM_IC_STM32_H_ */
