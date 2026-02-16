/*******************************************************************************
 * Copyright (C) 2017 Maxim Integrated Products, Inc., All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Maxim Integrated
 * Products, Inc. shall not be used except as stated in the Maxim Integrated
 * Products, Inc. Branding Policy.
 *
 * The mere transfer of this software does not imply any licenses
 * of trade secrets, proprietary technology, copyrights, patents,
 * trademarks, maskwork rights, or any other form of intellectual
 * property whatsoever. Maxim Integrated Products, Inc. retains all
 * ownership rights.
 *******************************************************************************
 */

#ifndef MODULES_SHMAX30101_SSMAX30101COMM_H_
#define MODULES_SHMAX30101_SSMAX30101COMM_H_

// #include <zephyr.h>
#include <zephyr/kernel.h>

// #include "mbed.h"
// #include "SensorComm.h"
// #include "USBSerial.h"
#include "SSInterface.h"
#include "queue.h"

#define ENABLE_MAX30101
#define ENABLE_ACCEL
#define ENABLE_WHRM_ANS_SP02
// #define ENABLE_BPT
#define ENABLE_BPT_CFGS	0
#define SSCOMM_ENABLE	0
// #define ASCII_COMM

#define COMM_SUCCESS        0
#define COMM_GENERAL_ERROR  -1
#define COMM_INVALID_PARAM  -254
#define COMM_NOT_RECOGNIZED -255

#define DS_BINARY_PACKET_START_BYTE	0xAA

/**
 * @brief	SSMAX30101Comm Command handler class for communication with MAX30101 on SmartSensor board
 * @details
 */
// class SSMAX30101Comm:	public SensorComm
// {
// public:

/* PUBLIC FUNCTION DECLARATIONS */
/**
* @brief	SSMAX30101Comm constructor.
*
*/
// SSMAX30101Comm(USBSerial* USB, SSInterface* ssInterface, DSInterface* dsInterface);
void SSMAX30101Comm_init();

/**
* @brief	Sets the SPO2 calibration coeficients
* @returns true if sensor acted upon the command, false if command was unknown
*/
bool SSMAX30101Comm_SPO2_calCoef_set(int32_t a, int32_t b, int32_t c, uint8_t algo_idx);

/**
* @brief	Parses DeviceStudio-style commands.
* @details  Parses and executes commands. Prints return code to i/o device.
* @returns true if sensor acted upon the command, false if command was unknown
*/
bool SSMAX30101Comm_parse_command(const char* cmd);

/**
 * @brief	 Fill in buffer with sensor data
 *
 * @param[in]	 buf Buffer to fill data into
 * @param[in]	 size Maximum size of buffer
 * @param[out]   Number of bytes written to buffer
 */
int SSMAX30101Comm_data_report_execute(char* buf, int size);

/**
 * @brief	Stop collecting data and disable sensor
 */
void SSMAX30101Comm_stop();

/**
 * @brief Get the maxim part number of the device
 */
const char* SSMAX30101Comm_get_part_name();

/**
 * @brief Get the algorithm version of the device
 */
const char* SSMAX30101Comm_get_algo_ver();

/**
 * @brief  Execute the smart sensor self test routine
 *
 * @return SS_SUCCESS or error code
 */
int SSMAX30101Comm_selftest_max30101();

/**
 * @brief  Execute the accelerometer self test routine
 * @return SS_SUCCESS or error code
 */
int SSMAX30101Comm_selftest_accelerometer();

/**
 * @brief Evaluate the accelerometer self test routine
 *
 * @param message - message to be printed in the failure cases
 * @param value - result of the self test passed as parameter
 * @return true if result is SUCCESSFULL false otherwise
 */
bool SSMAX30101Comm_self_test_result_evaluate(const char *message, uint8_t value);


// private:

/* PRIVATE METHODS */

/* PRIVATE TYPE DEFINITIONS */
typedef enum _cmd_state_t {
	get_format_ppg_0,
	get_format_bpt_0,
	get_format_bpt_1,
	read_ppg_0,
	read_bpt_0,
	read_bpt_1,
	get_reg_ppg,
	set_reg_ppg,
	dump_reg_ppg,
	set_agc_dis,
	set_agc_en,
	set_cfg_bpt_med,
	set_cfg_bpt_sys_bp,
	set_cfg_bpt_dia_bp,
	set_cfg_bpt_date,
	set_cfg_bpt_nonrest,
	set_cfg_spo2_cal_ab,
	set_cfg_spo2_cal_c,
	self_test_ppg_os24,
	self_test_ppg_acc,
	NUM_CMDS,
} cmd_state_t;

typedef struct {
	uint32_t led1;
	uint32_t led2;
	uint32_t led3;
	uint32_t led4;
} max30101_mode1_data;

typedef struct {
	uint16_t hr;
	uint8_t hr_conf;
	uint16_t spo2;
	uint8_t status;
} whrm_mode1_data;

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} accel_mode1_data;

typedef struct {
	uint8_t status;
	uint16_t sys_bp;
	uint16_t dia_bp;
	uint16_t hr;
	uint16_t prog;
} bpt_mode1_2_data;

typedef struct __attribute__((packed)) {
	uint32_t start_byte	:8;

	uint32_t sample_cnt	:32;
	uint32_t led1	:20;
	uint32_t led2	:20;
	uint32_t led3	:20;
	uint32_t led4	:20;
	uint32_t x	:14;	//Represent values of 0.000 through 8.191
	uint32_t y	:14;	//Represent values of 0.000 through 8.191
	uint32_t z	:14;	//Represent values of 0.000 through 8.191
	uint32_t hr	:12;	//Represent values of 0.0 through 204.7
	uint32_t hr_conf :8;	//Represent values of 0 through 255
	uint32_t spo2	:11;	//Represent values of 0.0 through 102.3 (only need up to 100.0)
	uint32_t status	:8;

	uint8_t	:0;			//Align CRC byte on byte boundary
	uint8_t crc8:8;
} ds_pkt_data_mode1;

typedef struct __attribute__((packed)) {
	uint32_t start_byte:8;

	uint32_t status:4;
	uint32_t irCnt:19;
	uint32_t hr:9;
	uint32_t prog:9;
	uint32_t sys_bp:9;
	uint32_t dia_bp:9;

	uint8_t :0; //Align to next byte
	uint8_t crc8:8;
} ds_pkt_bpt_data;

/* PRIVATE VARIABLES */
// USBSerial *m_USB;
// SSInterface *ss_int;
// DSInterface *ds_int;

/* PRIVATE CONST VARIABLES */
static const int SSMAX30101_REG_SIZE = 1;
static const int SSMAX30101_MODE1_DATASIZE = 12;	//Taken from API doc
static const int SSWHRM_MODE1_DATASIZE = 6;			//Taken from API doc
static const int SSACCEL_MODE1_DATASIZE = 6;		//Taken from API doc
static const int SSAGC_MODE1_DATASIZE = 0;			//Taken from API doc
static const int SSBPT_MODE1_2_DATASIZE = 6;		//Taken from API doc
// };

#endif /* MODULES_SHMAX30101_SSMAX30101COMM_H_ */
