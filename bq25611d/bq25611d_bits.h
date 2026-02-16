/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_BQ25611D_BQ25611D_BITS_H_
#define MODULES_BQ25611D_BQ25611D_BITS_H_

#define BQ25611D_BIT_MASK_BATFET_DIS	(1 << 5)
#define BQ25611D_BIT_MASK_BATFET_DLY	(1 << 3)
#define BQ25611D_BIT_MASK_EN_HIZ		(1 << 7)

/* Bit masks of status 0 register */
#define BQ25611D_BIT_MASK_VBUS_STAT			((1 << 7) | (1 << 6) | (1 << 5))
#define BQ25611D_BIT_MASK_CHRG_STAT			((1 << 4) | (1 << 3))
#define BQ25611D_BIT_MASK_THERM_STAT		(1 << 1)
#define BQ25611D_BIT_MASK_VSYS_STAT			(1 << 0)

/* Bit masks of status 1 register */
#define BQ25611D_BIT_MASK_WATCHDOG_FAULT	(1 << 7)
#define BQ25611D_BIT_MASK_BOOST_FAULT		(1 << 6)
#define BQ25611D_BIT_MASK_CHRG_FAULT		((1 << 5) | (1 << 4))
#define BQ25611D_BIT_MASK_BAT_FAULT			(1 << 3)
#define BQ25611D_BIT_MASK_NTC_FAULT			((1 << 2) | (1 << 1) | (1 << 0))

/* Bit masks of status 2 register */
#define BQ25611D_BIT_MASK_VBUS_GD				(1 << 7)
#define BQ25611D_BIT_MASK_VINDPM_STAT			(1 << 6)
#define BQ25611D_BIT_MASK_IINDPM_STAT			(1 << 5)
#define BQ25611D_BIT_MASK_BATSNS_STAT			(1 << 4)
#define BQ25611D_BIT_MASK_TOPOFF_ACTIVE			(1 << 3)
#define BQ25611D_BIT_MASK_ACOV_STAT				(1 << 2)
#define BQ25611D_BIT_MASK_VINDPM_INT_MASK		(1 << 1)
#define BQ25611D_BIT_MASK_IINDPM_INT_MASK		(1 << 0)

#endif /* MODULES_BQ25611D_BQ25611D_BITS_H_ */
