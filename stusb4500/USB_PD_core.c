#include "USB_PD_core.h"
#include <stdio.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_DECLARE(stusb4500);

//#include "i2c.h"
#define UCPD_PRINT 1

//extern I2C_HandleTypeDef *hi2c[I2CBUS_MAX];
static unsigned int Address;

//extern int PB_press ;
extern uint8_t connection_flag[USBPORT_MAX];

extern uint32_t VBUS_Current_limitation[USBPORT_MAX] ;
uint8_t Policy_Engine_State[USBPORT_MAX];
uint8_t Policy_Engine_Previous_State[USBPORT_MAX];
uint8_t Go_disable_once[USBPORT_MAX];
uint8_t TypeC_state[USBPORT_MAX];
uint8_t TypeC_Previous_state[USBPORT_MAX];
uint8_t Final_Nego_done[USBPORT_MAX] ;
uint8_t Core_Process_suspended = 0 ;

uint8_t Cut[USBPORT_MAX];
STUSB_GEN1S_ALERT_STATUS_MASK_RegTypeDef Alert_Mask;
USB_PD_StatusTypeDef PD_status[USBPORT_MAX] ;
USB_PD_SNK_PDO_TypeDef PDO_SNK[USBPORT_MAX][3];

USB_PD_SRC_PDOTypeDef PDO_FROM_SRC[USBPORT_MAX][7];
uint8_t PDO_FROM_SRC_Num[USBPORT_MAX]={0};
uint8_t PDO_FROM_SRC_Num_Sel[USBPORT_MAX]={0};
uint8_t PDO_FROM_SRC_Valid[USBPORT_MAX]={0};
STUSB_GEN1S_RDO_REG_STATUS_RegTypeDef Nego_RDO[USBPORT_MAX];
uint8_t TypeC_Only_status[USBPORT_MAX] = {0} ;
uint8_t PDO_SNK_NUMB[USBPORT_MAX];


extern uint8_t  USB_PD_Interupt_Flag[USBPORT_MAX] ;
//extern uint8_t USB_PD_Status_change_flag[USBPORT_MAX] ;

//extern USB_PD_I2C_PORT STUSB45DeviceConf[USBPORT_MAX];


/**
* @brief  asserts and de-asserts the STUSB4500 Hardware reset pin.
* @param  I2C Port used (I2C1 or I2C2).
* @param  none
* @retval none
*/

/************************   HW_Reset_state(uint8_t Port)  ***************************
This function asserts and de-asserts the STUSB4500 Hardware reset pin.  
After reset, STUSB4500 behave according to Non Volatile Memory defaults settings. 
************************************************************************************/
void HW_Reset_state(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  
//  LOCK_I2C_RESOURCE();
//  HAL_GPIO_WritePin(Reset_GPIO_Port,Reset_Pin,GPIO_PIN_SET);
  stusb4500_reset_pin_set(config);
//  HAL_Delay(27);  /*time to be dedected by the source almost error recovery time*/
  k_sleep(K_MSEC(27));
//  HAL_GPIO_WritePin(Reset_GPIO_Port,Reset_Pin,GPIO_PIN_RESET);
  stusb4500_reset_pin_clear(config);
//  HAL_Delay(30); /* this to live time for Device to load NVM*/
  k_sleep(K_MSEC(30));
  usb_pd_init(config, Usb_Port);
  connection_flag[Usb_Port] = 1;
//  UNLOCK_I2C_RESOURCE();
  
}

/************************   SW_reset_by_Reg (uint8_t Port)  *************************
This function resets STUSB45 type-C and USB PD state machines. It also clears any
ALERT. By initialisating Type-C pull-down termination, it forces electrical USB type-C
disconnection (both on SOURCE and SINK sides). 
************************************************************************************/

void SW_reset_by_Reg(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  int Status;
  uint8_t Buffer[12];
  Buffer[0] = 1;
//  LOCK_I2C_RESOURCE();
//  Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,STUSB_GEN1S_RESET_CTRL_REG,&Buffer[0],1 );
  Status = stusb4500_write_regs(config, STUSB_GEN1S_RESET_CTRL_REG, &Buffer[0], 1);
  if ( Status == HAL_OK)
  {
//    Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,ALERT_STATUS_1 ,&Buffer[0], 12 );  // clear ALERT Status
    Status = stusb4500_read_regs(config, ALERT_STATUS_1, &Buffer[0], 12);
//    HAL_Delay(200); // on source , the debounce time is more than 15ms error recovery < at 275ms
    k_sleep(K_MSEC(200)); // on source , the debounce time is more than 15ms error recovery < at 275ms
    Buffer[0] = 0; 
//    Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,STUSB_GEN1S_RESET_CTRL_REG,&Buffer[0],1 );
    Status = stusb4500_write_regs(config, STUSB_GEN1S_RESET_CTRL_REG, &Buffer[0], 1);
  }
//  UNLOCK_I2C_RESOURCE();
}

/************************   Send_Soft_reset_Message (uint8_t Usb_Port)  ***************************/
/**
* @brief Send Power delivery reset message). 
* @param  I2C Port used (I2C1 or I2C2).
* @retval none
*/

int Send_Soft_reset_Message(const struct stusb4500_config *config, uint8_t Usb_Port)
 {
	int Status;
	unsigned char DataRW[2];
	// Set Tx Header to Soft Reset
	DataRW[0] = Soft_Reset_Message_type;
//  Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,TX_HEADER ,&DataRW[0], 1 );
	Status = stusb4500_write_regs(config, TX_HEADER, &DataRW[0], 1);
	// send command message
	if (Status == HAL_OK) {
		DataRW[0] = Send_Message;
//    Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,STUSB_GEN1S_CMD_CTRL ,&DataRW[0], 1 );
		Status = stusb4500_write_regs(config, STUSB_GEN1S_CMD_CTRL, &DataRW[0], 1);
	}
	PDO_FROM_SRC_Valid[Usb_Port] = 0;
	return Status;
}


/***************************   usb_pd_init(uint8_t Port)  ***************************
this function clears all interrupts and unmask the usefull interrupt

************************************************************************************/

int usb_pd_init(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  int Status = HAL_OK ;
  static unsigned char DataRW[13];	
  DataRW[0]= 0;
  uint8_t ID_OK=0;
  int count = 10;
  do /* wait for NVM to be reloaded */
  {
//    Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit,DEVICE_ID ,&Cut[Usb_Port], 1 );
    Status = stusb4500_read_regs(config, DEVICE_ID, &Cut[Usb_Port], 1);
    
    if (Cut[Usb_Port] == (uint8_t)0x21)  ID_OK = 1;  // ST eval board
    if (Cut[Usb_Port] == (uint8_t)0x25)  ID_OK = 1;  // Product 
  } while((ID_OK == 0) && (count--));

  if (Status != HAL_OK) {
	  LOG_ERR("device ID not matched!");
	  return -1;
  }
//  I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit,DEVICE_ID ,&Cut[Usb_Port], 1 );
  Status = stusb4500_read_regs(config, DEVICE_ID, &Cut[Usb_Port], 1);
  
#ifdef PRINTF
  if ((Cut[Usb_Port] == 0x21 )|| (Cut[Usb_Port] == 0x25) )
  { 
    printf("\r\n------------------------------------------------");
    printf("\r\n---- INIT: STUSB4500 detected               ----");
    printf("\r\n- Port #%i                                   ----",Usb_Port);
    printf("\r\n- Device ID: 0x%02x                           ----",Cut[Usb_Port]);
    printf("\r\n------------------------------------------------\r\n");
  }
  else printf(" STUSB45 Not detected\r\n");
#endif
  
  Alert_Mask.d8 = 0xFF;  
  Alert_Mask.b.CC_DETECTION_STATUS_AL_MASK = 0;
  Alert_Mask.b.PD_TYPEC_STATUS_AL_MASK = 1;
  Alert_Mask.b.PRT_STATUS_AL_MASK = 0;
  Alert_Mask.b.MONITORING_STATUS_AL_MASK = 0;
  Alert_Mask.b.HARD_RESET_AL_MASK = 1;
  DataRW[0]= Alert_Mask.d8;

  if ( Status == HAL_OK) {
//    Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,ALERT_STATUS_MASK ,&DataRW[0], 1 ); // unmask port status alarm
    Status = stusb4500_write_regs(config, ALERT_STATUS_MASK, &DataRW[0], 1);
    if (Status != HAL_OK) {
  	  LOG_ERR("stusb4500_write_regs failed");
  	  return Status;
    }
  }
  
  /* clear ALERT Status */
//  Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,ALERT_STATUS_1 ,&DataRW[0], 12 );
  Status = stusb4500_read_regs(config, ALERT_STATUS_1, &DataRW[0], 12);
  
  USB_PD_Interupt_Flag[Usb_Port] =0;
  PD_status[Usb_Port].Port_Status.d8 = DataRW[ 3 ] ;
  PD_status[Usb_Port].CC_status.d8 = DataRW[6];
  PD_status[Usb_Port].HWFault_status.d8 = DataRW[8];
  PD_status[Usb_Port].Monitoring_status.d8=DataRW[5];
  TypeC_Only_status[Usb_Port] = 0;

  return Status;
}

/**********************   ALARM_MANAGEMENT(uint8_t Port)  ***************************
device interrupt Handler

************************************************************************************/
#if (CONFIG_STUSB4500_INTERRUPT)
void ALARM_MANAGEMENT(const struct stusb4500_config *config, uint8_t Usb_Port)
{
	int Status = HAL_OK;
	STUSB_GEN1S_ALERT_STATUS_RegTypeDef Alert_Status;
	STUSB_GEN1S_ALERT_STATUS_MASK_RegTypeDef Alert_Mask;
	static unsigned char DataRW[40];

//  if ( HAL_GPIO_ReadPin(ALERT_A_GPIO_Port,ALERT_A_Pin)== GPIO_PIN_RESET)
	if (stusb4500_alert_pin_read(config) == GPIO_PIN_RESET) {
		Address = ALERT_STATUS_1;
//    Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,Address ,&DataRW[0], 2 );
		Status = stusb4500_read_regs(config, Address, &DataRW[0], 2);
		UNUSED(Status);
		Alert_Mask.d8 = DataRW[1];
		Alert_Status.d8 = DataRW[0] & ~Alert_Mask.d8;
		if (Alert_Status.d8 != 0) {
			PD_status[Usb_Port].HW_Reset = (DataRW[0] >> 7);
			if (Alert_Status.b.CC_DETECTION_STATUS_AL != 0) {
				connection_flag[Usb_Port] = 1;
//        Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,PORT_STATUS_TRANS ,&DataRW[0], 2 );
				Status = stusb4500_read_regs(config, PORT_STATUS_TRANS, &DataRW[0], 2);
				PD_status[Usb_Port].Port_Status.d8 = DataRW[1];
				if (PD_status[Usb_Port].Port_Status.b.CC_ATTACH_STATE != 0) {
//          Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,CC_STATUS ,&PD_status[Usb_Port].CC_status.d8, 1 );
					Status = stusb4500_read_regs(config, CC_STATUS, &PD_status[Usb_Port].CC_status.d8, 1);
				} else /* Detached detected */
				{
					connection_flag[Usb_Port] = 1;
					PDO_FROM_SRC_Valid[Usb_Port] = 0;
				}
			}
			if (Alert_Status.b.MONITORING_STATUS_AL != 0) {
//        Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,TYPEC_MONITORING_STATUS_0 ,&DataRW[0], 2 );
				Status = stusb4500_read_regs(config, TYPEC_MONITORING_STATUS_0, &DataRW[0], 2);
				PD_status[Usb_Port].Monitoring_status.d8 = DataRW[1];
			}
//      Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,CC_STATUS ,&DataRW[0], 1);
			Status = stusb4500_read_regs(config, CC_STATUS, &DataRW[0], 1);
			PD_status[Usb_Port].CC_status.d8 = DataRW[0];

			if (Alert_Status.b.HW_FAULT_STATUS_AL != 0) {
//        Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,CC_HW_FAULT_STATUS_0 ,&DataRW[0], 2 );
				Status = stusb4500_read_regs(config, CC_HW_FAULT_STATUS_0, &DataRW[0], 2);
				PD_status[Usb_Port].HWFault_status.d8 = DataRW[1];
			}

			if (Alert_Status.b.PRT_STATUS_AL != 0) {
				USBPD_MsgHeader_TypeDef Header;
//        Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,PRT_STATUS ,&PD_status[Usb_Port].PRT_status.d8, 1 );
				Status = stusb4500_read_regs(config, PRT_STATUS, &PD_status[Usb_Port].PRT_status.d8, 1);

				if (PD_status[Usb_Port].PRT_status.b.MSG_RECEIVED == 1) {
//          Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,RX_HEADER ,&DataRW[0], 2 );
					Status = stusb4500_read_regs(config, RX_HEADER, &DataRW[0], 2);
					Header.d16 = LE16(&DataRW[0]);
					if (Header.b.NumberOfDataObjects > 0) {
						switch (Header.b.MessageType) {
						case 0x01: {
							static int i, j;
//                Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,RX_DATA_OBJ ,&DataRW[0], Header.b.NumberOfDataObjects * 4 );
							Status = stusb4500_read_regs(config, RX_DATA_OBJ, &DataRW[0], Header.b.NumberOfDataObjects * 4);
							j = 0;

							PDO_FROM_SRC_Num[Usb_Port] =
									Header.b.NumberOfDataObjects;
							for (i = 0; i < Header.b.NumberOfDataObjects; i++) {
								PDO_FROM_SRC[Usb_Port][i].d32 =
										(uint32_t) (DataRW[j]
												+ (DataRW[j + 1] << 8)
												+ (DataRW[j + 2] << 16)
												+ (DataRW[j + 3] << 24));
								j += 4;
							}
							PDO_FROM_SRC_Valid[Usb_Port] = 1;
						}
							break;
						default:
							break;
						}
					} else {
#if 1 //def PRINTF
            __NOP();
                         LOG_INF("Ctrl Message :0x%04x \r\n",Header.d16);
#else 
						__NOP();
#endif
						if (Header.b.MessageType == 0x06) /*if request accepted */
							connection_flag[Usb_Port] = 1;
					}
				}
			}
		}
//    if (HAL_GPIO_ReadPin(ALERT_A_GPIO_Port,ALERT_A_Pin) == GPIO_PIN_RESET)
		if (stusb4500_alert_pin_read(config) == GPIO_PIN_RESET)
			USB_PD_Interupt_Flag[Usb_Port] = 1;
		else
			USB_PD_Interupt_Flag[Usb_Port] = 0;
	}
}
#endif /* CONFIG_STUSB4500_INTERRUPT */


void port_message_rx(const struct stusb4500_config *config, uint8_t Usb_Port)
{
	int Status;
	static unsigned char DataRW[40];
	USBPD_MsgHeader_TypeDef Header;
//  Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,PRT_STATUS ,&PD_status[Usb_Port].PRT_status.d8, 1 );
//	Status = stusb4500_read_regs(config, PRT_STATUS, &PD_status[Usb_Port].PRT_status.d8, 1);

//	if (PD_status[Usb_Port].PRT_status.b.MSG_RECEIVED == 1) {
//          Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,RX_HEADER ,&DataRW[0], 2 );
		Status = stusb4500_read_regs(config, RX_HEADER, &DataRW[0], 2);
		Header.d16 = LE16(&DataRW[0]);
		if (Header.b.NumberOfDataObjects > 0) {
			switch (Header.b.MessageType) {
			case 0x01: {
				static int i, j;
//                Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,RX_DATA_OBJ ,&DataRW[0], Header.b.NumberOfDataObjects * 4 );
				Status = stusb4500_read_regs(config, RX_DATA_OBJ, &DataRW[0],
						Header.b.NumberOfDataObjects * 4);
				j = 0;

				PDO_FROM_SRC_Num[Usb_Port] = Header.b.NumberOfDataObjects;
				for (i = 0; i < Header.b.NumberOfDataObjects; i++) {
					PDO_FROM_SRC[Usb_Port][i].d32 = (uint32_t) (DataRW[j]
							+ (DataRW[j + 1] << 8) + (DataRW[j + 2] << 16)
							+ (DataRW[j + 3] << 24));
					j += 4;
				}
				PDO_FROM_SRC_Valid[Usb_Port] = 1;
			}
				break;
			default:
				break;
			}
		} else {
#if UCPD_PRINT
			__NOP();
			LOG_INF("Ctrl Message :0x%04x \r\n", Header.d16);
#else
			__NOP();
#endif
			if (Header.b.MessageType == 0x06) /*if request accepted */
				connection_flag[Usb_Port] = 1;
		}
//	}
}

/**********************     Read_SNK_PDO(uint8_t Port)   ***************************
This function reads the PDO registers. 

************************************************************************************/


void Read_SNK_PDO(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  static unsigned char DataRW[12];	
  DataRW[0]= 0;
  
  static int i ,j ;
  
  
  Address = DPM_PDO_NUMB ;
  
//  if ( I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,Address ,&DataRW[0], 1 )== HAL_I2C_ERROR_NONE  )
  if (stusb4500_read_regs(config, Address, &DataRW[0], 1) == 0)
  {
    
    PDO_SNK_NUMB[Usb_Port] = (DataRW[0] & 0x03 );
    Address = DPM_SNK_PDO1;
//    I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,Address ,&DataRW[0],PDO_SNK_NUMB[Usb_Port]*4  );
    stusb4500_read_regs(config, Address, &DataRW[0], PDO_SNK_NUMB[Usb_Port]*4);
    j=0;
    for ( i = 0 ; i < PDO_SNK_NUMB[Usb_Port] ; i++)
    {
      PDO_SNK[Usb_Port][i].d32 = (uint32_t )( DataRW[j] +(DataRW[j+1]<<8)+(DataRW[j+2]<<16)+(DataRW[j+3]<<24));
      j +=4;
    }
  }
  
  return;
}



/**********************     Print_SNK_PDO(uint8_t Port)   ***************************
This function print the STUSB4500 PDO to the serial interface. 

************************************************************************************/

void Print_SNK_PDO(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  int i;
  int PDO_V, PDO_V_Min, PDO_V_Max;
  int PDO_I;
  int PDO_P;
  
  static int MAX_POWER = 0;
  MAX_POWER = 0;
  
#ifdef PRINTF
  printf("\r\n---- Usb_Port #%i: Read PDO from STUSB4500 ------\r\n",Usb_Port);
#endif
  Read_SNK_PDO(config, Usb_Port);
#ifdef PRINTF          
  printf("%x x PDO:\r\n",PDO_SNK_NUMB[Usb_Port]);
#endif
  for (i=0; i< PDO_SNK_NUMB[Usb_Port]; i++)
  {
    switch (PDO_SNK[Usb_Port][i].fix.Fixed_Supply)
    {
    case 0:  /* fixed supply */
      {
        PDO_V = (PDO_SNK[Usb_Port][i].fix.Voltage)* 50;
        PDO_I = (PDO_SNK[Usb_Port][i].fix.Operationnal_Current)*10;
        PDO_P = PDO_V*PDO_I; 
#ifdef PRINTF   
        printf(" - PDO%u (fixed)     = (%4.2fV, %4.2fA, = %4.1fW)\r\n",i+1, (float )PDO_V/1000.0,(float )PDO_I/1000.0, (float )PDO_P/1000000.0 );
#endif 
        UNUSED(PDO_V_Min);
        UNUSED(PDO_V_Max);
        if (PDO_P >=MAX_POWER)
        { MAX_POWER = PDO_P;}
      }
      break;
    case 1: /* Variable Supply */
      {
        PDO_V_Max = (PDO_SNK[Usb_Port][i].var.Max_Voltage)*50;
        PDO_V_Min = (PDO_SNK[Usb_Port][i].var.Min_Voltage)*50;
        PDO_I = (PDO_SNK[Usb_Port][i].var.Operating_Current)*10;
#ifdef PRINTF   
        printf(" - PDO%u (variable)  =(%4.2fV, %4.2fV, = %4.2fA)\r\n",i+1, (float )PDO_V_Min/1000.0, (float )PDO_V_Max/1000.0, (float )PDO_I/1000.0 );              
#endif 
      }
      break;
    case 2: /* Battery Supply */
      {
        PDO_V_Max = (PDO_SNK[Usb_Port][i].bat.Max_Voltage)*50;
        PDO_V_Min = (PDO_SNK[Usb_Port][i].bat.Min_Voltage)*50;
        PDO_P =(PDO_SNK[Usb_Port][i].bat.Operating_Power)*250; 
#ifdef PRINTF   
        printf(" - PDO%u (battery)   =(%4.2fV, %4.2fV, = %4.1fW)\r\n",i+1,(float ) PDO_V_Min/1000.0,(float ) PDO_V_Max/1000.0,(float ) PDO_P/1000.0 );
#endif 
        if (PDO_P >=MAX_POWER)
        { MAX_POWER = PDO_P;}
      }
      break;            
    default:
      break;
    }
  }
  
}


/**********************     Read_RDO(uint8_t Port)   ***************************
This function reads the Requested Data Object (RDO) register. 

************************************************************************************/

int Read_RDO(const struct stusb4500_config *config, uint8_t Usb_Port, struct sensor_value *val)
{
//  I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,RDO_REG_STATUS ,(uint8_t *)&Nego_RDO[Usb_Port].d32, 4 );
	int ret = stusb4500_read_regs(config, RDO_REG_STATUS, (uint8_t*) &Nego_RDO[Usb_Port].d32, 4);
	(&val[0])->val1 = Nego_RDO[Usb_Port].b.Object_Pos;
	(&val[1])->val1 = (PDO_FROM_SRC[Usb_Port][Nego_RDO[Usb_Port].b.Object_Pos - 1].fix.Voltage) * 50; // mvolts = (voltage / 20) * 1000 = voltage * 50
	(&val[2])->val1 = Nego_RDO[Usb_Port].b.MaxCurrent * 10;			// mamps = (curr / 100) * 1000
	(&val[2])->val2 = Nego_RDO[Usb_Port].b.OperatingCurrent * 10;	// mamps = (curr / 100) * 1000
	return ret;
}


/**********************     Print_RDO(uint8_t Port)   ***************************
This function prints to the serial interface the current contract in case of 
capability MATCH between the STUSB4500 and the SOURCE.

************************************************************************************/

void Print_RDO(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  static uint8_t CC_Status_resume, Vbus_select;
//  I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,0x21 ,&Vbus_select, 1 );
  stusb4500_read_regs(config, 0x21 ,&Vbus_select, 1);
//  I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,TYPE_C_STATUS ,&CC_Status_resume, 1 );
  stusb4500_read_regs(config, TYPE_C_STATUS, &CC_Status_resume, 1);
  if (Nego_RDO[Usb_Port].d32 != 0)
  { 
#if 1//def PRINTF
    printf(" \r\n---- Usb_Port #%i: CONNECTION STATUS   ----------",Usb_Port);
    printf(" \r\n - CONTRACT        \t: EXPLICIT");
    printf(" \r\n - Requested PDO # \t: %d",Nego_RDO[Usb_Port].b.Object_Pos);
    if ( PDO_FROM_SRC_Valid[Usb_Port] == 1)
      printf(" \r\n - Voltage requested \t: %4.2fV" , (PDO_FROM_SRC[Usb_Port][Nego_RDO[Usb_Port].b.Object_Pos - 1].fix.Voltage/20.0) );
    else printf(" \r\n - PDO SRC not properly red by the SW  " );
    printf(" \r\n - PIN \t\t\t: CC%i" , ((CC_Status_resume >> 7 ) & 1 )+1);
    printf(" \r\n - Max Current \t\t: %4.2fA \r\n - Operating Current \t: %4.2fA \r\n - Capability Mismatch \t: %d ",Nego_RDO[Usb_Port].b.MaxCurrent/100.0,(Nego_RDO[Usb_Port].b.OperatingCurrent/100.0),Nego_RDO[Usb_Port].b.CapaMismatch);
    printf(" \r\n - Give back \t\t: %d ",Nego_RDO[Usb_Port].b.GiveBack);
    printf(" \r\n - USB Com Capable \t: %d ",Nego_RDO[Usb_Port].b.UsbComCap);
    printf(" \r\n - USB suspend \t\t: %d \r\n",Nego_RDO[Usb_Port].b.UsbSuspend);
#else
    __NOP();
#endif    
    
  }
  else
  {
#ifdef PRINTF    
    printf(" \r\n---- Usb_Port #%i: CONNECTION STATUS   ----------",Usb_Port);
    printf(" \r\n - CONTRACT        \t: not yet");
    if(PD_status[Usb_Port].Port_Status.b.CC_ATTACH_STATE  !=0)
      printf("\r\n - Voltage :%4.2fV\r\n" , (float )Vbus_select/10);
#else
    __NOP();
#endif    
  }
}


/******************   Update_PDO(Port, PDO_number, Voltage, Current)   *************
This function must be used to overwrite PDO2 or PDO3 content in RAM.
Arguments are:
- Port Number:
- PDO Number : 2 or 3 , 
- Voltage in(mV) truncated by 50mV ,
- Current in(mv) truncated by 10mA
************************************************************************************/

int Update_PDO(const struct stusb4500_config *config, uint8_t Usb_Port,uint8_t PDO_Number,int Voltage,int Current)
{
	uint8_t adresse;
	int Status = HAL_OK;
	PDO_SNK[Usb_Port][PDO_Number - 1].fix.Voltage = Voltage / 50;
	PDO_SNK[Usb_Port][PDO_Number - 1].fix.Operationnal_Current = Current / 10;
	if ((PDO_Number == 2) || (PDO_Number == 3)) {
		adresse = DPM_SNK_PDO1 + 4 * (PDO_Number - 1);
//    Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,adresse ,(uint8_t *)&PDO_SNK[Usb_Port][PDO_Number - 1].d32, 4 );
		Status = stusb4500_write_regs(config, adresse,
				(uint8_t*) &PDO_SNK[Usb_Port][PDO_Number - 1].d32, 4);
//    UNUSED(Status); /* to remove warning */
		return Status;
	}
	return -1;
}

/******************   Update_PDO1(Port, Current)   *************
This function must be used to overwrite PDO1(5V) current in RAM.
Arguments are:
- Port Number:
- Current in(mv) truncated by 10mA
************************************************************************************/

void Update_PDO1(const struct stusb4500_config *config, uint8_t Usb_Port,int Current)
{
  uint8_t adresse;
  int Status = HAL_OK;
  PDO_SNK[Usb_Port][0].fix.Voltage = 5000 /50 ;
  PDO_SNK[Usb_Port][0 ].fix.Operationnal_Current = Current / 10;
  adresse = DPM_SNK_PDO1  ;
//  Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,adresse ,(uint8_t *)&PDO_SNK[Usb_Port][0].d32, 4 );
  Status = stusb4500_write_regs(config, adresse ,(uint8_t *)&PDO_SNK[Usb_Port][0].d32, 4);
  UNUSED(Status); /* to remove warning */
}

/************* Update_Valid_PDO_Number(Port, PDO_Number)  ***************************
This function is used to overwrite the number of valid PDO
Arguments are: 
- Port Number,
- active PDO Number: from 1 to 3 
************************************************************************************/

void Update_Valid_PDO_Number(const struct stusb4500_config *config, uint8_t Usb_Port,uint8_t Number_PDO)
{
  
  if (Number_PDO >= 1 && Number_PDO <=3)
  {
    PDO_SNK_NUMB[Usb_Port] = Number_PDO;
//    I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,DPM_PDO_NUMB,&Number_PDO,1 );
    stusb4500_write_regs(config, DPM_PDO_NUMB, &Number_PDO, 1);
  }
}

/****************************     Negotiate_5V( Port)    ***************************
Sample function that reconfigures the PDO number to only one, so by default PDO1. 
This drives the STUSB4500 to negotiates 5V back with the SOURCE.

************************************************************************************/ 

void Negotiate_5V(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  Update_Valid_PDO_Number(config, Usb_Port, 1);
}


/**********************     Find_Matching_SRC_PDO(uint8_t Usb_Port,int Min_Power,int Min_V , int Max_V)   ************************/
/**
* @brief scans the SOURCE PDO (received at connection). If one of the SOURCE PDO
falls within the range of the functions arguments, ie. within a Voltage range and 
Power Range relevant for the applications, then it redefines the SINK_PDO3 with such
PDO parameters and re-negotiates. This allows STUSB4500 to best match to the SOURCE
capabilities.
* @param  I2C Port used (I2C1 or I2C2).
* @param  Min Power  in W 
* @param  Min Voltage in mV
* @param  Max Voltage in mV
* @retval 0 if PDO3 updated 1 if not 
*********************************************************************************************************************************/
int Find_Matching_SRC_PDO(const struct stusb4500_config *config, uint8_t Usb_Port,int Min_Power,int Min_V , int Max_V)
{
  static uint8_t i;
  int PDO_V;
  int PDO_I;
  int   PDO_P;
  int PDO1_updated = 0 ;
  if(PDO_FROM_SRC_Num[Usb_Port] > 1 )
  {
    for (i=1; i< PDO_FROM_SRC_Num[Usb_Port]; i++)   // loop started from PDO2 
    {
      PDO_V = PDO_FROM_SRC[Usb_Port][i].fix.Voltage * 50;
      PDO_I = PDO_FROM_SRC[Usb_Port][i].fix.Max_Operating_Current * 10;
      PDO_P = (int)(( PDO_V/1000) * (PDO_I/1000)); 
      if ((PDO_P >=Min_Power ) && (PDO_V > Min_V ) && (PDO_V <= Max_V ))
      {
        Update_PDO(config, Usb_Port, 2 ,PDO_V , PDO_I );
        PDO1_updated = 1 ;
      }              
    }
    
    Update_Valid_PDO_Number(config, Usb_Port,2);
  }
  else Negotiate_5V(config, Usb_Port); // only 5V SRC PDO avalable
    
  
  if(PDO1_updated)
  {
    return 0;
  }
  return 1;
  
}

/************ Request_SRC_PDO_NUMBER(uint8_t Usb_Port, uint8_t SRC_PDO_position)   ******************/
/*
* @brief This function copies the SRC_PDO corresponding to the position set in parameter into STUSB4500 PDO2
This allows STUSB4500 to negotiate with the SOURCE on the given PDO index, whatever its Voltage node.
* @param  I2C Port used (I2C1 or I2C2).
* @param  SRC_PDO_index
* @retval 0 if PDO updated 1 if not 
******************************************************************************************************/
int Request_SRC_PDO_NUMBER(const struct stusb4500_config *config, uint8_t Usb_Port, uint8_t SRC_PDO_position)
{
  int PDO_V;
  int PDO_I;
  int PDO1_updated = 0 ;
  
  if( SRC_PDO_position < 1) 
  {
#ifdef PRINTF
    printf("\r\n Request_SRC_PDO_NUMBER: PDO index must be > 0 \r\n");
    return 1;
#endif   
  }
  else if( SRC_PDO_position == 1 )
  {
    Update_Valid_PDO_Number(config, Usb_Port,1);
  }
  
  else if( SRC_PDO_position <= PDO_FROM_SRC_Num_Sel[Usb_Port] )
  {
    if (PDO_FROM_SRC[Usb_Port][SRC_PDO_position-1].fix.FixedSupply == 00 )
    {
      PDO_V = PDO_FROM_SRC[Usb_Port][SRC_PDO_position-1].fix.Voltage * 50;
      PDO_I = PDO_FROM_SRC[Usb_Port][SRC_PDO_position-1].fix.Max_Operating_Current * 10;
      
      Update_PDO(config, Usb_Port, 2 ,PDO_V , PDO_I );
      PDO1_updated = 1 ;
      Update_Valid_PDO_Number(config, Usb_Port,2);
    }
    else 
    {
#ifdef PRINTF
      printf("\r\n Request_SRC_PDO_NUMBER: PDO index must be <= %i \r\n", PDO_FROM_SRC_Num_Sel[Usb_Port]);
#endif
      return 1;
    }
  }
  
  if(PDO1_updated)
    return 0;
  
  return 1; 
}

/**********************     Print_PDO_FROM_SRC(uint8_t Usb_Port)    *************************** 
This function prints the SOURCE capabilities received by the STUSB4500. 
SOURCE capabilities are automatically stored at device connection in a dedicated structure.
***********************************************************************************************/

int Print_PDO_FROM_SRC(const struct stusb4500_config *config, uint8_t Usb_Port, struct sensor_value *val)
 {
	int i;
	int PDO_V, PDO_V_Min, PDO_V_Max;
	int PDO_I;
	float PDO_P;
	float MAX_POWER = 0;
	MAX_POWER = 0;

	if (PDO_FROM_SRC_Valid[Usb_Port] == 1) {
#ifdef UCPD_PRINT
		LOG_INF("\r\n---- Usb_Port #%i: Read PDO from SOURCE ---------\r\n", Usb_Port);
#endif
		int num_pdo = PDO_FROM_SRC_Num[Usb_Port];
		for (i = 0; i < PDO_FROM_SRC_Num[Usb_Port]; i++) // Check that PDO is not a APDO
				{
			if ((PDO_FROM_SRC[Usb_Port][i].d32 & 0xC0000000) != 0x00000000)
				num_pdo--;

		}
		PDO_FROM_SRC_Num_Sel[Usb_Port] = num_pdo;

		(&val[0])->val1 = PDO_FROM_SRC_Num_Sel[Usb_Port];	// number of selectable PDOs

#ifdef UCPD_PRINT
		LOG_INF("%i Objects => %i fixed PDO:\r\n", PDO_FROM_SRC_Num[Usb_Port], PDO_FROM_SRC_Num_Sel[Usb_Port]);
#endif
		for (i = 0; i < PDO_FROM_SRC_Num[Usb_Port]; i++) {
			switch (PDO_FROM_SRC[Usb_Port][i].fix.FixedSupply) {
			case 0: /* fixed supply */
			{
				PDO_V = (PDO_FROM_SRC[Usb_Port][i].fix.Voltage) * 50;
				PDO_I = (PDO_FROM_SRC[Usb_Port][i].fix.Max_Operating_Current) * 10;
				PDO_P = PDO_V * PDO_I;

				(&val[i+1])->val1 = PDO_V;	// mv
				(&val[i+1])->val2 = PDO_I;	// ma

#ifdef UCPD_PRINT
				LOG_INF(" - PDO%u (fixed)     = (%4.2fV, %4.2fA, = %4.1fW)\r\n", i+1, (double )PDO_V/1000.0, (double )PDO_I/1000.0, (double)PDO_P/1000000.0);
#endif 
				if (PDO_P >= MAX_POWER) {
					MAX_POWER = PDO_P / (float)1000000.0;
				}
			}
				break;
			case 2: /* Variable Supply */
			{
				PDO_V_Max = (PDO_FROM_SRC[Usb_Port][i].var.Max_Voltage) * 50;
				PDO_V_Min = (PDO_FROM_SRC[Usb_Port][i].var.Min_Voltage) * 50;
				PDO_I = (PDO_FROM_SRC[Usb_Port][i].var.Operating_Current) * 10;
#ifdef UCPD_PRINT
				LOG_INF(" - PDO%u (variable)  = (%4.2fV, %4.2fV, = %4.2fA) not selectable\r\n", i+1, (double )PDO_V_Min/1000.0, (double )PDO_V_Max/1000.0, (double )PDO_I/1000.0);
#endif 
				UNUSED(PDO_V_Min);
				PDO_P = PDO_V_Max * PDO_I / 10000000;
				if (PDO_P >= MAX_POWER) {
					MAX_POWER = PDO_P;
				}
			}
				break;
			case 1: /* Battery Supply */
			{
				PDO_V_Max = (PDO_FROM_SRC[Usb_Port][i].bat.Max_Voltage) * 50;
				PDO_V_Min = (PDO_FROM_SRC[Usb_Port][i].bat.Min_Voltage) * 50;
				PDO_P = (PDO_FROM_SRC[Usb_Port][i].bat.Operating_Power) * 250;
#ifdef UCPD_PRINT
				LOG_INF(" - PDO%u (battery)   = (%4.2fV, %4.2fV, = %4.1fW) not selectable\r\n", i+1, (double ) PDO_V_Min/1000.0, (double ) PDO_V_Max/1000.0, (double ) PDO_P/1000);
#endif 
				if ((float) PDO_P / 1000 >= MAX_POWER) {
					MAX_POWER = PDO_P / 1000;
				}
			}
				break;
			case 3: /* Augmented Supply */
			{
				PDO_V_Max = (PDO_FROM_SRC[Usb_Port][i].apdo.Max_Voltage) * 100;
				PDO_V_Min =
						(uint8_t) (PDO_FROM_SRC[Usb_Port][i].apdo.Min_Voltage)
								* 100;
				PDO_I = (PDO_FROM_SRC[Usb_Port][i].apdo.Max_Current) * 50;
				PDO_P = (float) (PDO_V_Max * PDO_I) / 1000000;
#ifdef UCPD_PRINT
				LOG_INF(" - PDO%u (augmented) = (%4.2fV, %4.2fV, %4.2fA = %4.1fW) => APDO not selectable \r\n", i+1, (double ) PDO_V_Min/1000.0, (double ) PDO_V_Max/1000.0, (double ) PDO_I/1000.0, (double)PDO_P);
#endif 
				if (PDO_P >= MAX_POWER) {
					MAX_POWER = PDO_P;
				}
			}
				break;
			}
		}
#ifdef UCPD_PRINT
		LOG_INF("P(max)=%4.1fW\r\n", (double)MAX_POWER);
#endif  
	} else {
#ifdef UCPD_PRINT
		LOG_INF("\r\n---- Usb_Port #%i: PDO from SOURCE not read  ----\r\n", Usb_Port);
#else 
    __NOP();
#endif
    	return -1;
	}
	return 0;
}

/*****************************     Get_Device_STATUS(uint8_t Usb_Port)    ********************************* 
This function prints the PD information received from the STUSB4500, vs Policy and type C status 
* @param  Port used ( ).
* @retval 0 Device is not connected 
* @retval 1 Device Connected in type C only 
* @retval 2 Device connected but PDO from source are not available soft reset requested 
* @retval 3 Device connected 5V PD 
* @retval 4 Device connected but no ideal matching found 
* @retval 5 Device connected matching found request ongoing 
* @retval 6 Device connected matching found power is OK  
* @retval 7 Device is not connected but in attached wait (monitoring issue ) reset by sw register is on going  
* @retval 8 Default no action
* @retval 9 Hard reset message received
******************************************************************************************************/
int Get_Device_STATUS(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  int Status;
  
  if(USB_PD_Interupt_Flag[Usb_Port] != 1)
  {
    if(PD_status[Usb_Port].Port_Status.b.CC_ATTACH_STATE  !=0) /* this status is updated by Alert routine */ 
    {
      bool Policy_Engine_Changed = false;
      Policy_Engine_Previous_State[Usb_Port] = Policy_Engine_State[Usb_Port];
      if ( Final_Nego_done[Usb_Port] == 0 ) {
//        Status=I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,PE_FSM ,&Policy_Engine_State, 1 );
    	  Status = stusb4500_read_regs(config, PE_FSM, &Policy_Engine_State[0], 1);
      }
      if ( Policy_Engine_Previous_State[Usb_Port] != Policy_Engine_State[Usb_Port]) Policy_Engine_Changed = true; else Policy_Engine_Changed = false;
      UNUSED(Status); /* to remove warning */
      switch (Policy_Engine_State[Usb_Port])
      {
      case PE_SNK_READY:
        {  
          unsigned char DataRW[2];
//          Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,0x20 ,&DataRW[0], 1);
          Status = stusb4500_read_regs(config, 0x20, &DataRW[0], 1);
          DataRW[0] = DataRW[0] & 0x24;
//          Status = I2C_Write_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,0x20 ,&DataRW[0], 1 );
          Status = stusb4500_write_regs(config, 0x20, &DataRW[0], 1);
          if ( Policy_Engine_Changed )
          {
            connection_flag[Usb_Port] = 0;
            //            ConnectionStamp[Usb_Port] = 0;
            Print_SNK_PDO(config, Usb_Port);
            struct sensor_value val[7];	// dummy
            Print_PDO_FROM_SRC(config, Usb_Port, val);
            struct sensor_value rdo[7];	// dummy
            Read_RDO(config, Usb_Port, rdo);
            Print_RDO(config, Usb_Port);
            if(PDO_FROM_SRC_Valid[Usb_Port] != 1 )
            {
              connection_flag[Usb_Port] = 1;
              Policy_Engine_State[Usb_Port] = 0;
              return Connected_Unknown_SRC_PDOs ;
            }
            else
            {
#ifndef TIMER_ACTION 
              if (Find_Matching_SRC_PDO(config, Usb_Port, 7, 8000, 13000) == 0) /* 0 means find matching PDO in source list to modify or fine tune primary STUSB4500 choise  */
#else
              if(1)
#endif                     
                {
                  if ( (PDO_SNK_NUMB[Usb_Port] > 1) && (PDO_FROM_SRC_Num[Usb_Port] > 1))
                  { 
                    if (( PDO_FROM_SRC[Usb_Port][Nego_RDO[Usb_Port].b.Object_Pos - 1].fix.Voltage == PDO_SNK[Usb_Port][1].fix.Voltage ) || 
                        ( PDO_FROM_SRC[Usb_Port][Nego_RDO[Usb_Port].b.Object_Pos - 1].fix.Voltage == PDO_SNK[Usb_Port][2].fix.Voltage ))
                    {
                      VBUS_Current_limitation[Usb_Port] = Nego_RDO[Usb_Port].b.OperatingCurrent*10 ;
                      Final_Nego_done[Usb_Port] = 1;
                      return Connected_Mached ;
                    }
                    else 
                    {
                      Policy_Engine_State[Usb_Port] = 0;
                      return Connected_Matching_ongoing ;
                    }
                  }
                  else // Only 1 5V PDO 
                  {
                    VBUS_Current_limitation[Usb_Port] = Nego_RDO[Usb_Port].b.OperatingCurrent*10 ;
                    Final_Nego_done[Usb_Port] = 1;
                    return Connected_5V_PD ;
                  }
                }
                else 
                {
                  VBUS_Current_limitation[Usb_Port] = Nego_RDO[Usb_Port].b.OperatingCurrent*10 ;
                  Final_Nego_done[Usb_Port] = 1;
                  return Connected_no_Match_found ;
                }
            }
          }
        }
        break;
      case PE_HARD_RESET:
        if ( Policy_Engine_Changed )
          return Hard_Reset_ongoing ; 
        else return Policy_Defauld;
          
        break;
        
      case PE_DISABLED_STATE:
        if ( Go_disable_once[Usb_Port] == 0 )
        {
          Go_disable_once[Usb_Port] =1 ;
          Final_Nego_done[Usb_Port] = 1;
          return TypeC_Only;
        }
        break;
      default :
        break;
        
      }
    }
    else 
    {
      VBUS_Current_limitation[Usb_Port] = 5; 
      if (connection_flag[Usb_Port] == 1)
      {
        
        bool TypeC_Changed ;
        TypeC_Previous_state[Usb_Port] = TypeC_state[Usb_Port];  
//        I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,TYPE_C_STATUS ,&TypeC_state[Usb_Port], 1 );
        stusb4500_read_regs(config, TYPE_C_STATUS ,&TypeC_state[Usb_Port], 1);
        if ( TypeC_Previous_state[Usb_Port] != TypeC_state[Usb_Port]) TypeC_Changed = true; else TypeC_Changed = false;
        connection_flag[Usb_Port] = 0; 
        Go_disable_once[Usb_Port] = 0;
        USB_PD_Interupt_Flag[Usb_Port] = 0;
        Final_Nego_done[Usb_Port]=0;  
        
        switch (TypeC_state[Usb_Port] & 0x1F )
        {
        case 0 : /* Unattached SNK */ 
          Final_Nego_done[Usb_Port] = 1;
          Policy_Engine_State[Usb_Port] = 0;
          return Not_Connected; 
          break;
        case 1 :  /* Attache wait SNK */
          if ( TypeC_Changed )
          {
            SW_reset_by_Reg(config, Usb_Port) ;
          }
          return Not_Connected_attached_wait;
          break;
        default: 
           break;
        } 
      }
      
    }
    
  }
  return Policy_Defauld ;
}

/**********************     Print_Type_C_Only_Status(uint8_t Usb_Port)    *************************** 
This function prints the USB-C information received by the STUSB4500, such as CC pin 
location and Rp value:
- CC pin information is usefull for USB3.x data signals routing (TX/RX)
- Rp value information is usefull to set charging current accordingly (when not in explicit contract)
***********************************************************************************************/

void Print_Type_C_Only_Status(const struct stusb4500_config *config, uint8_t Usb_Port)
{
  int Status;
  if(1)
    /* Status of Type C only is comfirm only after 500 ms to verify that no PD messages are sent by conterpart */
  {
#ifdef UCPD_PRINT
    printf(" \r\n---- Usb_Port #%i: CONNECTION STATUS   ----------",Usb_Port);
    printf(" \r\n - ATTACHED to USB-C only device");
#endif    
//    Status = I2C_Read_USB_PD(STUSB45DeviceConf[Usb_Port].I2cBus,STUSB45DeviceConf[Usb_Port].I2cDeviceID_7bit ,CC_STATUS ,&PD_status[Usb_Port].CC_status.d8, 1 );
    Status = stusb4500_read_regs(config, CC_STATUS, &PD_status[Usb_Port].CC_status.d8, 1);
    UNUSED(Status); /* to remove warning */
    //ConnectionStamp[Usb_Port] = 0;
    if ( PD_status[Usb_Port].CC_status.b.CONNECT_RESULT)
    {
      TypeC_Only_status[Usb_Port] = 1;
      switch( ( PD_status[Usb_Port].CC_status.b.CC1_STATE ) | ( PD_status[Usb_Port].CC_status.b.CC2_STATE << 2 ) )
      {
      case 0x1:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC1");
        printf(" \r\n - Power : Rp = USB Default");
#endif    
        VBUS_Current_limitation[Usb_Port] = 500 ;
        break;
      case 0x4:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC2");
        printf(" \r\n - Power : Rp = USB Default");
#endif  
        VBUS_Current_limitation[Usb_Port] = 500 ;
        break ;
      case 0x2:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC1");
        printf(" \r\n - Power : Rp = 1.5A");
#endif  
        VBUS_Current_limitation[Usb_Port] = 1500 ;
        break;
      case 0x8:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC2");
        printf(" \r\n - Power : Rp = 1.5A");
#endif  
        VBUS_Current_limitation[Usb_Port] = 1500 ;
        break ;
      case 0x3:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC1");
        printf(" \r\n - Power : Rp = 3A");
#endif  
        VBUS_Current_limitation[Usb_Port] = 3000 ;
        break;
      case 0xC:
#ifdef UCPD_PRINT
        printf(" \r\n - PIN   : CC2");
        printf(" \r\n - Power : Rp = 3A");
#endif  
        VBUS_Current_limitation[Usb_Port] = 3000 ;
        break ;
      default :
#ifdef UCPD_PRINT
        printf (" \r\n - DEVICE not supported");
#else  
        __NOP();    
#endif
        break ;
      }
    }
  }   
}

