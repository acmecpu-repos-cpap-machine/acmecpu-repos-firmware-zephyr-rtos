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
// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/sensor.h>
// #include <drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(shmax30101);

#include <stdio.h>
// #include "DSInterface.h"
#include "SSMAX30101Comm.h"
// #include "Peripherals.h"
#include "assert.h"
#include "utils.h"
#include "queue.h"
#include "CRC8.h"

#ifdef ENABLE_BLE
#include "BLE_ICARUS.h"
#endif

/* Static variables */
static int sample_count;
static volatile uint8_t data_report_mode;
static const char* sensor_type = "ppg";
// static char charbuf[512];
// static addr_val_pair reg_vals[64];
static bool agc_enabled = true;

#ifdef ENABLE_MAX30101
	static struct queue_t max30101_queue;
	static uint8_t max30101_queue_buf[64 * sizeof(max30101_mode1_data)];
#endif
#ifdef ENABLE_WHRM_ANS_SP02
	static struct queue_t whrm_queue;
	static uint8_t whrm_queue_buf[64 * sizeof(whrm_mode1_data)];
#endif
#ifdef ENABLE_ACCEL
	static struct queue_t accel_queue;
	static uint8_t accel_queue_buf[64 * sizeof(accel_mode1_data)];
#endif
#ifdef ENABLE_BPT
	static struct queue_t bpt_queue;
	static uint8_t bpt_queue_buf[64 * sizeof(bpt_mode1_2_data)];
#endif

#ifdef ENABLE_MAX30101
	static ss_data_req max30101_mode1_data_req;
#endif
#ifdef ENABLE_WHRM_ANS_SP02
	static ss_data_req whrm_mode1_data_req;
#endif
#ifdef ENABLE_ACCEL
	static ss_data_req accel_mode1_data_req;
#endif
	static ss_data_req agc_mode1_data_req;
#ifdef ENABLE_BPT
	static ss_data_req bpt_mode1_2_data_req;
#endif

static struct k_mutex comm_mutex;

static const char* const cmd_tbl[] = {
    "get_format ppg 0",
    "get_format bpt 0",
    "get_format bpt 1",
    "read ppg 0",
    "read bpt 0",
    "read bpt 1",
	"get_reg ppg",
	"set_reg ppg",
	"dump_reg ppg",
	"set_cfg ppg agc 0",
	"set_cfg ppg agc 1",
	"set_cfg bpt med",
	"set_cfg bpt sys_bp",
	"set_cfg bpt dia_bp",
	"set_cfg bpt date",
	"set_cfg bpt nonrest",
	"set_cfg spo2_cal_ab",
	"set_cfg spo2_cal_c",
	"self_test ppg os24",
	"self_test ppg acc",
};

/* static function declarations */
static void max30101_data_rx(uint8_t *data_ptr);
static void bpt_data_rx(uint8_t *data_ptr);
static void whrm_data_rx(uint8_t *data_ptr);
static void accel_data_rx(uint8_t *data_ptr);
static void agc_data_rx(uint8_t *data_ptr);

static int SSMAX30101Comm_parse_cal_str(const char *ptr_ch, const char *cmd, uint8_t *cal_data, int cal_data_sz);

// SSMAX30101Comm::SSMAX30101Comm(USBSerial *USB, SSInterface* ssInterface, DSInterface* dsInterface)
//     :SensorComm("ppg", true), m_USB(USB), ss_int(ssInterface), ds_int(dsInterface), agc_enabled(true)
void SSMAX30101Comm_init()
{
	k_mutex_init(&comm_mutex);

#ifdef ENABLE_MAX30101
	max30101_mode1_data_req.data_size = SSMAX30101_MODE1_DATASIZE;
	max30101_mode1_data_req.callback = max30101_data_rx;
#endif
#ifdef ENABLE_WHRM_ANS_SP02
	whrm_mode1_data_req.data_size = SSWHRM_MODE1_DATASIZE;
	whrm_mode1_data_req.callback = whrm_data_rx;
#endif
#ifdef ENABLE_ACCEL
	accel_mode1_data_req.data_size = SSACCEL_MODE1_DATASIZE;
	accel_mode1_data_req.callback = accel_data_rx;
#endif
	agc_mode1_data_req.data_size = SSAGC_MODE1_DATASIZE;
	agc_mode1_data_req.callback = agc_data_rx;
#ifdef ENABLE_BPT
	bpt_mode1_2_data_req.data_size = SSBPT_MODE1_2_DATASIZE;
	bpt_mode1_2_data_req.callback = bpt_data_rx;
#endif

#ifdef ENABLE_MAX30101
	queue_init(&max30101_queue, max30101_queue_buf, sizeof(max30101_mode1_data), sizeof(max30101_queue_buf));
#endif
#ifdef ENABLE_WHRM_ANS_SP02
	queue_init(&whrm_queue, whrm_queue_buf, sizeof(whrm_mode1_data), sizeof(whrm_queue_buf));
#endif
#ifdef ENABLE_ACCEL
	queue_init(&accel_queue, accel_queue_buf, sizeof(accel_mode1_data), sizeof(accel_queue_buf));
#endif
#ifdef ENABLE_BPT
	queue_init(&bpt_queue, bpt_queue_buf, sizeof(bpt_mode1_2_data), sizeof(bpt_queue_buf));
#endif
}

void SSMAX30101Comm_stop()
{
	// comm_mutex.lock();
	k_mutex_lock(&comm_mutex, K_FOREVER);
	SSInterface_disable_irq();
	data_report_mode = 0;
	sample_count = 0;
#ifdef ENABLE_MAX30101
	SSInterface_disable_sensor(SS_SENSORIDX_MAX30101);
#endif
#ifdef ENABLE_ACCEL
	SSInterface_disable_sensor(SS_SENSORIDX_ACCEL);
#endif
#ifdef ENABLE_WHRM_ANS_SP02
	SSInterface_disable_algo(SS_ALGOIDX_WHRM);
#endif
#ifdef ENABLE_BPT
	SSInterface_disable_algo(SS_ALGOIDX_BPT);
#endif
	SSInterface_ss_clear_interrupt_flag();
	// SSInterface_enable_irq();
	// comm_mutex.unlock();
	k_mutex_unlock(&comm_mutex);
}

bool SSMAX30101Comm_SPO2_calCoef_set(int32_t a, int32_t b, int32_t c, uint8_t algo_idx)
{
	uint8_t cal_coefs[12] = {0x00};
	memcpy(cal_coefs, &a, 4);
	memcpy(cal_coefs+4, &b, 4);
	memcpy(cal_coefs+8, &c, 4);
	SS_STATUS status;
	int cfg_idx;
	if (algo_idx == SS_ALGOIDX_BPT) {
		cfg_idx = SS_CFGIDX_BP_EST_SPO2_CAL;
	} else if (algo_idx == SS_ALGOIDX_WHRM) {
		cfg_idx = 0x0B;
	} else {
		return false;
	}
	status = SSInterface_set_algo_cfg(algo_idx, cfg_idx, &cal_coefs[0], 12);
	if (status == SS_SUCCESS) {
		LOG_INF("\r\n%s err=%d\r\n", "SPO2_calCoef_set", COMM_SUCCESS);
		return true;
	} else {
		LOG_ERR("\r\n%s err=%d\r\n", "SPO2_calCoef_set", COMM_GENERAL_ERROR);
		return false;
	}

	return true;
}

static int SSMAX30101Comm_parse_cal_str(const char *ptr_ch, const char *cmd, uint8_t *cal_data, int cal_data_sz)
{
	char ascii_byte[] = { 0, 0, 0 };
	const char* sptr = ptr_ch + strlen(cmd);
	int found = 0;
	int ssfound;
	unsigned int val32;

	//Eat spaces after cmd
	while (*sptr == ' ') { sptr++; }
	if (*sptr == '\0')
		return -1;
	//sptr++;

	while (found < cal_data_sz) {
		if (*sptr == '\0')
			break;
		ascii_byte[0] = *sptr++;
		ascii_byte[1] = *sptr++;
		ssfound = sscanf(ascii_byte, "%x", &val32);
		if (ssfound != 1)
			break;
		*(cal_data + found) = (uint8_t)val32;
		//LOG_ERR("cal_data[%d]=%d\r\n", found, val32);
		found++;
	}

	//LOG_ERR("total found: %d\r\n", found);
	if (found < cal_data_sz)
		return -1;
	return 0;
}

bool SSMAX30101Comm_parse_command(const char* cmd)
{
	int ret=0;
	char cal_str_to_be_set[650];
    SS_STATUS status;
    bool recognizedCmd = false;

    // if (!ss_int) {
    //     LOG_ERR("No SmartSensor Interface defined!");
    //     return false;
    // }
    // if (!ds_int) {
    //     LOG_ERR("No DeviceStudio Interface defined!");
    //     return false;
    // }

    for (int i = 0; i < NUM_CMDS; i++) {
        if (starts_with(cmd, cmd_tbl[i])) {
            cmd_state_t user_cmd = (cmd_state_t)i;
            recognizedCmd = true;

            switch (user_cmd) {
				case get_format_ppg_0:
				{
#ifdef ASCII_COMM
					LOG_INF("\r\n%s format=smpleCnt,irCnt,redCnt,led3,led4,"
							"accelX,accelY,accelZ,hr,hrconf,spo2,status err=0\r\n", cmd);
#else
					LOG_INF("\r\n%s enc=bin cs=1 format={smpleCnt,32},{irCnt,20},"
							"{redCnt,20},{led3,20},{led4,20},{accelX,14,3},{accelY,14,3},"
							"{accelZ,14,3},{hr,12,1},{hrconf,8},{spo2,11,1},{status,8} err=0\r\n", cmd);
#endif
				} break;

				case get_format_bpt_0:
				{
#ifdef ASCII_COMM
					LOG_INF("\r\n%s format=status,prog,irCnt,hr,sys_bp,dia_bp err=0\r\n", cmd);
#else
					LOG_INF("\r\n%s enc=bin cs=1 format={status,4},{irCnt,19},{hr,9},"
							"{prog,9},{sys_bp,9},{dia_bp,9} err=0\r\n", cmd);
#endif
				} break;

				case get_format_bpt_1:
				{
#ifdef ASCII_COMM
					LOG_INF("\r\n%s format=status,prog,irCnt,hr,sys_bp,dia_bp err=0\r\n", cmd);
#else
					LOG_INF("\r\n%s enc=bin cs=1 format={status,4},{irCnt,19},{hr,9},"
							"{prog,9},{sys_bp,9},{dia_bp,9} err=0\r\n", cmd);
#endif
				} break;

				case read_ppg_0:
				{
					sample_count = 0;

					status = SSInterface_set_data_type(SS_DATATYPE_BOTH, false);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						break;
					}

					status = SSInterface_set_fifo_thresh(15);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						break;
					}

#if 1
					if (agc_enabled) {
						status = SSInterface_enable_algo(SS_ALGOIDX_AGC, 1, &agc_mode1_data_req);
					} else {
						status = SSInterface_disable_algo(SS_ALGOIDX_AGC);
					}
#endif
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d - agc_enabled: %d\n", __LINE__, agc_enabled);
						SSInterface_enable_irq();
						break;
					}

					SSInterface_disable_irq();
#ifdef ENABLE_ACCEL
					status = SSInterface_enable_sensor(SS_SENSORIDX_ACCEL, 1, &accel_mode1_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						SSInterface_enable_irq();
						break;
					}

#endif
#ifdef ENABLE_MAX30101
					status = SSInterface_enable_sensor(SS_SENSORIDX_MAX30101, 1, &max30101_mode1_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						SSInterface_enable_irq();
						break;
					}
#endif
#ifdef ENABLE_WHRM_ANS_SP02
					status = SSInterface_enable_algo(SS_ALGOIDX_WHRM, 1, &whrm_mode1_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						SSInterface_enable_irq();
						break;
					}

#endif
					k_sleep(K_MSEC(100));
					// comm_mutex.lock();
					k_mutex_lock(&comm_mutex, K_FOREVER);
					data_report_mode = read_ppg_0;
					// comm_mutex.unlock();
					k_mutex_unlock(&comm_mutex);
					LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					SSInterface_enable_irq();
					break;
				}

				/* BP Calibration */
				case read_bpt_0:
				{
					sample_count = 0;

					status = SSInterface_set_data_type(SS_DATATYPE_BOTH, false);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						break;
					}

					status = SSInterface_set_fifo_thresh(15);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						break;
					}

#if 0
					if (agc_enabled) {
						status = SSInterface_enable_algo(SS_ALGOIDX_AGC, 1, &agc_mode1_data_req);
					} else {
						status = SSInterface_disable_algo(SS_ALGOIDX_AGC);
					}
#endif
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d - agc_enabled: %d\n", __LINE__, agc_enabled);
						SSInterface_enable_irq();
						break;
					}

					SSInterface_disable_irq();
#ifdef ENABLE_MAX30101
					status = SSInterface_enable_sensor(SS_SENSORIDX_MAX30101, 1, &max30101_mode1_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						SSInterface_enable_irq();
						break;
					}
#endif

#ifdef ENABLE_BPT
					status = SSInterface_enable_algo(SS_ALGOIDX_BPT, 1, &bpt_mode1_2_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						LOG_ERR("FAILED at line %d\n", __LINE__);
						SSInterface_enable_irq();
						break;
					}
#endif
					// comm_mutex.lock();
					k_mutex_lock(&comm_mutex, K_FOREVER);
					data_report_mode = read_bpt_0;
					// comm_mutex.unlock();
					k_mutex_unlock(&comm_mutex);
					LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					SSInterface_enable_irq();
					break;
				}

				/* BP Estimation */
				case read_bpt_1:
				{
					sample_count = 0;

					status = SSInterface_set_data_type(SS_DATATYPE_BOTH, false);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						break;
					}

					status = SSInterface_set_fifo_thresh(15);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						break;
					}

					if (agc_enabled) {
						status = SSInterface_enable_algo(SS_ALGOIDX_AGC, 1, &agc_mode1_data_req);
					} else {
						status = SSInterface_disable_algo(SS_ALGOIDX_AGC);
					}

					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						SSInterface_enable_irq();
						break;
					}

					SSInterface_disable_irq();
#ifdef ENABLE_MAX30101
					status = SSInterface_enable_sensor(SS_SENSORIDX_MAX30101, 1, &max30101_mode1_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						SSInterface_enable_irq();
						break;
					}
#endif

#ifdef ENABLE_BPT
					status = SSInterface_enable_algo(SS_ALGOIDX_BPT, 2, &bpt_mode1_2_data_req);
					if (status != SS_SUCCESS) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, status);
						SSInterface_enable_irq();
						break;
					}
#endif
					// comm_mutex.lock();
					k_mutex_lock(&comm_mutex, K_FOREVER);
					data_report_mode = read_bpt_0;
					// comm_mutex.unlock();
					k_mutex_unlock(&comm_mutex);
					LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					SSInterface_enable_irq();
				} break;

				case get_reg_ppg:
				{
					uint8_t addr;
					uint32_t val;

					ret = parse_get_reg_cmd(cmd, sensor_type, &addr);
					if (!ret) {
						status = SSInterface_get_reg(SS_SENSORIDX_MAX30101, addr, &val);
						if (status == SS_SUCCESS) {
							LOG_INF("\r\n%s reg_val=%02X err=%d\r\n", cmd, (uint8_t)val, COMM_SUCCESS);
						} else {
							LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						}
					} else {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
					}

				} break;

				case set_reg_ppg:
				{
					uint8_t addr;
					uint8_t val;

					ret = parse_set_reg_cmd(cmd, sensor_type, &addr, &val);
					if (!ret) {
						status = SSInterface_set_reg(SS_SENSORIDX_MAX30101, addr, val, SSMAX30101_REG_SIZE);
						if (status == SS_SUCCESS) {
							LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
						} else {
							LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
						}
					} else {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
					}

				} break;

				case dump_reg_ppg:
				{
#if SSCOMM_ENABLE
					int num_regs;
					status = SSInterface_dump_reg(SS_SENSORIDX_MAX30101, &reg_vals[0], ARRAY_SIZE(reg_vals), &num_regs);
					if (status == SS_SUCCESS) {
						int len = 0;
						bool comma = false;
						for (int reg = 0; reg < num_regs; reg++) {
							if (comma) {
								len += snprintf(&charbuf[0] + len, sizeof(charbuf) - len - 1, ",");
							}
							len += snprintf(&charbuf[0] + len, sizeof(charbuf) - len - 1,
											"{%X,%lX}", reg_vals[reg].addr, reg_vals[reg].val);
							comma = true;
						}
						charbuf[len] = '\0';

						LOG_INF("\r\n%s reg_val=%s err=%d\r\n", cmd, &charbuf[0], COMM_SUCCESS);

					} else {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
					}
#endif	/*SSCOMM_ENABLE*/
				} break;

				case set_agc_en:
				{
					agc_enabled = true;
					LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
				} break;
				case set_agc_dis:
				{
					agc_enabled = false;
					LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
				} break;

				case set_cfg_bpt_med:
				{
#if ENABLE_BPT_CFGS
					uint8_t val;
					ret = (parse_cmd_data_8(cmd, cmd_tbl[i], &val, 1, false) != 1);
					if (ret) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
						break;
					}

					status = SSInterface_set_algo_cfg(SS_ALGOIDX_BPT, SS_CFGIDX_BP_USE_MED, &val, 1);
					if (status == SS_SUCCESS)
						LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					else
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
#endif
				} break;

				case set_cfg_bpt_sys_bp:
				{
#if ENABLE_BPT_CFGS
					uint8_t val[3];
					ret = (parse_cmd_data_8(cmd, cmd_tbl[i], &val[0], 3, false) != 3);
					if (ret) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
						break;
					}

					status = SSInterface_set_algo_cfg(SS_ALGOIDX_BPT, SS_CFGIDX_BP_SYS_BP_CAL, &val[0], 3);
					if (status == SS_SUCCESS)
						LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					else
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
#endif
				} break;

				case set_cfg_bpt_dia_bp:
				{
#if ENABLE_BPT_CFGS
					uint8_t val[3];
					ret = (parse_cmd_data_8(cmd, cmd_tbl[i], &val[0], 3, false) != 3);
					if (ret) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
						break;
					}

					status = SSInterface_set_algo_cfg(SS_ALGOIDX_BPT, SS_CFGIDX_BP_DIA_BP_CAL, &val[0], 3);
					if (status == SS_SUCCESS)
						LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					else
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
#endif
				} break;

				case set_cfg_bpt_date:
				{
#if ENABLE_BPT_CFGS
					// Date format is yyyy mm dd
					uint32_t val[3];
					ret = (parse_cmd_data_32(cmd, cmd_tbl[i], &val[0], 3, false) != 3);
					if (ret) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
						break;
					}

					uint8_t date[4] = { (uint8_t)((val[0] >> 8) & 0xFF), (uint8_t)(val[0] & 0xFF), //year_MSB, year_LSB
										(uint8_t)val[1],	//Month
										(uint8_t)val[2] };	//Day

					status = SSInterface_set_algo_cfg(SS_ALGOIDX_BPT, SS_CFGIDX_BP_EST_DATE, &date[0], 4);
					if (status == SS_SUCCESS)
						LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					else
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
#endif
				} break;
				case set_cfg_bpt_nonrest:
				{
#if ENABLE_BPT_CFGS
					uint8_t val;
					ret = (parse_cmd_data_8(cmd, cmd_tbl[i], &val, 1, false) != 1);
					if (ret) {
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_INVALID_PARAM);
						break;
					}

					status = SSInterface_set_algo_cfg(SS_ALGOIDX_BPT, SS_CFGIDX_BP_EST_NONREST, &val, 1);
					if (status == SS_SUCCESS)
						LOG_INF("\r\n%s err=%d\r\n", cmd, COMM_SUCCESS);
					else
						LOG_ERR("\r\n%s err=%d\r\n", cmd, COMM_GENERAL_ERROR);
#endif
				} break;
				
				case set_cfg_spo2_cal_ab:
				{

				} break;
				
				case set_cfg_spo2_cal_c:
				{

				} break;

				case self_test_ppg_os24:
				{
#if SSCOMM_ENABLE
					ret = selftest_max30101();
#endif
					LOG_INF("%s selftest_max30101: err=<%d>\r\n", cmd, ret);
				} break;

				case self_test_ppg_acc:
				{
#if SSCOMM_ENABLE
					ret = selftest_accelerometer();
#endif
					LOG_INF("%s selftest_accelerometer: err=<%d>\r\n", cmd, ret);
				} break;

                default:
                {
                    // assert_msg(false, "Invalid switch case!");
					LOG_WRN("Invalid switch case!");
                } break;

            }
			break;
        }
    }

    return recognizedCmd;
}
#ifdef ENABLE_MAX30101
static void max30101_data_rx(uint8_t* data_ptr)
{
	max30101_mode1_data sample;
	sample.led1 = (data_ptr[0] << 16) | (data_ptr[1] << 8) | data_ptr[2];
	sample.led2 = (data_ptr[3] << 16) | (data_ptr[4] << 8) | data_ptr[5];
	sample.led3 = (data_ptr[6] << 16) | (data_ptr[7] << 8) | data_ptr[8];
	sample.led4 = (data_ptr[9] << 16) | (data_ptr[10] << 8) | data_ptr[11];

	LOG_DBG("led1=%.6X led2=%.6X led3=%.6X led4=%.6X\r\n", sample.led1, sample.led2, sample.led3, sample.led4);

	enqueue(&max30101_queue, &sample);
}
#endif /*ENABLE_MAX30101*/
#ifdef ENABLE_BPT
static void bpt_data_rx(uint8_t* data_ptr)
{
	bpt_mode1_2_data sample;
	sample.status = data_ptr[0];
	sample.prog = data_ptr[1];
	sample.hr = (data_ptr[2] << 8) | data_ptr[3];
	sample.sys_bp = data_ptr[4];
	sample.dia_bp = data_ptr[5];

	LOG_INF("status=%d prog=%d hr=%d sys=%d dia=%d\r\n", sample.status, sample.prog, sample.hr, sample.sys_bp, sample.dia_bp);
	enqueue(&bpt_queue, &sample);
}
#endif	/*ENABLE_BPT*/
#ifdef ENABLE_WHRM_ANS_SP02
static void whrm_data_rx(uint8_t* data_ptr)
{
	//See API doc for data format
	whrm_mode1_data sample;
	sample.hr = (data_ptr[0] << 8) | data_ptr[1];
	sample.hr_conf = data_ptr[2];
	sample.spo2 = (data_ptr[3] << 8) | data_ptr[4];
	sample.status = data_ptr[5];

	LOG_DBG("hr=%.1f conf=%d spo2=%d status=%d", (float)sample.hr / 10.0, sample.hr_conf, sample.spo2, sample.status);
	// LOG_INF("status = %d", sample.status);
	enqueue(&whrm_queue, &sample);
}
#endif	/*ENABLE_WHRM_ANS_SP02*/
#ifdef ENABLE_ACCEL
static void accel_data_rx(uint8_t* data_ptr)
{
	//See API doc for data format
	accel_mode1_data sample;
	sample.x = (data_ptr[0] << 8) | data_ptr[1];
	sample.y = (data_ptr[2] << 8) | data_ptr[3];
	sample.z = (data_ptr[4] << 8) | data_ptr[5];

	enqueue(&accel_queue, &sample);
}
#endif
static void agc_data_rx(uint8_t* data_ptr)
{	
	//NOP: AGC does not collect data
}

int SSMAX30101Comm_data_report_execute(char* buf, int size)
{
	uint8_t tmp_report_mode;
	max30101_mode1_data max30101_sample = { 0 };
	whrm_mode1_data whrm_sample = { 0 };
	accel_mode1_data accel_sample = { 0 };
	bpt_mode1_2_data bpt_sample = { 0 };
	int16_t data_len = 0;

	if (size <= 0)
		return 0;

	// if (!is_enabled())
	// 	return 0;
	if (!data_report_mode)
		return 0;

	SSInterface_ss_execute_once();

	// comm_mutex.lock();
	k_mutex_lock(&comm_mutex, K_FOREVER);
    tmp_report_mode = data_report_mode;
    // comm_mutex.unlock();
	k_mutex_unlock(&comm_mutex);

	switch (tmp_report_mode) {
		case read_ppg_0:
		{
			if (1

#ifdef ENABLE_MAX30101
				&& queue_len(&max30101_queue) > 0
#endif
#ifdef ENABLE_ACCEL
				&& queue_len(&accel_queue) > 0
#endif
#ifdef ENABLE_WHRM_ANS_SP02
				&& queue_len(&whrm_queue) > 0
#endif
				)
			{
#ifdef ENABLE_MAX30101
				dequeue(&max30101_queue, &max30101_sample);
#endif
#ifdef ENABLE_ACCEL
				dequeue(&accel_queue, &accel_sample);
#endif
#ifdef ENABLE_WHRM_ANS_SP02
				dequeue(&whrm_queue, &whrm_sample);
#endif

#ifdef ASCII_COMM
				// data_len = snprintf(buf, size - 1, "%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.1f,%d,%.1f,%d",
				// 		sample_count++,
				// 		max30101_sample.led1,
				// 		max30101_sample.led2,
				// 		max30101_sample.led3,
				// 		max30101_sample.led4,
				// 		accel_sample.x * 0.001,
				// 		accel_sample.y * 0.001,
				// 		accel_sample.z * 0.001,
				// 		whrm_sample.hr * 0.1,
				// 		whrm_sample.hr_conf,
				// 		whrm_sample.spo2 * 0.1,
				// 		whrm_sample.status);
				
				data_len = sprintf(buf, "%d,%d,%d",
						whrm_sample.hr,
						whrm_sample.hr_conf,
						whrm_sample.spo2);
#else
				// assert_msg(((uint32_t)size > sizeof(ds_pkt_data_mode1)), "data_report_execute buffer too small");
				if (((uint32_t)size < sizeof(ds_pkt_data_mode1))) {
					LOG_ERR("data_report_execute buffer too small");
				}
				ds_pkt_data_mode1* pkt = (ds_pkt_data_mode1*)buf;
				pkt->start_byte = DS_BINARY_PACKET_START_BYTE;
				pkt->led1 = max30101_sample.led1;
				pkt->led2 = max30101_sample.led2;
				pkt->led3 = max30101_sample.led3;
				pkt->led4 = max30101_sample.led4;
				pkt->x = accel_sample.x;
				pkt->y = accel_sample.y;
				pkt->z = accel_sample.z;
				pkt->hr = whrm_sample.hr;
				pkt->hr_conf = whrm_sample.hr_conf;
				pkt->spo2 = whrm_sample.spo2;
				pkt->status = whrm_sample.status;
				pkt->crc8 = shm_crc8((uint8_t*)pkt, sizeof(ds_pkt_data_mode1) - sizeof(uint8_t));
				data_len = sizeof(ds_pkt_data_mode1);
#endif
#if 1
				//static uint32_t led4_old_cnt = 0;

				//if (max30101_sample.led4 != (led4_old_cnt + 1)) {
				//	LOG_ERR("\r\n**** JUMP %d to %d\r\n", max30101_sample.led4, led4_old_cnt);
				//}
				//led4_old_cnt = max30101_sample.led4;
#endif

			}
		} break;


		case read_bpt_0:
		case read_bpt_1:
		{
			if (1

#ifdef ENABLE_MAX30101
				&& queue_len(&max30101_queue) > 0
#endif
#ifdef ENABLE_BPT
				&& queue_len(&bpt_queue) > 0
#endif
				)
			{
#ifdef ENABLE_MAX30101
				dequeue(&max30101_queue, &max30101_sample);
#endif
#ifdef ENABLE_BPT
				dequeue(&bpt_queue, &bpt_sample);
#endif

#ifdef ASCII_COMM
				data_len = snprintf(buf, size - 0, "%d,%d,%.1f,%d,%d,%d\r\n",
						bpt_sample.status,
						max30101_sample.led1,
						bpt_sample.hr * 0.1,
						bpt_sample.prog,
						bpt_sample.sys_bp,
						bpt_sample.dia_bp);
#else
				// assert_msg(((uint32_t)size > sizeof(ds_pkt_bpt_data)), "data_report_execute buffer too small");
				if (((uint32_t)size < sizeof(ds_pkt_bpt_data))) {
					LOG_ERR("data_report_execute buffer too small");
				}

				ds_pkt_bpt_data *pkt = (ds_pkt_bpt_data *)buf;
				pkt->start_byte = DS_BINARY_PACKET_START_BYTE;
				pkt->status = bpt_sample.status;
				pkt->irCnt = max30101_sample.led1;
				pkt->hr = bpt_sample.hr / 10;
				pkt->prog = bpt_sample.prog;
				pkt->sys_bp = bpt_sample.sys_bp;
				pkt->dia_bp = bpt_sample.dia_bp;
				pkt->crc8 = shm_crc8((uint8_t*)pkt, sizeof(ds_pkt_bpt_data) - sizeof(uint8_t));
				data_len = sizeof(ds_pkt_bpt_data);
#endif

			}
		} break;

		default:
			return 0;
	}

    if (data_len < 0) {
		LOG_ERR("snprintf console_tx_buf failed");
	} else if (data_len > size) {
		// LOG_ERR("buffer is insufficient to hold data");
	}

	return data_len;
}
#if SSCOMM_ENABLE
// TODO: convert this to PPG sensor test
int SSMAX30101Comm::selftest_max30101(){
	int ret;
	uint8_t test_result;
	bool test_failed = false;
	LOG_INF("starting selftest_max30101\r\n");
	// configure mfio pin for self test
	SSInterface_mfio_selftest();
	ret = SSInterface_self_test(SS_SENSORIDX_MAX30101, &test_result, 500);
	if(ret != SS_SUCCESS){
		LOG_ERR("SSInterface_self_test(SS_SENSORIDX_MAX30101, &test_result) has failed err<-1>\r\n");
		test_failed = true;
	}
	// reset mfio pin to old state
	if(!SSInterface_reset_mfio_irq()){
		LOG_ERR("smart sensor reset_mfio_irq has failed err<-1>\r\n");
		test_failed = true;
	}
	// reset the sensor to turn off the LED
	ret = SSInterface_reset();
	if(test_failed | !self_test_result_evaluate("selftest_max30101", test_result)){
		return -1;
	}else{
		return SS_SUCCESS;
	}
}

int SSMAX30101Comm::selftest_accelerometer(){
	int ret;
	uint8_t test_result;
	bool test_failed = false;
	LOG_INF("starting selftest_accelerometer\r\n");
	ret = SSInterface_self_test(SS_SENSORIDX_ACCEL, &test_result, 1000);
	if(ret != SS_SUCCESS){
		LOG_ERR("SSInterface_self_test(SS_SENSORIDX_ACCEL, &test_result) has failed err<-1>\r\n");
		test_failed = true;
	}
	// reset the sensor to turn off the LED
	ret = SSInterface_reset();
	if(ret != SS_SUCCESS){
		LOG_ERR("smart sensor reset has failed err<-1>\r\n");
		test_failed = true;
	}
	if(test_failed | !self_test_result_evaluate("selftest_accelerometer", test_result)){
		return -1;
	}else{
		return SS_SUCCESS;
	}
}

bool SSMAX30101Comm::self_test_result_evaluate(const char *message, uint8_t result){
	// check i2c response status
	if(result != 0x00){
		LOG_ERR("%s has failed % 02X err<-1>\r\n", message, result);
		if((result & FAILURE_COMM))
			LOG_ERR("%s communication has failed err<-1>\r\n", message);
		if(result & FAILURE_INTERRUPT)
			LOG_ERR("%s interrupt pin check has failed err<-1>\r\n", message);
		return false;
	}
	return true;
}
#endif /*SSCOMM_ENABLE*/
const char* SSMAX30101Comm_get_algo_ver()
{
	return SSInterface_get_ss_algo_version();
}

const char* SSMAX30101Comm_get_part_name()
{ 
	return "max30101";
}