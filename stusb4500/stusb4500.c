/*
 * Copyright (c) 2023 Acme CPU
 *
 *  Created on: 22-Sept-2023
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#define DT_DRV_COMPAT st_stusb4500

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#define LOG_LEVEL CONFIG_STUSB4500_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stusb4500);

#include "stusb4500.h"
#include "USB_PD_core.h"

uint8_t connection_flag[USBPORT_MAX]={1};
uint32_t VBUS_Current_limitation[USBPORT_MAX] = {5};
uint32_t Previous_VBUS_Current_limitation[USBPORT_MAX];
uint8_t USB_PD_Interupt_Flag[USBPORT_MAX];
uint8_t USB_PD_Interupt_PostponedFlag[USBPORT_MAX];

/* PDO Variables */
extern USB_PD_StatusTypeDef PD_status[USBPORT_MAX] ;
extern USB_PD_SNK_PDO_TypeDef PDO_SNK[USBPORT_MAX][3];
extern USB_PD_SRC_PDOTypeDef PDO_FROM_SRC[USBPORT_MAX][7];
extern uint8_t PDO_FROM_SRC_Valid[USBPORT_MAX];
extern uint8_t PDO_FROM_SRC_Num_Sel[USBPORT_MAX];
extern uint8_t PDO_FROM_SRC_Num[USBPORT_MAX];
extern uint8_t Policy_Engine_State[USBPORT_MAX];
extern uint8_t Go_disable_once[USBPORT_MAX];
extern uint8_t Final_Nego_done[USBPORT_MAX] ;
extern uint8_t Core_Process_suspended ;


static int Usb_Port = 0;

static int stusb4500_sample_fetch(const struct device *dev,
		enum sensor_channel chan)
{
	int ret = 0;
	return ret;
}

static int stusb4500_channel_get(const struct device *dev,
		enum sensor_channel chan, struct sensor_value *valp)
{
	int ret = 0;
	return ret;
}

static int stusb4500_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct stusb4500_config *config = dev->config;
	int ret = 0;
	int prop = (int) attr;

	switch (prop) {
	case USBCPD_ATTR_INIT:
		ret = usb_pd_init(config, Usb_Port);
		break;
	case USBCPD_ATTR_SOFT_RESET:
		ret = Send_Soft_reset_Message(config, Usb_Port);
		break;
	case USBCPD_ATTR_SNKPDO:
		uint8_t pdo_number = (&val[0])->val1;
		int mvolts = (&val[1])->val1;
		int mamps = (&val[1])->val2;
		ret = Update_PDO(config, Usb_Port, pdo_number, mvolts, mamps);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static int stusb4500_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	const struct stusb4500_config *config = dev->config;
	int ret = 0;
	int prop = (int) attr;

	switch (prop) {
	case USBCPD_ATTR_RDO:
	{
		ret = Read_RDO(config, Usb_Port, val);
//		Print_RDO(config, Usb_Port);
	}
		break;
	case USBCPD_ATTR_SRCPDO:
//		port_message_rx(config, Usb_Port);
		ret = Print_PDO_FROM_SRC(config, Usb_Port, val);
		break;
	case USBCPD_ATTR_STATUS:
		break;
	case USBCPD_ATTR_CC_STATUS:
		Print_Type_C_Only_Status(config, Usb_Port);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

#if (CONFIG_STUSB4500_INTERRUPT)
static void stusb4500_interrupt_worker(struct k_work *work)
{
	struct stusb4500_data *stusb = CONTAINER_OF(work, struct stusb4500_data, int_worker);
	const struct stusb4500_config *config = stusb->instance->config;

	k_sem_take(&stusb->lock, K_FOREVER);
//	k_sleep(K_MSEC(500));	// wait before we read the statuses
	ALARM_MANAGEMENT(config, Usb_Port);
	k_sem_give(&stusb->lock);

	if (stusb->trig_handler != NULL) {
		stusb->trig_handler(stusb->instance, stusb->trigger);
	}

//irq_out:
	return;
}

static void stusb4500_interrupt_callback(const struct device *dev, struct gpio_callback *cb,
											gpio_port_pins_t pins)
{
	struct stusb4500_data *const data = CONTAINER_OF(cb, struct stusb4500_data, gpio_callback);

	ARG_UNUSED(pins);

	/* Cannot read registers from ISR context, queue worker */
	k_work_submit(&data->int_worker);
}


static int stusb4500_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				    				sensor_trigger_handler_t handler)
{
	struct stusb4500_data *stusb = dev->data;

	if (trig->type == USBCPD_TRIG_UCPD_INTR) {
		stusb->trig_handler = handler;
		if (handler == NULL) {
			return 0;
		}
		stusb->trigger = trig;
	}
	return 0;
}
#endif /* CONFIG_STUSB4500_INTERRUPT */

static int stusb4500_init(const struct device *dev)
{
	const struct stusb4500_config *config = dev->config;
	struct stusb4500_data *data = dev->data;
	int ret = 0;

   	if (!device_is_ready(config->i2c_bus.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	k_sem_init(&data->lock, 1, 1);

	/* interrupt configuration */
#ifdef CONFIG_STUSB4500_INTERRUPT
	data->instance = dev;
	k_work_init(&data->int_worker, stusb4500_interrupt_worker);

	if (!device_is_ready(config->alert_gpio.port)) {
		LOG_ERR("INT device is not ready");
//		return -ENODEV;
	} else {
		ret = gpio_pin_configure(config->alert_gpio.port, config->alert_gpio.pin,
				(GPIO_INPUT | config->alert_gpio.dt_flags));
		ret |= gpio_pin_interrupt_configure(config->alert_gpio.port,
				config->alert_gpio.pin, (GPIO_INT_EDGE_FALLING));
		if (ret != 0) {
			LOG_ERR("Failed to configure %s pin %d (%d)",
					config->alert_gpio.port->name, config->alert_gpio.pin, ret);
			return ret;
		}

		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&data->gpio_callback, stusb4500_interrupt_callback,
				BIT(config->alert_gpio.pin));
		gpio_add_callback(config->alert_gpio.port, &data->gpio_callback);
	}
#endif

	  USB_PD_Interupt_PostponedFlag[0]= 0; /* this flag is 1 if I2C is busy when Alert signal raise */
	  USB_PD_Interupt_Flag[Usb_Port] = 1;
	  Final_Nego_done[Usb_Port]=0;
	  Go_disable_once[Usb_Port] = 0;
	  connection_flag[Usb_Port] =1;
	  Previous_VBUS_Current_limitation[Usb_Port] = VBUS_Current_limitation[Usb_Port];

	  memset((uint32_t *)PDO_FROM_SRC[Usb_Port], 0, 7);
	  memset((uint32_t *)PDO_SNK[Usb_Port], 0, 3);


	return ret;
}

static const struct sensor_driver_api stusb4500_driver_api = {
	.sample_fetch = stusb4500_sample_fetch,
	.channel_get = stusb4500_channel_get,
	.attr_set = stusb4500_attr_set,
	.attr_get = stusb4500_attr_get,
#ifdef CONFIG_STUSB4500_INTERRUPT
	.trigger_set = stusb4500_trigger_set,
#endif
};

#define DEVICE_INSTANCE(inst) \
\
const static struct stusb4500_config stusb4500_##inst##_cfg = { \
	.i2c_bus = I2C_DT_SPEC_INST_GET(inst),		       \
	IF_ENABLED(CONFIG_STUSB4500_INTERRUPT, (			\
	    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, alert_gpios), (	\
            .alert_gpio = GPIO_DT_SPEC_INST_GET(inst, alert_gpios), \
	))))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, abside_gpios), (	\
        .abside_gpio = GPIO_DT_SPEC_INST_GET(inst, abside_gpios), \
    ))								\
    IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, reset_gpios), (	\
        .reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios), \
    ))								\
};\
static struct stusb4500_data stusb4500_##inst##_drvdata = { \
}; \
\
DEVICE_DT_INST_DEFINE(inst,								\
		stusb4500_init,									\
		device_pm_control_nop,							\
		&stusb4500_##inst##_drvdata,						\
		&stusb4500_##inst##_cfg,							\
		APPLICATION, CONFIG_STUSB4500_INIT_PRIORITY,		\
		&stusb4500_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEVICE_INSTANCE);
