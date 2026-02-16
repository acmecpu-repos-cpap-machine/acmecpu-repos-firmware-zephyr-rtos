/*
 * Copyright (c) 2021 Acme CPU
 */

#ifndef SRC_INCLUDE_APP_FIRMWARE_UPDATE_BSP_FIRMWARE_UPDATE_H_
#define SRC_INCLUDE_APP_FIRMWARE_UPDATE_BSP_FIRMWARE_UPDATE_H_

/* The STM32G4 can be put into the bootloader mode by applying
 * any one of the patterns as stated in Pattern 14 of
 * Bootloader Activation Patterns of AN2606, Table 2
 *
 * Since we have to put the chip into bootloader mode implicitly,
 * we will apply the 2nd Pattern and perform a reboot
 * BOOT_LOCK(bit) = 0, nBoot1(bit) = 1, nBoot0(bit) = 0 and nSWBoot0(bit) = 0
 *
 * These bits are present in the Option Bytes section of the Flash Memory
 * as defined in RM0440 Section 3.4
 * */
int bsp_fwupdate_config_bootloader();

#endif /* SRC_INCLUDE_APP_FIRMWARE_UPDATE_BSP_FIRMWARE_UPDATE_H_ */
