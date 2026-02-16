/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef MODULES_STM32G4_UCPD_STM32G4_UCPD_H_
#define MODULES_STM32G4_UCPD_STM32G4_UCPD_H_

//#ifdef __cplusplus
//extern "C" {
//#endif

#include <stdint.h>
#include <device.h>
#include <drivers/adc.h>

/* Application layer callback function typedefs */
typedef void (*stm32g4_ucpd_post_notif_msg_handler_t) (uint8_t port_num, uint16_t event_val);
typedef uint32_t (*stm32g4_ucpd_fmt_send_notif_handler_t) (uint32_t port_num, uint32_t type_notification, uint32_t value);
typedef void (*stm32g4_ucpd_save_info_handler_t) (uint8_t port_num, uint8_t data_id, uint8_t *ptr, uint32_t size);

/* Driver APIs typedefs */
typedef void (*callbacks_set_t)(const struct device *, stm32g4_ucpd_post_notif_msg_handler_t, stm32g4_ucpd_fmt_send_notif_handler_t, stm32g4_ucpd_save_info_handler_t);
typedef int (*src_capability_req_t)(const struct device *);
typedef int (*power_profile_req_t)(const struct device *, uint8_t, uint16_t);

struct stm32g4ucpd_driver_api {
	/**
	  * @brief  Function signature:
	  * 			void callback_handlers_set(const struct device *dev,
	  * 										stm32g4_ucpd_post_notif_msg_handler_t PtrPost,
	  * 										stm32g4_ucpd_fmt_send_notif_handler_t PtrFormatSend,
	  * 										stm32g4_ucpd_save_info_handler_t PtrSaveInfo);
	  *
	  * 		API to set callback handlers of the STM32G4 UCPD driver
	  *
	  * @param  dev										device instance
	  * @param  stm32g4_ucpd_post_notif_msg_handler_t	handler address
	  * @param	stm32g4_ucpd_fmt_send_notif_handler_t	handler address
	  * @param	stm32g4_ucpd_save_info_handler_t		handler address
	  *
	  */
	callbacks_set_t callbacks_set;

	/**
	  * @brief  Function signature:
	  * 			int source_capability_get_request(const struct device *dev);
	  * 		API to request the PE layer to get connected distant source capabilities (PDOs)
	  * 		The response will be made via save_info callback
	  *
	  * @param  dev		device instance
	  *
	  * @retval	0		success
	  * 		other	fail
	  *
	  */
	src_capability_req_t src_capability_req;

	/**
	  * @brief  Function signature:
	  * 			int power_profile_request(const struct device *dev, uint8_t index_src_pdo, uint16_t requested_voltage);
	  * 		API to request the PE layer to send a request message to switch the power profile
	  * 		The response will be made via save_info callback
	  *
	  * @param  dev					device instance
	  * @param	index_src_pdo		Index on the selected SRC PDO (value between 1 to 7)
	  * @param	requested_voltage	Requested voltage (in MV and use mainly for APDO)
	  *
	  * @retval	0		success
	  * 		other	fail
	  */
	power_profile_req_t power_profile_req;
};

/*
 * Data structure used for VBUS level sensing.
 * Used in usbpd_pwr_user.c
 * */
struct vbus_ucpd_sense {
	const struct device *dev;
	struct adc_channel_cfg chcfg;
	struct adc_sequence seq;
};

//#ifdef __cplusplus
//}
//#endif

#endif /* MODULES_STM32G4_UCPD_STM32G4_UCPD_H_ */
