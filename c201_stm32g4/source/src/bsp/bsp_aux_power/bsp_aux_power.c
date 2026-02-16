/*
 * Copyright (c) 2021 Acme CPU
 */

#include <zephyr.h>
#include "load_switch_tps22810.h"

int bsp_aux_power_enable() {
#if LOAD_SWITCH_TPS22810
	return tps22810_ps_enable();
#endif
	return 0;
}

int bsp_aux_power_disable() {
#if LOAD_SWITCH_TPS22810
	return tps22810_ps_disable();
#endif
	return 0;
}
