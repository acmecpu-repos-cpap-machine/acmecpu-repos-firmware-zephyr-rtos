/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_BATTERY_BSP_UCPD_H_
#define SRC_INCLUDE_APP_BATTERY_BSP_UCPD_H_

#if (CONFIG_STM32G4UCPD)
#include "stm32g4_ucpd.h"
#include "usbpd_def.h"
#endif

/**
  * @brief  USBPD DPM handle Structure definition
  * @{
  */
typedef struct
{
  uint32_t              ListOfRcvSRCPDO[USBPD_MAX_NB_PDO];   /*!< The list of received Source Power Data Objects
                                                                  from Port partner (when Port partner is a Source
                                                                  or a DRP port). */
  uint32_t              NumberOfRcvSRCPDO;                   /*!< The number of received Source Power Data Objects
                                                                  from port Partner (when Port partner is a Source
                                                                  or a DRP port). This parameter must be set
                                                                  to a value lower than USBPD_MAX_NB_PDO */
  uint32_t              ListOfRcvSNKPDO[USBPD_MAX_NB_PDO];   /*!< The list of received Sink Power Data Objects
                                                                  from Port partner (when Port partner is a Sink
                                                                  or a DRP port). */
  uint32_t              NumberOfRcvSNKPDO;                   /*!< The number of received Sink Power Data Objects
                                                                  from port Partner(when Port partner is a Sink
                                                                  or a DRP port). This parameter must be set to
                                                                  a value lower than USBPD_MAX_NB_PDO */
  uint32_t              RDOPosition;                         /*!< RDO Position of requested DO in Source list
                                                                  of capabilities */
  uint32_t              RequestedVoltage;                    /*!< Value of requested voltage */
  uint32_t              RequestedCurrent;                    /*!< Value of requested current */
  uint32_t              RDOPositionPrevious;                 /*!< RDO Position of previous requested DO
                                                                  in Source list of capabilities */
  uint32_t              RcvRequestDOMsg;                     /*!< Received request Power Data Object message
                                                                  from the port Partner */
#if defined(USBPD_REV30_SUPPORT)
#if _STATUS
  USBPD_SDB_TypeDef     RcvStatus;                           /*!< Status received by port partner */
#endif /* _STATUS */
#if _PPS
  USBPD_PPSSDB_TypeDef  RcvPPSStatus;                        /*!< PPS Status received by port partner */
#endif /* _PPS */
#if _SRC_CAPA_EXT
  USBPD_SCEDB_TypeDef   RcvSRCExtendedCapa;                  /*!< SRC Extended Capability received by port partner */
#endif /* _SRC_CAPA_EXT */
#if defined(USBPDCORE_SNK_CAPA_EXT)
  USBPD_SKEDB_TypeDef   RcvSNKExtendedCapa;                  /*!< SNK Extended Capability received by port partner */
#endif /* USBPDCORE_SNK_CAPA_EXT */
#if _MANU_INFO
  USBPD_GMIDB_TypeDef   GetManufacturerInfo;                 /*!< Get Manufacturer Info */
#endif /* _MANU_INFO */
#if _BATTERY
  USBPD_GBSDB_TypeDef   GetBatteryStatus;                    /*!< Get Battery status */
  USBPD_GBCDB_TypeDef   GetBatteryCapability;                /*!< Get Battery Capability */
  USBPD_BSDO_TypeDef    BatteryStatus;                       /*!< Battery status */
#endif /* _BATTERY */
  USBPD_ADO_TypeDef     RcvAlert;                            /*!< Save the Alert received by port partner */
#endif /* USBPD_REV30_SUPPORT */
#ifdef _VCONN_SUPPORT
  USBPD_DiscoveryIdentity_TypeDef VDM_DiscoCableIdentify;                /*!< VDM Cable Discovery Identify */
#endif /* _VCONN_SUPPORT */
#ifdef _VDM
  USBPD_DiscoveryIdentity_TypeDef   VDM_DiscoIdentify;                   /*!< VDM Discovery Identify */
  USBPD_SVIDPortPartnerInfo_TypeDef VDM_SVIDPortPartner;                 /*!< VDM SVID list */
  USBPD_ModeInfo_TypeDef            VDM_ModesPortPartner;                /*!< VDM Modes list */
#endif /* _VDM */
} BSPUCPD_HandleTypeDef;

/**
 * @brief: 	Initializes the BSP layer of UCPD subsystem.
 * 			It registers callback functions with the UCPD driver
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_ucpd_init();

/**
 * @brief: 	Sends request to the UCPD Policy Engine to get source capabilities, waits until the UCPD PE responds
 * 			copies the received information and returns.
 * 			This function will remain blocked until the PE responds or timeout occurs
 * 			Calling function must provide allocated memory
 *
 * @param:	p_num_src_pdo[out]	Number of source PDO, pointer
 * 			p_list_of_pdo[out]	List of source PDO, pointer
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_ucpd_source_capabilities_get(uint32_t *p_num_src_pdo, uint32_t *p_list_of_pdo);

/**
 * @brief: 	Decode the input PDO into milli-volts and milli-amps
 *
 * @param:	PDO_to_decode[in]	input PDO to decode
 * 			mvolts[out]			decode milli-volts output
 * 			mamps[out]			decode milli-mamps output
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_ucpd_decode_PDO_to_VA(uint32_t PDO_to_decode, uint16_t *p_mvolts, uint16_t *p_mamps);

/**
 * @brief: 	Send a request to the UCPD Policy Engine to switch the Power Role
 *
 * @param:	index_src_pdo[in]		Index on the selected SRC PDO (value between 1 to 7)
 * 			requested_voltage[in]	Requested voltage (in MV)
 *
 * @return:	0 for Success
 * 			-ERRNO for failure
 * */
int bsp_ucpd_power_profile_request(uint8_t index_src_pdo, uint16_t requested_voltage);


#endif /* SRC_INCLUDE_APP_BATTERY_BSP_UCPD_H_ */
