/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 05-Oct-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef SRC_INCLUDE_APP_UCPD_APP_UCPD_EXT_CONTR_H_
#define SRC_INCLUDE_APP_UCPD_APP_UCPD_EXT_CONTR_H_

#include <stdint.h>

/**
 * Returns the negotiated voltage and current obtained from a UCPD source
 *
 * @param	mvolts[out]			ucpd negotiated voltage
 * @param	max_curr_ma[out]	max current supported by the source
 * @param	oper_curr_ma[out]	operating current of the system
 *
 * @return	0 			SUCCESS
 * 			-EBUSY		negotiation is progress (caller should check again later)
 * 			-ENOTSUP	detected source is not a UCPD source
 */
int app_ucpd_ext_contr_nego_power_get(uint32_t *mvolts, uint32_t *max_curr_ma, uint32_t *oper_curr_ma);

/**
 * Initializes the ucpd app
 * @return	0		SUCCESS
 * 			-ve 	Fail
 */
int app_ucpd_ext_contr_init();

#endif /* SRC_INCLUDE_APP_UCPD_APP_UCPD_EXT_CONTR_H_ */
