/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uart_m2m_comm.h"
#include "uart_m2m_comm_mc_cmds.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
DMA_HandleTypeDef hdma_adc;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim14;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
#if NO_UART
extern __IO uint8_t g_uart_m2m_comm_data_available;
static uint8_t m_um2m_rx_buf[CONFIG_UART_M2M_BUFFER_SIZE] = {0x00};
static uint8_t m_um2m_tx_buf[CONFIG_UART_M2M_BUFFER_SIZE] = {0x00};
static uint32_t m_um2m_rx_len = 0;
static struct m2m_frame_t frame, resp_frame;
#endif
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM14_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if NO_UART
static void frame_header_single_make(struct m2m_frame_t *frame) {
	frame->sof = UART_M2M_START_OF_FRAME;
	frame->type = UART_M2M_FRAME_SINGLE_RESP;
	frame->sequence = 0;
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC_Init();
  MX_TIM1_Init();
  MX_TIM14_Init();
  MX_USART1_UART_Init();
  MX_MotorControl_Init();
  MX_I2C1_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */
#if NO_UART
  /* Enable RXNE and Error interrupts */
  LL_USART_EnableIT_RXNE(USART1);
//  LL_USART_EnableIT_TXE(USART1);
  LL_USART_EnableIT_ERROR(USART1);
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  /* Set the default ramp settings */
  HAL_Delay(250);
  MC_ProgramSpeedRampMotor1(DEF_RAMP_SPEED_01HZ, DEF_RAMP_DURATION_MS);
//  MC_StartMotor1();
#if NO_UART
  int ret = -1;
  uart_m2m_comm_set_app_buf(m_um2m_rx_buf, &m_um2m_rx_len);

  char cmd[3] = {0x00};
  uint8_t i=0;
#endif
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
#if 0
	  uint16_t faults = MC_GetCurrentFaultsMotor1();
	  if (faults != 0) {
		  MC_AcknowledgeFaultMotor1();
		  MC_StopMotor1();
		  HAL_Delay(1000);
		  MC_StartMotor1();
	  }

	  uint16_t state = MC_GetSTMStateMotor1();
	  if ((state != IDLE) || (state != IDLE_START)) {
		  MC_StopMotor1();
		  HAL_Delay(500);
		  MC_StartMotor1();
	  }
#endif

#if NO_UART
		if (g_uart_m2m_comm_data_available) {
			g_uart_m2m_comm_data_available = 0;

			/* decode the frame */
			memset(&frame, 0x00, sizeof(struct m2m_frame_t));
			ret = m2m_com_frame_decode(m_um2m_rx_buf, m_um2m_rx_len, &frame);
			if (ret < 0) {
				LL_USART_EnableIT_RXNE(USART1);
				continue;
			}

			/* parse the data */
			if (frame.sof != UART_M2M_START_OF_FRAME) {
				LL_USART_EnableIT_RXNE(USART1);
				continue;
			}

			/* extract the command id */
			i=0;
			memset(cmd, 0x00, sizeof (cmd));
			while (frame.payload[i] != ',') {
				cmd[i] = frame.payload[i];
				i++;
			}
			/* the command should not be more the 4 bytes */
			if (i > 4) {
				LL_USART_EnableIT_RXNE(USART1);
				continue;
			}
			uint16_t cmd_id = atoi(cmd);

//			char *tok = strtok(frame.payload, ",\n");
//			uint16_t cmd_id = atoi(tok);

			i++;
			frame_header_single_make(&resp_frame);

			switch (cmd_id) {
			case M2M_CMD_ID_DO_SW_RST:
				HAL_NVIC_SystemReset();
				break;

			case M2M_CMD_ID_COMM_CHK:
				if (frame.payload[i] == M2M_CMD_PAYLOAD_GET_CHAR) {
					sprintf((char*)resp_frame.payload, "%d%c%s%c", M2M_CMD_ID_COMM_CHK,
					M2M_CMD_PAYLOAD_DELIM, M2M_CMD_RESP_OK,
					M2M_CMD_PAYLOAD_TERM);
//					resp_frame.payload_len = strlen(resp_frame.payload);
				}
				break;

			case M2M_CMD_ID_BLOWER_STATE:
				if (frame.payload[i] == M2M_CMD_PAYLOAD_GET_CHAR) {
					uint8_t state = BLOWER_OFF;
					if (MC_GetSTMStateMotor1() != IDLE) state = BLOWER_ON;

					sprintf((char*)resp_frame.payload, "%d%c%d%c", M2M_CMD_ID_BLOWER_STATE,
										M2M_CMD_PAYLOAD_DELIM, state,
										M2M_CMD_PAYLOAD_TERM);
				} else {
					bool ret = false;
					if (frame.payload[i] == '1') {
						/* turn motor on */
						ret = MC_StartMotor1();
					}
					else if (frame.payload[i] == '0') {
						/* turn motor off */
						ret = MC_StopMotor1();
					}
					sprintf((char*)resp_frame.payload, "%d%c%s%c",
							M2M_CMD_ID_BLOWER_STATE,
							M2M_CMD_PAYLOAD_DELIM,
							(ret ? M2M_CMD_RESP_OK : M2M_CMD_RESP_ERR),
							M2M_CMD_PAYLOAD_TERM);
				}
				break;

			case M2M_CMD_ID_BLOWER_VOLT_MV:
#if 0
				if (frame.payload[i] == M2M_CMD_PAYLOAD_GET_CHAR) {
					int16_t blower_v = MC_GetPhaseVoltageAmplitudeMotor1();
					blower_v = (blower_v * 12)/(SQRT_3 * 32767);
					sprintf(resp_frame.payload, "%d%s%d%s", M2M_CMD_ID_BLOWER_VOLT_MV,
										M2M_CMD_PAYLOAD_DELIM, blower_v*1000,
										M2M_CMD_PAYLOAD_TERM);
				}
#endif
				break;

			case M2M_CMD_ID_BLOWER_SPEED_RPM:
				if (frame.payload[i] == M2M_CMD_PAYLOAD_GET_CHAR) {
					int16_t speed = MC_GetMecSpeedAverageMotor1();
					int speed_rpm = speed*6;
					sprintf((char*)resp_frame.payload, "%d%c%d%c", M2M_CMD_ID_BLOWER_SPEED_RPM,
										M2M_CMD_PAYLOAD_DELIM, speed_rpm, M2M_CMD_PAYLOAD_TERM);
				} else {
					uint8_t j=0, k=3;
					char tmp[7] = {0x00};
					char cmd_state[4] = M2M_CMD_RESP_OK;

					while (frame.payload[k] != M2M_CMD_PAYLOAD_TERM) {
						tmp[j] = frame.payload[k];
						j++;
						k++;
					}
					int32_t speed_in_rpm = atoi(tmp);
					MC_ProgramSpeedRampMotor1(speed_in_rpm/6, 0/*DEF_RAMP_DURATION_MS*/);
					MCI_CommandState_t state = MC_GetCommandStateMotor1();

					if (state == MCI_COMMAND_EXECUTED_UNSUCCESFULLY)
						memcpy(cmd_state, M2M_CMD_RESP_ERR, 3);

					sprintf((char*)resp_frame.payload, "%d%c%s%c", M2M_CMD_ID_BLOWER_SPEED_RPM,
										M2M_CMD_PAYLOAD_DELIM, cmd_state, M2M_CMD_PAYLOAD_TERM);
				}
				break;

			case M2M_CMD_ID_BLOWER_DUTY:
				break;

			case M2M_CMD_ID_BLOWER_RUNSTAT:
				if (frame.payload[i] == M2M_CMD_PAYLOAD_GET_CHAR) {
					/* get faults */
					uint16_t faults_occ = MC_GetOccurredFaultsMotor1();
					uint16_t faults_now = MC_GetCurrentFaultsMotor1();

					uint32_t faults = (uint32_t) faults_occ;
					faults |= (uint32_t) (faults_now << 16);

					/* get speed */
					int16_t speed = MC_GetMecSpeedAverageMotor1();
					int speed_rpm = speed*6;

					/* get bus voltage */

					/* get currents */

					/* */
					sprintf((char*)resp_frame.payload, "%d%c%ld%c%d%c", M2M_CMD_ID_BLOWER_RUNSTAT, M2M_CMD_PAYLOAD_DELIM,
							faults, M2M_CMD_PAYLOAD_DELIM,
							speed_rpm,
							M2M_CMD_PAYLOAD_TERM);
				}
				break;

			case M2M_CMD_ID_BLOWER_FLTACK:
			{
				char cmd_state[4] = M2M_CMD_RESP_OK;
				bool ret = MC_AcknowledgeFaultMotor1();
				if (!ret)
					memcpy(cmd_state, M2M_CMD_RESP_ERR, 3);

				sprintf((char*)resp_frame.payload, "%d%c%s%c", M2M_CMD_ID_BLOWER_FLTACK,
									M2M_CMD_PAYLOAD_DELIM, cmd_state, M2M_CMD_PAYLOAD_TERM);
			}
				break;

			case M2M_CMD_ID_BLOWER_SPEED_RAMP:
			{
				uint8_t j=0, k=3;
				char tmp[6+6+2] = {0x00};	/*[speed-6b 0x00 duration-6b 0x00]*/
				char cmd_state[4] = M2M_CMD_RESP_OK;

				while (frame.payload[k] != M2M_CMD_PAYLOAD_TERM) {
					if (frame.payload[k] == M2M_CMD_PAYLOAD_DELIM) {
						/* speed data copied, now copy duration*/
						j = 7;
						k++;
					}
					tmp[j] = frame.payload[k];
					j++;
					k++;
				}

				int32_t speed_in_rpm = atoi(tmp);
				uint16_t dur = atoi(tmp+7);

				MC_ProgramSpeedRampMotor1(speed_in_rpm/6, dur);
				MCI_CommandState_t state = MC_GetCommandStateMotor1();

				if (state == MCI_COMMAND_EXECUTED_UNSUCCESFULLY)
					memcpy(cmd_state, M2M_CMD_RESP_ERR, 3);

				sprintf((char*)resp_frame.payload, "%d%c%s%c", M2M_CMD_ID_BLOWER_SPEED_RAMP,
									M2M_CMD_PAYLOAD_DELIM, cmd_state, M2M_CMD_PAYLOAD_TERM);
			}
				break;
			}

			resp_frame.payload_len = strlen((const char*)resp_frame.payload);

			/* serialize the frame and send response */
			/* buffer size = frame header size + pay load size + 1 NULL char */
			uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+resp_frame.payload_len+1;
			uint32_t sdata_len=0;
			m2m_comm_frame_serialize(m_um2m_tx_buf, sbuf_len, &resp_frame, &sdata_len);

			/* transmit the frame */
//			if (HAL_UART_Transmit(&huart1, m_um2m_tx_buf, sdata_len, 500) != HAL_OK) {
//				Error_Handler();
//			}

		  if(HAL_UART_Transmit_IT(&huart1, (uint8_t*)m_um2m_tx_buf, sdata_len)!= HAL_OK) {
		    /* Transfer error in transmission process */
		    Error_Handler();
		  }

		  /* enable USART RX Interrupt */
		  LL_USART_EnableIT_RXNE(USART1);
		}
#endif
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* TIM1_BRK_UP_TRG_COM_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* USART1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(USART1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_LEFT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_TRGO;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc.Init.DMAContinuousRequests = ENABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_3;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x0000020B;
  hi2c1.Init.OwnAddress1 = 224;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = ((TIM_CLOCK_DIVIDER) - 1);
  htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1;
  htim1.Init.Period = ((PWM_PERIOD_CYCLES) / 2);
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV2;
  htim1.Init.RepetitionCounter = (REP_COUNTER);
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC4REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = (((PWM_PERIOD_CYCLES) / 2) - (HTMIN));
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_ENABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_ENABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_1;
  sBreakDeadTimeConfig.DeadTime = ((DEAD_TIME_COUNTS) / 2);
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_ENABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM14 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM14_Init(void)
{

  /* USER CODE BEGIN TIM14_Init 0 */

  /* USER CODE END TIM14_Init 0 */

  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM14_Init 1 */

  /* USER CODE END TIM14_Init 1 */
  htim14.Instance = TIM14;
  htim14.Init.Prescaler = 0;
  htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim14.Init.Period = 0x800;
  htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim14) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0x400;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim14, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM14_Init 2 */

  /* USER CODE END TIM14_Init 2 */
  HAL_TIM_MspPostInit(&htim14);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 921600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OC_SEL_GPIO_Port, OC_SEL_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(_3FG_HIZ_GPIO_Port, _3FG_HIZ_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OCTH_STBY2_GPIO_Port, OCTH_STBY2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OCTH_STBY1_GPIO_Port, OCTH_STBY1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : OC_SEL_Pin _3FG_HIZ_Pin */
  GPIO_InitStruct.Pin = OC_SEL_Pin|_3FG_HIZ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : OCTH_STBY2_Pin OCTH_STBY1_Pin */
  GPIO_InitStruct.Pin = OCTH_STBY2_Pin|OCTH_STBY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
