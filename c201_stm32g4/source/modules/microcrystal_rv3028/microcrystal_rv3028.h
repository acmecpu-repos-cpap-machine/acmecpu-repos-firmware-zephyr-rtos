/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_MICROCRYSTAL_RV3028_MICROCRYSTAL_RV3028_H_
#define MODULES_MICROCRYSTAL_RV3028_MICROCRYSTAL_RV3028_H_

#include <time.h>

#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/notify.h>

/*
 * Real-time clock control based on the RV3028 counter API.
 *
 * The [Micro Crystal RV3028]
 * (https://www.microcrystal.com/fileadmin/Media/Products/RTC/Datasheet/RV-3028-C7.pdf)
 * is a high-precision extreme low power real-time clock
 *
 * The core Zephyr API to this device is as a counter, with the
 * following limitations:
 * * counter_read() and counter_*_alarm() cannot be invoked from
 *   interrupt context, as they require communication with the device
 *   over an I2C bus.
 * * many other counter APIs, such as start/stop/set_top_value are not
 *   supported as the clock is always running.
 * * two alarm channels are supported but are not equally capable:
 *   channel 0 supports alarms at 1 s resolution, while channel 1
 *   supports alarms at 1 minute resolution.
 *
 * Most applications for this device will need to use the extended
 * functionality exposed by this header to access the real-time-clock
 * features.  The majority of these functions must be invoked from
 * supervisor mode.
 */


/**
 * __read_poll_timeout - Periodically poll an address until a condition is
 *			met or a timeout occurs
 * @op: accessor function (takes @args as its arguments)
 * @cond: Break condition (usually involving @val)
 * @sleep_us: Maximum time to sleep between reads in us (0
 *            tight-loops).
 * @timeout_us: Timeout in us, 0 means never timeout, minimum timeout should be 1000us
 * @sleep_before_read: if it is true, sleep @sleep_us before read.
 * @args: arguments for @op poll
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout. In either
 * case, the last read value at @args is stored in @val. Must not
 * be called from atomic context if sleep_us or timeout_us are used.
 *
 * When available, you'll probably want to use one of the specialized
 * macros defined below rather than this macro directly.
 *
 * This is derived from read_poll_timeout in linux/iopoll.h
 */
#define __read_poll_timeout(op, cond, sleep_us, timeout_us, sleep_before_read, args...) \
({ \
		int64_t __timeout_first_ms = k_uptime_get(); \
		int64_t __timeout_ms = (timeout_us/1000); \
		int64_t __start = 0; \
		uint32_t __sleep_us = sleep_us; \
		if (sleep_before_read && __sleep_us) \
			k_usleep(__sleep_us); \
	 	while (1) \
		{ \
	 		op(args); \
			if (cond) \
				break; \
 \
		__start = __timeout_first_ms; \
		if (__timeout_ms && (k_uptime_delta(&__start) > __timeout_ms)) \
			break; \
 \
		if (__sleep_us) \
			k_usleep(__sleep_us); \
	 	} \
 \
		(cond) ? 0 : -ETIMEDOUT; \
})

/**
 * read_poll_timeout - Poll until a condition is met or a timeout occurs
 *
 * @dev: Device to read from
 * @reg: Register to poll
 * @val: uint8_t variable to read the value into
 * @cond: Break condition (usually involving @val)
 * @sleep_us: Maximum time to sleep between reads in us (0
 *            tight-loops).
 * @timeout_us: Timeout in us, 0 means never timeout, minimum timeout should be 1000us
 *
 * Returns 0 on success and -ETIMEDOUT upon a timeout or the read
 * error return value in case of a error read. In the two former cases,
 * the last read value at @reg is stored in @val. Must not be called
 * from atomic context if sleep_us or timeout_us are used.
 *
 * This is derived from regmap_read_poll_timeout in linux/regmap.h
 */
#define read_poll_timeout(dev, reg, val, cond, sleep_us, timeout_us) \
({ \
	int __tmp; \
	__tmp = __read_poll_timeout(read_regs_i2c, cond, sleep_us, timeout_us, false, (dev), (reg), (&val)); \
	__tmp; \
})


/* Function declarations */

/*
 * rv3028_activate_vbackup - This function must be called from the
 * application just before the main power Vdd turned off. This function
 * sets the Vbackup power to be enabled and prevents threads from
 * performing any I2C operation further.
 * */
int rv3028_activate_vbackup(const struct device *dev);

int rv3028_set_time(const struct device *dev, struct tm *tm);



#endif /* MODULES_MICROCRYSTAL_RV3028_MICROCRYSTAL_RV3028_H_ */
