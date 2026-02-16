/*
 * Copyright (c) 2021 Acme CPU
 *
 * stm32g4xx_flash.h
 * Created on: 18-Oct-2021
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#ifndef COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32G4XX_FLASH_H_
#define COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32G4XX_FLASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
  * @brief  FLASH Option Bytes Program structure definition
  */
typedef struct
{
  uint32_t OptionType;     /*!< Option byte to be configured.
                                This parameter can be a combination of the values of @ref FLASH_OB_Type */
  uint32_t WRPArea;        /*!< Write protection area to be programmed (used for OPTIONBYTE_WRP).
                                Only one WRP area could be programmed at the same time.
                                This parameter can be value of @ref FLASH_OB_WRP_Area */
  uint32_t WRPStartOffset; /*!< Write protection start offset (used for OPTIONBYTE_WRP).
                                This parameter must be a value between 0 and (max number of pages in the bank - 1) */
  uint32_t WRPEndOffset;   /*!< Write protection end offset (used for OPTIONBYTE_WRP).
                                This parameter must be a value between WRPStartOffset and (max number of pages in the bank - 1) */
  uint32_t RDPLevel;       /*!< Set the read protection level.. (used for OPTIONBYTE_RDP).
                                This parameter can be a value of @ref FLASH_OB_Read_Protection */
  uint32_t USERType;       /*!< User option byte(s) to be configured (used for OPTIONBYTE_USER).
                                This parameter can be a combination of @ref FLASH_OB_USER_Type */
  uint32_t USERConfig;     /*!< Value of the user option byte (used for OPTIONBYTE_USER).
                                This parameter can be a combination of @ref FLASH_OB_USER_BOR_LEVEL,
                                @ref FLASH_OB_USER_nRST_STOP, @ref FLASH_OB_USER_nRST_STANDBY,
                                @ref FLASH_OB_USER_nRST_SHUTDOWN, @ref FLASH_OB_USER_IWDG_SW,
                                @ref FLASH_OB_USER_IWDG_STOP, @ref FLASH_OB_USER_IWDG_STANDBY,
                                @ref FLASH_OB_USER_WWDG_SW, @ref FLASH_OB_USER_BFB2 (*),
                                @ref FLASH_OB_USER_nBOOT1, @ref FLASH_OB_USER_SRAM_PE,
                                @ref FLASH_OB_USER_CCMSRAM_RST
                                @note (*) availability depends on devices */
  uint32_t PCROPConfig;    /*!< Configuration of the PCROP (used for OPTIONBYTE_PCROP).
                                This parameter must be a combination of @ref FLASH_Banks (except FLASH_BANK_BOTH)
                                and @ref FLASH_OB_PCROP_RDP */
  uint32_t PCROPStartAddr; /*!< PCROP Start address (used for OPTIONBYTE_PCROP).
                                This parameter must be a value between begin and end of bank
                                => Be careful of the bank swapping for the address */
  uint32_t PCROPEndAddr;   /*!< PCROP End address (used for OPTIONBYTE_PCROP).
                                This parameter must be a value between PCROP Start address and end of bank */
  uint32_t BootEntryPoint; /*!< Set the Boot Lock (used for OPTIONBYTE_BOOT_LOCK).
                                This parameter can be a value of @ref FLASH_OB_Boot_Lock */
  uint32_t SecBank;        /*!< Bank of securable memory area to be programmed (used for OPTIONBYTE_SEC).
                                Only one securable memory area could be programmed at the same time.
                                This parameter can be one of the following values:
                                FLASH_BANK_1: Securable memory area to be programmed in bank 1
                                FLASH_BANK_2: Securable memory area to be programmed in bank 2 (*)
                                @note (*) availability depends on devices */
  uint32_t SecSize;        /*!< Size of securable memory area to be programmed (used for OPTIONBYTE_SEC),
                                in number of pages. Securable memory area is starting from first page of the bank.
                                Only one securable memory could be programmed at the same time.
                                This parameter must be a value between 0 and (max number of pages in the bank - 1) */
} FLASH_OBProgramInitTypeDef;

typedef struct {
	/* Flash option register */
	uint32_t flash_optr;
	uint32_t flash_optr_c;

	/* Flash PCROP start address register */
	uint32_t flash_pcrop_sr;
	uint32_t flash_pcrop_sr_c;

	/* Flash PCROP end address register */
	uint32_t flash_pcrop_er;
	uint32_t flash_pcrop_er_c;

	/* Flash WRP Area A address register */
	uint32_t flash_wrpar;
	uint32_t flash_wrpar_c;

	/* Flash WRP Area B address register */
	uint32_t flash_wrpbr;
	uint32_t flash_wrpbr_c;

	/* Flash Secure Protection register */
	uint32_t flash_sec;
	uint32_t flash_sec_c;
} FLASH_OB_REG;

/**
  * @}
  */

/*******************  Bits definition for FLASH_OPTR register  ***************/
#define FLASH_OPTR_RDP_Pos                (0U)
#define FLASH_OPTR_RDP_Msk                (0xFFUL << FLASH_OPTR_RDP_Pos)       /*!< 0x000000FF */
#define FLASH_OPTR_RDP                    FLASH_OPTR_RDP_Msk
#define FLASH_OPTR_BOR_LEV_Pos            (8U)
#define FLASH_OPTR_BOR_LEV_Msk            (0x7UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000700 */
#define FLASH_OPTR_BOR_LEV                FLASH_OPTR_BOR_LEV_Msk
#define FLASH_OPTR_BOR_LEV_0              (0x0UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000000 */
#define FLASH_OPTR_BOR_LEV_1              (0x1UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000100 */
#define FLASH_OPTR_BOR_LEV_2              (0x2UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000200 */
#define FLASH_OPTR_BOR_LEV_3              (0x3UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000300 */
#define FLASH_OPTR_BOR_LEV_4              (0x4UL << FLASH_OPTR_BOR_LEV_Pos)    /*!< 0x00000400 */
#define FLASH_OPTR_nRST_STOP_Pos          (12U)
#define FLASH_OPTR_nRST_STOP_Msk          (0x1UL << FLASH_OPTR_nRST_STOP_Pos)  /*!< 0x00001000 */
#define FLASH_OPTR_nRST_STOP              FLASH_OPTR_nRST_STOP_Msk
#define FLASH_OPTR_nRST_STDBY_Pos         (13U)
#define FLASH_OPTR_nRST_STDBY_Msk         (0x1UL << FLASH_OPTR_nRST_STDBY_Pos) /*!< 0x00002000 */
#define FLASH_OPTR_nRST_STDBY             FLASH_OPTR_nRST_STDBY_Msk
#define FLASH_OPTR_nRST_SHDW_Pos          (14U)
#define FLASH_OPTR_nRST_SHDW_Msk          (0x1UL << FLASH_OPTR_nRST_SHDW_Pos)  /*!< 0x00004000 */
#define FLASH_OPTR_nRST_SHDW              FLASH_OPTR_nRST_SHDW_Msk
#define FLASH_OPTR_IWDG_SW_Pos            (16U)
#define FLASH_OPTR_IWDG_SW_Msk            (0x1UL << FLASH_OPTR_IWDG_SW_Pos)    /*!< 0x00010000 */
#define FLASH_OPTR_IWDG_SW                FLASH_OPTR_IWDG_SW_Msk
#define FLASH_OPTR_IWDG_STOP_Pos          (17U)
#define FLASH_OPTR_IWDG_STOP_Msk          (0x1UL << FLASH_OPTR_IWDG_STOP_Pos)  /*!< 0x00020000 */
#define FLASH_OPTR_IWDG_STOP              FLASH_OPTR_IWDG_STOP_Msk
#define FLASH_OPTR_IWDG_STDBY_Pos         (18U)
#define FLASH_OPTR_IWDG_STDBY_Msk         (0x1UL << FLASH_OPTR_IWDG_STDBY_Pos) /*!< 0x00040000 */
#define FLASH_OPTR_IWDG_STDBY             FLASH_OPTR_IWDG_STDBY_Msk
#define FLASH_OPTR_WWDG_SW_Pos            (19U)
#define FLASH_OPTR_WWDG_SW_Msk            (0x1UL << FLASH_OPTR_WWDG_SW_Pos)    /*!< 0x00080000 */
#define FLASH_OPTR_WWDG_SW                FLASH_OPTR_WWDG_SW_Msk
#define FLASH_OPTR_BFB2_Pos               (20U)
#define FLASH_OPTR_BFB2_Msk               (0x1UL << FLASH_OPTR_BFB2_Pos)       /*!< 0x00100000 */
#define FLASH_OPTR_BFB2                   FLASH_OPTR_BFB2_Msk
#define FLASH_OPTR_DBANK_Pos              (22U)
#define FLASH_OPTR_DBANK_Msk              (0x1UL << FLASH_OPTR_DBANK_Pos)      /*!< 0x00400000 */
#define FLASH_OPTR_DBANK                  FLASH_OPTR_DBANK_Msk
#define FLASH_OPTR_nBOOT1_Pos             (23U)
#define FLASH_OPTR_nBOOT1_Msk             (0x1UL << FLASH_OPTR_nBOOT1_Pos)     /*!< 0x00800000 */
#define FLASH_OPTR_nBOOT1                 FLASH_OPTR_nBOOT1_Msk
#define FLASH_OPTR_SRAM_PE_Pos            (24U)
#define FLASH_OPTR_SRAM_PE_Msk            (0x1UL << FLASH_OPTR_SRAM_PE_Pos)    /*!< 0x01000000 */
#define FLASH_OPTR_SRAM_PE                FLASH_OPTR_SRAM_PE_Msk
#define FLASH_OPTR_CCMSRAM_RST_Pos        (25U)
#define FLASH_OPTR_CCMSRAM_RST_Msk        (0x1UL << FLASH_OPTR_CCMSRAM_RST_Pos)/*!< 0x02000000 */
#define FLASH_OPTR_CCMSRAM_RST            FLASH_OPTR_CCMSRAM_RST_Msk
#define FLASH_OPTR_nSWBOOT0_Pos           (26U)
#define FLASH_OPTR_nSWBOOT0_Msk           (0x1UL << FLASH_OPTR_nSWBOOT0_Pos)   /*!< 0x04000000 */
#define FLASH_OPTR_nSWBOOT0               FLASH_OPTR_nSWBOOT0_Msk
#define FLASH_OPTR_nBOOT0_Pos             (27U)
#define FLASH_OPTR_nBOOT0_Msk             (0x1UL << FLASH_OPTR_nBOOT0_Pos)     /*!< 0x08000000 */
#define FLASH_OPTR_nBOOT0                 FLASH_OPTR_nBOOT0_Msk
#define FLASH_OPTR_NRST_MODE_Pos          (28U)
#define FLASH_OPTR_NRST_MODE_Msk          (0x3UL << FLASH_OPTR_NRST_MODE_Pos)  /*!< 0x30000000 */
#define FLASH_OPTR_NRST_MODE              FLASH_OPTR_NRST_MODE_Msk
#define FLASH_OPTR_NRST_MODE_0            (0x1UL << FLASH_OPTR_NRST_MODE_Pos)  /*!< 0x10000000 */
#define FLASH_OPTR_NRST_MODE_1            (0x2UL << FLASH_OPTR_NRST_MODE_Pos)  /*!< 0x20000000 */
#define FLASH_OPTR_IRHEN_Pos              (30U)
#define FLASH_OPTR_IRHEN_Msk              (0x1UL << FLASH_OPTR_IRHEN_Pos)      /*!< 0x40000000 */
#define FLASH_OPTR_IRHEN                  FLASH_OPTR_IRHEN_Msk


/**
  * @}
  */

/** @defgroup FLASH_OB_USER_BOR_LEVEL FLASH Option Bytes User BOR Level
  * @{
  */
#define OB_BOR_LEVEL_0            FLASH_OPTR_BOR_LEV_0     /*!< Reset level threshold is around 1.7V */
#define OB_BOR_LEVEL_1            FLASH_OPTR_BOR_LEV_1     /*!< Reset level threshold is around 2.0V */
#define OB_BOR_LEVEL_2            FLASH_OPTR_BOR_LEV_2     /*!< Reset level threshold is around 2.2V */
#define OB_BOR_LEVEL_3            FLASH_OPTR_BOR_LEV_3     /*!< Reset level threshold is around 2.5V */
#define OB_BOR_LEVEL_4            FLASH_OPTR_BOR_LEV_4     /*!< Reset level threshold is around 2.8V */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_STOP FLASH Option Bytes User Reset On Stop
  * @{
  */
#define OB_STOP_RST               0x00000000U              /*!< Reset generated when entering the stop mode */
#define OB_STOP_NORST             FLASH_OPTR_nRST_STOP     /*!< No reset generated when entering the stop mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_STANDBY FLASH Option Bytes User Reset On Standby
  * @{
  */
#define OB_STANDBY_RST            0x00000000U              /*!< Reset generated when entering the standby mode */
#define OB_STANDBY_NORST          FLASH_OPTR_nRST_STDBY    /*!< No reset generated when entering the standby mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nRST_SHUTDOWN FLASH Option Bytes User Reset On Shutdown
  * @{
  */
#define OB_SHUTDOWN_RST           0x00000000U              /*!< Reset generated when entering the shutdown mode */
#define OB_SHUTDOWN_NORST         FLASH_OPTR_nRST_SHDW     /*!< No reset generated when entering the shutdown mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_SW FLASH Option Bytes User IWDG Type
  * @{
  */
#define OB_IWDG_HW                0x00000000U              /*!< Hardware independent watchdog */
#define OB_IWDG_SW                FLASH_OPTR_IWDG_SW       /*!< Software independent watchdog */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_STOP FLASH Option Bytes User IWDG Mode On Stop
  * @{
  */
#define OB_IWDG_STOP_FREEZE       0x00000000U              /*!< Independent watchdog counter is frozen in Stop mode */
#define OB_IWDG_STOP_RUN          FLASH_OPTR_IWDG_STOP     /*!< Independent watchdog counter is running in Stop mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_IWDG_STANDBY FLASH Option Bytes User IWDG Mode On Standby
  * @{
  */
#define OB_IWDG_STDBY_FREEZE      0x00000000U              /*!< Independent watchdog counter is frozen in Standby mode */
#define OB_IWDG_STDBY_RUN         FLASH_OPTR_IWDG_STDBY    /*!< Independent watchdog counter is running in Standby mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_WWDG_SW FLASH Option Bytes User WWDG Type
  * @{
  */
#define OB_WWDG_HW                0x00000000U              /*!< Hardware window watchdog */
#define OB_WWDG_SW                FLASH_OPTR_WWDG_SW       /*!< Software window watchdog */
/**
  * @}
  */

#if defined (FLASH_OPTR_DBANK)
/** @defgroup FLASH_OB_USER_BFB2 FLASH Option Bytes User BFB2 Mode
  * @{
  */
#define OB_BFB2_DISABLE           0x00000000U              /*!< Dual-bank boot disable */
#define OB_BFB2_ENABLE            FLASH_OPTR_BFB2          /*!< Dual-bank boot enable */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_DBANK FLASH Option Bytes User DBANK Type
  * @{
  */
#define OB_DBANK_128_BITS         0x00000000U              /*!< Single-bank with 128-bits data */
#define OB_DBANK_64_BITS          FLASH_OPTR_DBANK         /*!< Dual-bank with 64-bits data */
/**
  * @}
  */
#endif

#if defined (FLASH_OPTR_PB4_PUPEN)
/** @defgroup FLASH_OB_USER_PB4_PUPEN FLASH Option Bytes User PB4 PUPEN bit
  * @{
  */
#define OB_PB4_PUPEN_DISABLE      0x00000000U              /*!< USB power delivery dead-battery enabled/ TDI pull-up deactivated */
#define OB_PB4_PUPEN_ENABLE       FLASH_OPTR_PB4_PUPEN     /*!< USB power delivery dead-battery disabled/ TDI pull-up activated */
/**
  * @}
  */
#endif

/** @defgroup FLASH_OB_USER_nBOOT1 FLASH Option Bytes User BOOT1 Type
  * @{
  */
#define OB_BOOT1_SRAM             0x00000000U              /*!< Embedded SRAM1 is selected as boot space (if BOOT0=1) */
#define OB_BOOT1_SYSTEM           FLASH_OPTR_nBOOT1        /*!< System memory is selected as boot space (if BOOT0=1) */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_SRAM_PE FLASH Option Bytes User SRAM Parity Check Type
  * @{
  */
#define OB_SRAM_PARITY_ENABLE     0x00000000U              /*!< SRAM parity check enable (first 32kB of SRAM1 + CCM SRAM) */
#define OB_SRAM_PARITY_DISABLE    FLASH_OPTR_SRAM_PE       /*!< SRAM parity check disable (first 32kB of SRAM1 + CCM SRAM) */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_CCMSRAM_RST FLASH Option Bytes User CCMSRAM Erase On Reset Type
  * @{
  */
#define OB_CCMSRAM_RST_ERASE      0x00000000U              /*!< CCMSRAM erased when a system reset occurs */
#define OB_CCMSRAM_RST_NOT_ERASE  FLASH_OPTR_CCMSRAM_RST   /*!< CCMSRAM is not erased when a system reset occurs */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nSWBOOT0 FLASH Option Bytes User Software BOOT0
  * @{
  */
#define OB_BOOT0_FROM_OB          0x00000000U              /*!< BOOT0 taken from the option bit nBOOT0 */
#define OB_BOOT0_FROM_PIN         FLASH_OPTR_nSWBOOT0      /*!< BOOT0 taken from PB8/BOOT0 pin */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_nBOOT0 FLASH Option Bytes User nBOOT0 option bit
  * @{
  */
#define OB_nBOOT0_RESET           0x00000000U              /*!< nBOOT0 = 0 */
#define OB_nBOOT0_SET             FLASH_OPTR_nBOOT0        /*!< nBOOT0 = 1 */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_NRST_MODE FLASH Option Bytes User NRST mode bit
  * @{
  */
#define OB_NRST_MODE_INPUT_ONLY   FLASH_OPTR_NRST_MODE_0   /*!< Reset pin is in Reset input mode only */
#define OB_NRST_MODE_GPIO         FLASH_OPTR_NRST_MODE_1   /*!< Reset pin is in GPIO mode only */
#define OB_NRST_MODE_INPUT_OUTPUT FLASH_OPTR_NRST_MODE     /*!< Reset pin is in reset input and output mode */
/**
  * @}
  */

/** @defgroup FLASH_OB_USER_INTERNAL_RESET_HOLDER FLASH Option Bytes User internal reset holder bit
  * @{
  */
#define OB_IRH_DISABLE            0x00000000U              /*!< Internal Reset holder disable */
#define OB_IRH_ENABLE             FLASH_OPTR_IRHEN         /*!< Internal Reset holder enable */
/**
  * @}
  */

/** @defgroup FLASH_OB_PCROP_RDP FLASH Option Bytes PCROP On RDP Level Type
  * @{
  */
#define OB_PCROP_RDP_NOT_ERASE    0x00000000U              /*!< PCROP area is not erased when the RDP level
                                                                is decreased from Level 1 to Level 0 */
#define OB_PCROP_RDP_ERASE        FLASH_PCROP1ER_PCROP_RDP /*!< PCROP area is erased when the RDP level is
                                                                decreased from Level 1 to Level 0 (full mass erase) */
/**
  * @}
  */


#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_STM32_USART_BL_HOST_INCLUDE_STM32G4XX_FLASH_H_ */
