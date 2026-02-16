/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 22-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef MODULES_STUSB4500_STUSB4500_H_
#define MODULES_STUSB4500_STUSB4500_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define USBCPD_CHAN_BASE			(SENSOR_CHAN_PRIV_START + 200)
#define USBCPD_CHAN_UCPD_CONTR		(USBCPD_CHAN_BASE + 1)

#define USBCPD_ATTR_BASE			(SENSOR_ATTR_PRIV_START + 200)
#define USBCPD_ATTR_INIT			(USBCPD_ATTR_BASE + 1)
#define USBCPD_ATTR_SOFT_RESET		(USBCPD_ATTR_BASE + 2)

/** data structure and usage for USBCPD_ATTR_RDO
 * 	this attribute is used to get RDO which has been negotiated
 *
 * 	val[0].val1 is the PDO position which was negotiated
 * 	val[1].val1 is the RDO voltage in milli volts
 * 	val[2].val1 is the RDO max current in milli amps
 * 	val[2].val1 is the RDO operating current in milli amps
 *
 * */
#define USBCPD_ATTR_RDO				(USBCPD_ATTR_BASE + 3)

/** data structure and usage for USBCPD_ATTR_SNKPDO
 * 	this attribute is used to set / update the PDO in the STUSB4500 DPM_SNK_PDOx registers
 *
 * 	val[0].val1 is the PDO position to be updated (2 or 3)
 * 	val[1].val1 is the PDO voltage in milli volts
 * 	val[1].val2 is the PDO current in milli amps
 *
 * */
#define USBCPD_ATTR_SNKPDO			(USBCPD_ATTR_BASE + 4)
#define USBCPD_ATTR_SNKPDO_NUM		(USBCPD_ATTR_BASE + 5)

/** data structure and usage for USBCPD_ATTR_SRCPDO
 *
 * static int stusb4500_attr_get(const struct device *dev, enum sensor_channel chan,
 *				 enum sensor_attribute attr, struct sensor_value *val)
 *	{
 *	...
 *	case USBCPD_ATTR_SRCPDO:
 *		val should be passed as an array of struct sensor_value as shown below
 *		val[0].val1 is the number selectable PDOs
 *		val[1,2...].val1 gives the fixed PDO voltages
 *		val[1,2...].val2 gives the fixed PDO currents
 *	break;
 *	...
 *	}
 *
 *	...
 *		int chan = USBCPD_CHAN_UCPD_CONTR;
 *		int attr = USBCPD_ATTR_SRCPDO;
 *		struct sensor_value val[7];
 *		ret = sensor_attr_get(dev, chan, attr, val);
 *
 *	...
 * */
#define USBCPD_ATTR_SRCPDO			(USBCPD_ATTR_BASE + 6)

#define USBCPD_ATTR_STATUS			(USBCPD_ATTR_BASE + 7)
#define USBCPD_ATTR_CC_STATUS		(USBCPD_ATTR_BASE + 8)

#define USBCPD_TRIG_BASE			(SENSOR_TRIG_PRIV_START + 200)
#define USBCPD_TRIG_UCPD_INTR	(USBCPD_TRIG_BASE + 1)

struct stusb4500_config {
	struct i2c_dt_spec i2c_bus;
#ifdef CONFIG_STUSB4500_INTERRUPT
    struct gpio_dt_spec alert_gpio;		/* Alert pin */
#endif
    struct gpio_dt_spec abside_gpio;	/* A_B_SIDE pin */
    struct gpio_dt_spec reset_gpio;		/* Reset pin */
};

#define HIGH	1
#define LOW		0

struct stusb4500_data {
	struct k_sem lock;
#ifdef CONFIG_STUSB4500_INTERRUPT
	const struct device *instance;
	struct gpio_callback gpio_callback;
	struct k_work int_worker;
#endif
	const struct sensor_trigger *trigger;
	sensor_trigger_handler_t trig_handler;
};


#endif /* MODULES_STUSB4500_STUSB4500_H_ */
