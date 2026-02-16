/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usbpd_dpm_core.c
  * @author  MCD Application Team
  * @brief   USBPD dpm core file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#define __USBPD_DPM_CORE_C

#define STM32CUBE_OS	0

/* Includes ------------------------------------------------------------------*/
#include <zephyr.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>

#include "usbpd_core.h"
#include "usbpd_trace.h"
#include "usbpd_dpm_core.h"
#include "usbpd_dpm_conf.h"
#include "usbpd_dpm_user.h"

/* Generic STM32 prototypes */
extern uint32_t HAL_GetTick(void);

#if STM32CUBE_OS

#else	/* STM32CUBE_OS */

void USBPD_PE_Task(void *p1, void *p2, void *p3);
void USBPD_CAD_Task(void *p1, void *p2, void *p3);

/* USB PD PE thread variables */
#define USBPD_PE_THREAD_PRIORITY	6
K_THREAD_STACK_DEFINE(m_usbpd_pe_thread_stack, 512);
static struct k_thread m_usbpd_pe_thread_data[USBPD_PORT_COUNT];
static k_tid_t m_usbpd_pe_tid[USBPD_PORT_COUNT];
static struct k_fifo m_usbpd_pe_fifo[USBPD_PORT_COUNT];
//static uint32_t m_usbpd_pe_fifo_data = 0xFFFF;
//static uint32_t m_usbpd_pe_fifo_data[2] = {0,0};

/* USB PD CAD thread variables */
#define USBPD_CAD_THREAD_PRIORITY	7
K_THREAD_STACK_DEFINE(m_usbpd_cad_thread_stack, 1024);
static struct k_thread m_usbpd_cad_thread_data;
static k_tid_t m_usbpd_cad_tid;
static struct k_fifo m_usbpd_cad_fifo;
//static uint32_t m_usbpd_cad_fifo_data = 0xFFFF;
//static uint32_t m_usbpd_cad_fifo_data[2] = {0,0};

#endif /*STM32CUBE_OS */

/* Private macro -------------------------------------------------------------*/
#define CHECK_PE_FUNCTION_CALL(_function_)  _retr = _function_;                  \
                                            if(USBPD_OK != _retr) {return _retr;}
#define CHECK_CAD_FUNCTION_CALL(_function_) if(USBPD_CAD_OK != _function_) {return USBPD_ERROR;}

#if defined(_DEBUG_TRACE)
#define DPM_CORE_DEBUG_TRACE(_PORTNUM_, __MESSAGE__)  USBPD_TRACE_Add(USBPD_TRACE_DEBUG, _PORTNUM_, 0u, (uint8_t *)(__MESSAGE__), sizeof(__MESSAGE__) - 1u);
#else
#define DPM_CORE_DEBUG_TRACE(_PORTNUM_, __MESSAGE__)
#endif /* _DEBUG_TRACE */

USBPD_ParamsTypeDef   DPM_Params[USBPD_PORT_COUNT];

/* Private function prototypes -----------------------------------------------*/
static void USBPD_PE_TaskWakeUp(uint8_t PortNum);
static void DPM_ManageAttachedState(uint8_t PortNum, USBPD_CAD_EVENT State, CCxPin_TypeDef Cc);
void USBPD_DPM_CADCallback(uint8_t PortNum, USBPD_CAD_EVENT State, CCxPin_TypeDef Cc);
static void USBPD_DPM_CADTaskWakeUp(void);

/**
  * @brief  Initialize the core stack (port power role, PWR_IF, CAD and PE Init procedures)
  * @retval USBPD status
  */
USBPD_StatusTypeDef USBPD_DPM_InitCore(void)
{
  /* variable to get dynamique memory allocated by usbpd stack */
  uint32_t stack_dynamemsize;
  USBPD_StatusTypeDef _retr = USBPD_OK;

  static const USBPD_PE_Callbacks dpmCallbacks =
  {
    NULL,
    USBPD_DPM_HardReset,
    NULL,
    USBPD_DPM_Notification,
    USBPD_DPM_ExtendedMessageReceived,
    USBPD_DPM_GetDataInfo,
    USBPD_DPM_SetDataInfo,
    NULL,
    USBPD_DPM_SNK_EvaluateCapabilities,
    NULL,
    USBPD_PE_TaskWakeUp,
#if defined(_VCONN_SUPPORT)
    USBPD_DPM_EvaluateVconnSwap,
    USBPD_DPM_PE_VconnPwr,
#else
    NULL,
    NULL,
#endif /* _VCONN_SUPPORT */
    USBPD_DPM_EnterErrorRecovery,
    USBPD_DPM_EvaluateDataRoleSwap,
    USBPD_DPM_IsPowerReady
  };

  static const USBPD_CAD_Callbacks CAD_cbs =
  {
    USBPD_DPM_CADCallback,
    USBPD_DPM_CADTaskWakeUp
  };

  /* Check the lib selected */
  if (USBPD_TRUE != USBPD_PE_CheckLIB(_LIB_ID))
  {
    return USBPD_ERROR;
  }

  /* to get how much memory are dynamically allocated by the stack
     the memory return is corresponding to 2 ports so if the application
     managed only one port divide the value return by 2                   */
  stack_dynamemsize = USBPD_PE_GetMemoryConsumption();

  /* done to avoid warning */
  (void)stack_dynamemsize;

  for (uint8_t _port_index = 0; _port_index < USBPD_PORT_COUNT; ++_port_index)
  {
    /* Variable to be sure that DPM is correctly initialized */
    DPM_Params[_port_index].DPM_Initialized = USBPD_FALSE;

    /* check the stack settings */
    DPM_Params[_port_index].PE_SpecRevision  = DPM_Settings[_port_index].PE_SpecRevision;
    DPM_Params[_port_index].PE_PowerRole     = DPM_Settings[_port_index].PE_DefaultRole;
    DPM_Params[_port_index].PE_SwapOngoing   = USBPD_FALSE;
    DPM_Params[_port_index].ActiveCCIs       = CCNONE;
    DPM_Params[_port_index].VconnCCIs        = CCNONE;
    DPM_Params[_port_index].VconnStatus      = USBPD_FALSE;

    /* CAD SET UP : Port 0 */
    CHECK_CAD_FUNCTION_CALL(USBPD_CAD_Init(_port_index, (USBPD_CAD_Callbacks *)&CAD_cbs,
                                           (USBPD_SettingsTypeDef *)&DPM_Settings[_port_index], &DPM_Params[_port_index]));

    /* PE SET UP : Port 0 */
    CHECK_PE_FUNCTION_CALL(USBPD_PE_Init(_port_index, (USBPD_SettingsTypeDef *)&DPM_Settings[_port_index],
                                         &DPM_Params[_port_index], &dpmCallbacks));

  /* DPM is correctly initialized */
    DPM_Params[_port_index].DPM_Initialized = USBPD_TRUE;

    /* Enable CAD on Port 0 */
  USBPD_CAD_PortEnable(_port_index, USBPD_CAD_ENABLE);
  }

  return _retr;
}


/**
  * @brief  Initialize the OS parts (task, queue,... )
  * @retval USBPD status
  */
USBPD_StatusTypeDef USBPD_DPM_InitOS(void) {
	/* Initialize CAD queue and thread */
	k_fifo_init(&m_usbpd_cad_fifo);
	m_usbpd_cad_tid = k_thread_create(&m_usbpd_cad_thread_data,
			m_usbpd_cad_thread_stack,
			K_THREAD_STACK_SIZEOF(m_usbpd_cad_thread_stack), USBPD_CAD_Task,
			NULL, NULL, NULL, USBPD_CAD_THREAD_PRIORITY, 0, K_NO_WAIT);
	if (m_usbpd_cad_tid == NULL) {
		return USBPD_ERROR;
	}

	/* Initialize PE queue and thread */
	k_fifo_init(&m_usbpd_pe_fifo[0]);
#if USBPD_PORT_COUNT == 2
	k_fifo_init(&m_usbpd_pe_fifo[1]);
#endif	/* USBPD_PORT_COUNT == 2 */
	/* PE task to be created on attachment */
	m_usbpd_pe_tid[USBPD_PORT_0] = NULL;
#if USBPD_PORT_COUNT == 2
	  DPM_PEThreadId_Table[USBPD_PORT_1] = NULL;
#endif /* USBPD_PORT_COUNT == 2 */

	return USBPD_OK;
}

#if STM32CUBE_OS
/**
  * @brief  Initialize the OS parts (port power role, PWR_IF, CAD and PE Init procedures)
  * @retval None
  */
void USBPD_DPM_Run(void)
{
#if (osCMSIS >= 0x20000U)
  osKernelInitialize();
#endif /* osCMSIS >= 0x20000U */
  osKernelStart();
}
#endif /* STM32CUBE_OS */

/**
  * @brief  Initialize DPM (port power role, PWR_IF, CAD and PE Init procedures)
  * @retval USBPD status
  */
void USBPD_DPM_TimerCounter(void)
{
  /* Call PE/PRL timers functions only if DPM is initialized */
  if (USBPD_TRUE == DPM_Params[USBPD_PORT_0].DPM_Initialized)
  {
    USBPD_DPM_UserTimerCounter(USBPD_PORT_0);
    USBPD_PE_TimerCounter(USBPD_PORT_0);
    USBPD_PRL_TimerCounter(USBPD_PORT_0);
  }
#if USBPD_PORT_COUNT==2
  if (USBPD_TRUE == DPM_Params[USBPD_PORT_1].DPM_Initialized)
  {
    USBPD_DPM_UserTimerCounter(USBPD_PORT_1);
    USBPD_PE_TimerCounter(USBPD_PORT_1);
    USBPD_PRL_TimerCounter(USBPD_PORT_1);
  }
#endif /* USBPD_PORT_COUNT == 2 */

}

/**
  * @brief  WakeUp PE task
  * @param  PortNum port number
  * @retval None
  */
static void USBPD_PE_TaskWakeUp(uint8_t PortNum) {
//#if (osCMSIS < 0x20000U)
//  (void)osMessagePut(PEQueueId[PortNum], 0xFFFF, 0);
//#endif /* osCMSIS < 0x20000U */
//	k_fifo_put(&m_usbpd_pe_fifo[PortNum], m_usbpd_pe_fifo_data);

	uint32_t *pe_data = (uint32_t*) calloc(2, sizeof(uint32_t));
	k_fifo_put(&m_usbpd_pe_fifo[PortNum], pe_data);
}

/**
  * @brief  WakeUp CAD task
  * @retval None
  */
static void USBPD_DPM_CADTaskWakeUp(void) {
//#if (osCMSIS < 0x20000U)
//  (void)osMessagePut(CADQueueId, 0xFFFF, 0);
//#endif /* osCMSIS < 0x20000U */

//	k_fifo_put(&m_usbpd_cad_fifo, m_usbpd_cad_fifo_data);

	uint32_t *cad_data = (uint32_t*) calloc(2, sizeof(uint32_t));
	k_fifo_put(&m_usbpd_cad_fifo, cad_data);
}

/**
  * @brief  Main task for PE layer
  * @param  argument Not used
  * @retval None
  */
void USBPD_PE_Task(void *p1, void *p2, void *p3) {
	uint8_t _port = 0;//(uint32_t) p1;
	uint32_t _timing;

	for (;;) {
		_timing = USBPD_PE_StateMachine_SNK(_port);
//    osMessageGet(PEQueueId[_port],_timing);
		uint32_t *pe_data = k_fifo_get(&m_usbpd_pe_fifo[_port], K_MSEC(_timing));
		if (pe_data != NULL) {
			free(pe_data);
		}
	}
}

/**
  * @brief  Main task for CAD layer
  * @param  argument Not used
  * @retval None
  */
void USBPD_CAD_Task(void *p1, void *p2, void *p3) {
	uint32_t _timing;
	for (;;) {
//#if (osCMSIS < 0x20000U)
//    osMessageGet(CADQueueId, USBPD_CAD_Process());
//#endif /* osCMSIS < 0x20000U */
		_timing = USBPD_CAD_Process();
		uint32_t *cad_data = k_fifo_get(&m_usbpd_cad_fifo, K_MSEC(_timing));
		if (cad_data != NULL) {
			free(cad_data);
		}
	}
}

/**
  * @brief  CallBack reporting events on a specified port from CAD layer.
  * @param  PortNum   The handle of the port
  * @param  State     CAD state
  * @param  Cc        The Communication Channel for the USBPD communication
  * @retval None
  */
void USBPD_DPM_CADCallback(uint8_t PortNum, USBPD_CAD_EVENT State, CCxPin_TypeDef Cc)
 {
	switch (State) {
	case USBPD_CAD_EVENT_ATTEMC: {
#ifdef _VCONN_SUPPORT
      DPM_Params[PortNum].VconnStatus = USBPD_TRUE;
#endif /* _VCONN_SUPPORT */
		DPM_ManageAttachedState(PortNum, State, Cc);
#ifdef _VCONN_SUPPORT
      DPM_CORE_DEBUG_TRACE(PortNum, "Note: VconnStatus=TRUE");
#endif /* _VCONN_SUPPORT */
		break;
	}
	case USBPD_CAD_EVENT_ATTACHED:
		DPM_ManageAttachedState(PortNum, State, Cc);
		break;
	case USBPD_CAD_EVENT_DETACHED:
	case USBPD_CAD_EVENT_EMC: {
		/* The ufp is detached */
		(void) USBPD_PE_IsCableConnected(PortNum, 0);
		/* Terminate PE task */
		if (m_usbpd_pe_tid[PortNum] != NULL) {
			uint8_t _timeout = 0;
			/* Let time to PE to complete the ongoing action */
//        while (eBlocked != eTaskGetState(DPM_PEThreadId_Table[PortNum]))
			while (strcmp(k_thread_state_str(m_usbpd_pe_tid[PortNum]), "queued")
					== 0) {
				k_sleep(K_MSEC(1));
				_timeout++;
				if (_timeout > 30) {
					break;
				}
			};

			/* Kill PE task */
//        osThreadTerminate(DPM_PEThreadId_Table[PortNum]);
//        DPM_PEThreadId_Table[PortNum] = NULL;
			k_thread_abort(m_usbpd_pe_tid[PortNum]);
			m_usbpd_pe_tid[PortNum] = NULL;
		}
		DPM_Params[PortNum].PE_SwapOngoing = USBPD_FALSE;
		DPM_Params[PortNum].ActiveCCIs = CCNONE;
		DPM_Params[PortNum].PE_Power = USBPD_POWER_NO;
		USBPD_DPM_UserCableDetection(PortNum, State);
#ifdef _VCONN_SUPPORT
      DPM_Params[PortNum].VconnCCIs = CCNONE;
      DPM_Params[PortNum].VconnStatus = USBPD_FALSE;
      DPM_CORE_DEBUG_TRACE(PortNum, "Note: VconnStatus=FALSE");
#endif /* _VCONN_SUPPORT */
		break;
	}
	default:
		/* nothing to do */
		break;
	}
}

static void DPM_ManageAttachedState(uint8_t PortNum, USBPD_CAD_EVENT State, CCxPin_TypeDef Cc)
 {
#ifdef _VCONN_SUPPORT
  if (CC1 == Cc)
  {
    DPM_Params[PortNum].VconnCCIs = CC2;
  }
  if (CC2 == Cc)
  {
    DPM_Params[PortNum].VconnCCIs = CC1;
  }
#endif /* _VCONN_SUPPORT */
	DPM_Params[PortNum].ActiveCCIs = Cc;
	(void) USBPD_PE_IsCableConnected(PortNum, 1);

	USBPD_DPM_UserCableDetection(PortNum, State);

	/* Create PE task */
	if (m_usbpd_pe_tid[PortNum] == NULL) {
//#if (osCMSIS < 0x20000U)
//    DPM_PEThreadId_Table[PortNum] = osThreadCreate(OSTHREAD_PE(PortNum), (void *)((uint32_t)PortNum));
//#endif /* osCMSIS < 0x20000U */

		m_usbpd_pe_tid[PortNum] = k_thread_create(
				&m_usbpd_pe_thread_data[PortNum], m_usbpd_pe_thread_stack,
				K_THREAD_STACK_SIZEOF(m_usbpd_pe_thread_stack), USBPD_PE_Task,
				NULL, NULL, NULL, USBPD_PE_THREAD_PRIORITY, 0, K_NO_WAIT);
		if (m_usbpd_pe_tid[PortNum] == NULL) {
			/* should not occur. May be an issue with FreeRTOS heap size too small */
			while (1)
				;
		}
	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
