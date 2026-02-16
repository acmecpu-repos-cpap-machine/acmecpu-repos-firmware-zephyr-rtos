/*
 * Copyright (c) 2021 Acme CPU
 */


#include <zephyr.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(bsp_ucpd);

#include "acpu_c201_modules.h"
#if (CONFIG_STM32G4UCPD)
#include "stm32g4_ucpd.h"
#include "usbpd_def.h"
#endif
#include "app_battery/bsp_ucpd.h"

#define USBPD_PORT_COUNT	CONFIG_USBPD_PORT_COUNT

#define BSPUCPD_DECODE_50MV(_Value_)           ((uint16_t)(((_Value_) * 50U)))     /* From 50mV multiples to mV     */
#define BSPUCPD_DECODE_100MV(_Value_)          ((uint16_t)(((_Value_) * 100U)))    /* From 100mV multiples to mV    */
#define BSPUCPD_DECODE_10MA(_Value_)           ((uint16_t)(((_Value_) * 10U)))     /* From 10mA multiples to mA     */
#define BSPUCPD_DECODE_50MA(_Value_)           ((uint16_t)(((_Value_) * 50U)))     /* From 50mA multiples to mA     */
#define BSPUCPD_DECODE_MW(_Value_)             ((uint16_t)(((_Value_) * 250U)))    /* From 250mW multiples to mW    */

/* Work queue thread for waiting and getting responses from the UCPD Policy Engine layer */
static struct k_work m_work_resp;

#if (CONFIG_STM32G4UCPD)
static bool m_pe_response = false;
static BSPUCPD_HandleTypeDef BSPUCPD_SaveInfo;	/* only 1 UCPD port is supported */

void EventNotification(uint8_t PortNum, uint16_t EventVal) {
	switch(EventVal) {
	case USBPD_NOTIFY_REQUEST_ACCEPTED:
		m_pe_response = true;
		break;
	case USBPD_NOTIFY_REQUEST_REJECTED:
		break;
	case USBPD_NOTIFY_POWER_EXPLICIT_CONTRACT:
		break;
	case USBPD_NOTIFY_PE_DISABLED:
		break;
	}
}

uint32_t CableDetectionNotification(uint32_t PortNum, uint32_t state, uint32_t Cc) {
	switch (state) {
	case USBPD_CAD_EVENT_EMC:
	case USBPD_CAD_EVENT_DETACHED:
		break;
	case USBPD_CAD_EVENT_ATTEMC:
	case USBPD_CAD_EVENT_ATTACHED:
		break;
	}
	return 0;
}

void SaveInfo(uint8_t PortNum, uint8_t DataId, uint8_t *Ptr, uint32_t Size) {
	LOG_INF("UCPD Port = %d", PortNum);
	LOG_INF("DataId = %d", DataId);
	LOG_INF("Size = %d", Size);

	/* USER CODE BEGIN USBPD_DPM_SetDataInfo */
	uint32_t index;

	/* Check type of information targeted by request */
	switch (DataId) {
	/* Case Received Source PDO values Data information :
	 */
	case USBPD_CORE_DATATYPE_RCV_SRC_PDO:
		if (Size <= (USBPD_MAX_NB_PDO * 4U)) {
			uint8_t *rdo;
			BSPUCPD_SaveInfo.NumberOfRcvSRCPDO = (Size / 4U);

			/* Copy PDO data in DPM Handle field */
			for (index = 0U; index < (Size / 4U); index++) {
				rdo = (uint8_t *)&BSPUCPD_SaveInfo.ListOfRcvSRCPDO[index];
				(void) memcpy(rdo, (Ptr + (index * 4U)), (4U * sizeof(uint8_t)));
			}
			m_pe_response = true;
		}
		break;
	default:
		break;
	}
}

#endif	/* (CONFIG_STM32G4UCPD) */

static void resp_worker(struct k_work *work) {

}

int bsp_ucpd_init() {
	int ret = 0;

	k_work_init(&m_work_resp, resp_worker);

#if (CONFIG_STM32G4UCPD)

	/* register callback handlers with UCPD driver */
	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_UCPD);
	if (dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_UCPD);
		return -ENXIO;
	}

	/* variable set by callback functions */
	m_pe_response = false;

	const struct stm32g4ucpd_driver_api *api = dev->api;
	api->callbacks_set(dev, EventNotification, CableDetectionNotification, SaveInfo);

#endif /* (CONFIG_STM32G4UCPD) */


	return ret;
}

int bsp_ucpd_source_capabilities_get(uint32_t *p_num_src_pdo, uint32_t *p_list_of_pdo) {
	int ret = 0;
	m_pe_response = false;

#if (CONFIG_STM32G4UCPD)

	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_UCPD);
	if (dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_UCPD);
		return -ENXIO;
	}

	/* Request the PE to get source capabilities */
	const struct stm32g4ucpd_driver_api *api = dev->api;
	ret = api->src_capability_req(dev);
	if (ret != 0) {
		LOG_ERR("Get source capability request rejected");
		return ret;
	}

	/* Wait until we get a response from the UCPD PE, else timeout in 2000 ms */
	uint32_t _timeout = 0;
	while (m_pe_response != true) {

		k_sleep(K_MSEC(1));
		_timeout++;

		if (_timeout > 2000) {
			LOG_ERR("Response timed out!");
			ret = -ETIMEDOUT;
			return ret;
		}
	}

	if (p_num_src_pdo != NULL)
		*p_num_src_pdo = BSPUCPD_SaveInfo.NumberOfRcvSRCPDO;

	if (p_list_of_pdo != NULL)
		memcpy(p_list_of_pdo, BSPUCPD_SaveInfo.ListOfRcvSRCPDO, (BSPUCPD_SaveInfo.NumberOfRcvSRCPDO * 4U));

#endif /* (CONFIG_STM32G4UCPD) */

	return ret;
}

int bsp_ucpd_decode_PDO_to_VA(uint32_t PDO_to_decode, uint16_t *p_mvolts, uint16_t *p_mamps) {
	int ret = 0;

//	uint32_t _max_power = 0;
	uint16_t _voltage = 0;
	uint16_t _curr = 0;
	uint16_t _power;
//	uint16_t _min_voltage = 0xFFFF;
//	uint16_t _max_voltage = 0;
//	uint16_t _max_current = 0;

#if (CONFIG_STM32G4UCPD)

	USBPD_PDO_TypeDef pdo;
	pdo.d32 = PDO_to_decode;

	switch (pdo.GenericPDO.PowerObject) {
	case USBPD_CORE_PDO_TYPE_FIXED: /*!< Fixed Supply PDO */
		_voltage = BSPUCPD_DECODE_50MV(pdo.SNKFixedPDO.VoltageIn50mVunits);
//		GUI_UPDATE_VOLTAGE_MIN(_voltage, _min_voltage);
//		GUI_UPDATE_VOLTAGE_MAX(_voltage, _max_voltage);
		_curr = BSPUCPD_DECODE_10MA(pdo.SNKFixedPDO.OperationalCurrentIn10mAunits);
//		GUI_UPDATE_CURRENT_MAX(_curr, _max_current);
		break;
	case USBPD_CORE_PDO_TYPE_BATTERY: /*!< Battery Supply PDO */
		_voltage = BSPUCPD_DECODE_50MV(pdo.SNKBatteryPDO.MinVoltageIn50mVunits);
//		GUI_UPDATE_VOLTAGE_MIN(_voltage, _min_voltage);
		_voltage = BSPUCPD_DECODE_50MV(pdo.SNKBatteryPDO.MaxVoltageIn50mVunits);
//		GUI_UPDATE_VOLTAGE_MAX(_voltage, _max_voltage);
		_power = BSPUCPD_DECODE_MW(pdo.SNKBatteryPDO.OperationalPowerIn250mWunits);
//		GUI_UPDATE_POWER_MAX(_power, _max_power);
		break;
	case USBPD_CORE_PDO_TYPE_VARIABLE: /*!< Variable Supply (non-battery) PDO */
		_voltage = BSPUCPD_DECODE_50MV(pdo.SNKVariablePDO.MinVoltageIn50mVunits);
//		GUI_UPDATE_VOLTAGE_MIN(_voltage, _min_voltage);
		_voltage = BSPUCPD_DECODE_50MV(pdo.SNKVariablePDO.MaxVoltageIn50mVunits);
//		GUI_UPDATE_VOLTAGE_MAX(_voltage, _max_voltage);
		_curr = BSPUCPD_DECODE_10MA(pdo.SNKVariablePDO.OperationalCurrentIn10mAunits);
//		GUI_UPDATE_CURRENT_MAX(_curr, _max_current);
		break;
#if _PPS
		case USBPD_CORE_PDO_TYPE_APDO: /*!< Augmented Power Data Object (APDO) */
		_voltage = BSPUCPD_DECODE_100MV(pdo.SRCSNKAPDO.MinVoltageIn100mV);
//		GUI_UPDATE_VOLTAGE_MIN(_voltage, _min_voltage);
		_voltage = BSPUCPD_DECODE_100MV(pdo.SRCSNKAPDO.MaxVoltageIn100mV);
//		GUI_UPDATE_VOLTAGE_MAX(_voltage, _max_voltage);
		_current = BSPUCPD_DECODE_50MA(pdo.SRCSNKAPDO.MaxCurrentIn50mAunits);
//		GUI_UPDATE_CURRENT_MAX(_current, _max_current);
		break;
#endif /*_USBPD_REV30_SUPPORT && PPS*/
		default:
		break;
	}

#endif /* (CONFIG_STM32G4UCPD) */

	*p_mvolts = _voltage;
	*p_mamps = _curr;

	return ret;
}

int bsp_ucpd_power_profile_request(uint8_t index_src_pdo, uint16_t requested_voltage) {
	int ret=0;
	m_pe_response = false;

#if (CONFIG_STM32G4UCPD)

	const struct device *dev = device_get_binding(ACPU_C201_MOD_NAME_UCPD);
	if (dev == NULL) {
		LOG_ERR("device %s not found", ACPU_C201_MOD_NAME_UCPD);
		return -ENXIO;
	}

	/* Request the PE to switch power profile */
	const struct stm32g4ucpd_driver_api *api = dev->api;
	ret = api->power_profile_req(dev, index_src_pdo, requested_voltage);
	if (ret != 0) {
		LOG_ERR("Power role switch request rejected");
		return ret;
	}

	/* Wait until we get a response from the UCPD PE, else timeout in 2000 ms */
	uint32_t _timeout = 0;
	while (m_pe_response != true) {

		k_sleep(K_MSEC(1));
		_timeout++;

		if (_timeout > 2000) {
			LOG_ERR("Response timed out!");
			ret = -ETIMEDOUT;
			return ret;
		}
	}

#endif /* (CONFIG_STM32G4UCPD) */

	return ret;
}
