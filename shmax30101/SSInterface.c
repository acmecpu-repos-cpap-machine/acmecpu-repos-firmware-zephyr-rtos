/***************************************************************************
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
****************************************************************************
*/

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <errno.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(shmax30101);
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "SSInterface.h"
// #include "Peripherals.h"
// #include "assert.h"
// #include "utils.h"
// #include "i2cm.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////

/* PRIVATE VARIABLES */
static char fw_version[128];
static char algo_version[128];
static const char* plat_name;

static bool in_bootldr;
static bool m_sc_en;
static int m_data_type;

static int sensor_enabled_mode[SS_MAX_SUPPORTED_SENSOR_NUM];
static int algo_enabled_mode[SS_MAX_SUPPORTED_ALGO_NUM];
static ss_data_req* sensor_data_reqs[SS_MAX_SUPPORTED_SENSOR_NUM];
static ss_data_req* algo_data_reqs[SS_MAX_SUPPORTED_ALGO_NUM];

static volatile bool mfio_int_happened;
static volatile bool m_irq_received_;

static const struct i2c_dt_spec *i2cBus;
static const struct gpio_dt_spec *reset_pin;
static const struct gpio_dt_spec *irq_pin;
static const struct gpio_dt_spec *mfio_pin;
static struct gpio_callback m_irq_cb;

/* PRIVATE METHODS */
// SS_STATUS write_cmd_small(uint8_t *cmd_bytes, int cmd_bytes_len,
// 						uint8_t *data, int data_len,
// 						int sleep_ms = SS_DEFAULT_CMD_SLEEP_MS);
static SS_STATUS write_cmd_small(uint8_t *cmd_bytes, int cmd_bytes_len,
						uint8_t *data, int data_len,
						int sleep_ms);
// SS_STATUS write_cmd_medium(uint8_t *cmd_bytes, int cmd_bytes_len,
// 						uint8_t *data, int data_len,
// 						int sleep_ms = SS_DEFAULT_CMD_SLEEP_MS);
static SS_STATUS write_cmd_medium(uint8_t *cmd_bytes, int cmd_bytes_len,
						uint8_t *data, int data_len,
						int sleep_ms);
// SS_STATUS write_cmd_large(uint8_t *cmd_bytes, int cmd_bytes_len,
// 						uint8_t *data, int data_len,
// 						int sleep_ms = SS_DEFAULT_CMD_SLEEP_MS);
static SS_STATUS write_cmd_large(uint8_t *cmd_bytes, int cmd_bytes_len,
						uint8_t *data, int data_len,
						int sleep_ms);
// void irq_handler();
static void irq_handler(const struct device *dev, 
						struct gpio_callback *cb,
						gpio_port_pins_t pins);

static SS_STATUS read_fifo_data(int num_samples, int sample_size, uint8_t* databuf, int databuf_sz);
static SS_STATUS num_avail_samples(int* num_samples);
static void fifo_sample_size(int data_type, int* sample_size);

static void SSInterface_cfg_mfio(uint32_t PinDirection);
#if REMOVE
static void SSInterface_irq_handler_selftest();
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////

// SSInterface_SSInterface(I2C &i2cBus, PinName ss_mfio, PinName ss_reset)
// 	:m_i2cBus(&i2cBus), m_spiBus(NULL),
// 	mfio_pin(ss_mfio), reset_pin(ss_reset), irq_pin(ss_mfio)/*,
// 	irq_evt(1000000, "irq")*/
int SSInterface_init(const struct i2c_dt_spec *i2c_master, 
						const struct gpio_dt_spec *rst_gpio, 
						const struct gpio_dt_spec *mfio_gpio)
{
	// reset_pin.input();
	// irq_pin.fall(callback(this, &irq_handler));

	int ret = 0;
	i2cBus = i2c_master;
	reset_pin = rst_gpio;
	irq_pin = mfio_gpio;
	mfio_pin = mfio_gpio;

	/* Configure reset pin */
	if (device_is_ready(reset_pin->port)) {
		ret = gpio_pin_configure(reset_pin->port, reset_pin->pin, 
									(GPIO_OUTPUT | GPIO_PUSH_PULL | reset_pin->dt_flags));
        /* reset the sensor hub */
		ret = gpio_pin_set(reset_pin->port, reset_pin->pin, 1); // active low, 1 = low
        k_sleep(K_MSEC(100));
		ret = gpio_pin_set(reset_pin->port, reset_pin->pin, 0); // active low, 0 = high
		k_sleep(K_MSEC(100));
		if (ret != 0) {
			LOG_ERR("Failed to set reset pin %d (%d)", reset_pin->pin, ret);
			// return ret;
		}
	}

	/* Configure irq pin */
	if (device_is_ready(irq_pin->port)) {
		ret = gpio_pin_configure(irq_pin->port, irq_pin->pin,
								(GPIO_INPUT | GPIO_PULL_UP | irq_pin->dt_flags));
		ret = gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
		if (ret != 0) {
			LOG_ERR("Failed to configure interrupt pin %d (%d)", irq_pin->pin, ret);
			return ret;
		}
		/* Prepare GPIO callback for interrupt pin */
		gpio_init_callback(&m_irq_cb, irq_handler, BIT(irq_pin->pin));
		gpio_add_callback(irq_pin->port, &m_irq_cb);
	}

	ret = SSInterface_reset_to_main_app();
	// if (ret != 0) {
	// 	LOG_ERR("SSInterface_reset_to_main_app failed (%d)", ret);
	// 	return ret;
	// }
	k_sleep(K_MSEC(1000));
	ret = SSInterface_get_data_type(&m_data_type, &m_sc_en);
	if (ret != 0) {
		LOG_ERR("get_data_type failed (%d)", ret);
		return ret;
	}

	return ret;
}

#if REMOVE
// SSInterface_SSInterface(SPI &spiBus, PinName ss_mfio, PinName ss_reset)
// 	:m_i2cBus(NULL), m_spiBus(&spiBus),
// 	mfio_pin(ss_mfio), reset_pin(ss_reset), irq_pin(ss_mfio)/*,
// 	irq_evt(1000000, "irq")*/
// {
// 	reset_pin.input();
// 	irq_pin.fall(callback(this, &irq_handler));

// 	SSInterface_reset_to_main_app();
// 	get_data_type(&data_type, &sc_en);
// }

// SSInterface_~SSInterface()
// {
// }
#endif /*REMOVE*/

SS_STATUS SSInterface_reset_to_main_app()
{
	// irq_pin.disable_irq();
	gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_DISABLE);
#if defined(BOOTLOADER_USES_MFIO)
	reset_pin.output();
	cfg_mfio(PIN_OUTPUT);
	reset_pin.write(0);
	wait_ms(SS_RESET_TIME);
	mfio_pin.write(1);
	reset_pin.write(1);
	wait_ms(SS_STARTUP_TO_MAIN_APP_TIME);
	cfg_mfio(PIN_INPUT);
	reset_pin.input();
	irq_pin.enable_irq();
	// Verify we exited bootloader mode
	if (in_bootldr_mode() == 0)
		return SS_SUCCESS;
	else
		return SS_ERR_UNKNOWN;
#else
	SS_STATUS status = SSInterface_exit_from_bootloader();
	// irq_pin.enable_irq();
	gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
	return status;
#endif
}

SS_STATUS SSInterface_reset_to_bootloader()
{
	// irq_pin.disable_irq();
	gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_DISABLE);
#if defined(BOOTLOADER_USES_MFIO)
	reset_pin.output();
	cfg_mfio(PIN_OUTPUT);
	reset_pin.write(0);
	wait_ms(SS_RESET_TIME);
	mfio_pin.write(0);
	reset_pin.write(1);
	wait_ms(SS_STARTUP_TO_BTLDR_TIME);
	cfg_mfio(PIN_INPUT);
	reset_pin.input();
	irq_pin.enable_irq();
	stay_in_bootloader();

	// Verify we entered bootloader mode
	if (in_bootldr_mode() < 0)
		return SS_ERR_UNKNOWN;
	return SS_SUCCESS;
#else
	stay_in_bootloader();
	// irq_pin.enable_irq();
	gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
	return SS_SUCCESS;
#endif
}

SS_STATUS SSInterface_exit_from_bootloader()
{
	uint8_t cmd_bytes[] = { SS_FAM_W_MODE, SS_CMDIDX_MODE };
	uint8_t data[] = { 0x00 };

	SS_STATUS status = write_cmd(
			&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
			&data[0], ARRAY_SIZE(data), SS_DEFAULT_CMD_SLEEP_MS);

	in_bootldr = (status == SS_SUCCESS) ? true : false;
	return status;
}

SS_STATUS stay_in_bootloader()
{
	uint8_t cmd_bytes[] = { SS_FAM_W_MODE, SS_CMDIDX_MODE };
	uint8_t data[] = { SS_MASK_MODE_BOOTLDR };

	SS_STATUS status = write_cmd(
			&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
			&data[0], ARRAY_SIZE(data), SS_DEFAULT_CMD_SLEEP_MS);

	in_bootldr = (status == SS_SUCCESS) ? true : false;
	return status;
}

SS_STATUS SSInterface_reset()
{
	int bootldr = in_bootldr_mode();
	if (bootldr > 0)
		return SSInterface_reset_to_bootloader();
	else if (bootldr == 0)
		return SSInterface_reset_to_main_app();
	else
		return SS_ERR_UNKNOWN;
}

SS_STATUS SSInterface_self_test(int idx, uint8_t *result, int sleep_ms)
{
    uint8_t cmd_bytes[] = { SS_FAM_R_SELFTEST, (uint8_t)idx };
    uint8_t rxbuf[2];
    SS_STATUS ret;

	result[0] = 0xFF;
	ret = read_cmd(cmd_bytes, 2, (uint8_t *)0, 0, rxbuf, ARRAY_SIZE(rxbuf), sleep_ms);
	result[0] = rxbuf[1];
	return ret;
}

static void SSInterface_cfg_mfio(uint32_t dir)
{
	if (dir == GPIO_INPUT) {
		gpio_pin_configure(mfio_pin->port, mfio_pin->pin, 
								(GPIO_INPUT | GPIO_PULL_UP | reset_pin->dt_flags));
		// mfio_pin.input();
		// mfio_pin.mode(PullUp);
	} else {
		gpio_pin_configure(mfio_pin->port, mfio_pin->pin, 
								(GPIO_OUTPUT | GPIO_PUSH_PULL | reset_pin->dt_flags));
		// mfio_pin.output();
	}
}

void SSInterface_enable_irq()
{
	// irq_pin.enable_irq();
	int ret = gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("could not enable IRQ");
	}
}

void SSInterface_disable_irq()
{
	// irq_pin.disable_irq();
	int ret = gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("could not disable IRQ");
	}
}

void SSInterface_mfio_selftest()
{
	SSInterface_disable_irq();
	// irq_pin.fall(callback(this, &SSInterface_irq_handler_selftest));
	SSInterface_enable_irq();
}

int in_bootldr_mode()
{
	uint8_t cmd_bytes[] = { SS_FAM_R_MODE, SS_CMDIDX_MODE };
	uint8_t rxbuf[2] = { 0 };

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
			0, 0,
			&rxbuf[0], ARRAY_SIZE(rxbuf), SS_DEFAULT_CMD_SLEEP_MS);
	if (status != SS_SUCCESS)
		return -1;

	return (rxbuf[1] & SS_MASK_MODE_BOOTLDR);
}

const char* SSInterface_get_ss_fw_version()
{
    uint8_t cmd_bytes[2];
    uint8_t rxbuf[4];

	int bootldr = in_bootldr_mode();

	if (bootldr > 0) {
		cmd_bytes[0] = SS_FAM_R_BOOTLOADER;
		cmd_bytes[1] = SS_CMDIDX_BOOTFWVERSION;
	} else if (bootldr == 0) {
		cmd_bytes[0] = SS_FAM_R_IDENTITY;
		cmd_bytes[1] = SS_CMDIDX_FWVERSION;
	} else {
		return plat_name;
	}

    SS_STATUS status = read_cmd(
             &cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
             0, 0,
             &rxbuf[0], ARRAY_SIZE(rxbuf),
			 SS_DEFAULT_CMD_SLEEP_MS);

    if (status == SS_SUCCESS) {
        snprintf(fw_version, sizeof(fw_version),
            "%d.%d.%d", rxbuf[1], rxbuf[2], rxbuf[3]);
		LOG_INF("fw_version:%s\r\n", fw_version);
    }

    return &fw_version[0];
}

const char* SSInterface_get_ss_algo_version()
{
    uint8_t cmd_bytes[3];
    uint8_t rxbuf[4];

	int bootldr = in_bootldr_mode();

	if (bootldr > 0) {
		cmd_bytes[0] = SS_FAM_R_BOOTLOADER;
		cmd_bytes[1] = SS_CMDIDX_BOOTFWVERSION;
		cmd_bytes[2] = 0;
	} else if (bootldr == 0) {
		cmd_bytes[0] = SS_FAM_R_IDENTITY;
		cmd_bytes[1] = SS_CMDIDX_ALGOVER;
		cmd_bytes[2] = SS_CMDIDX_AVAILSENSORS;
	} else {
		return plat_name;
	}

    SS_STATUS status = read_cmd(
             &cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
             0, 0,
             &rxbuf[0], ARRAY_SIZE(rxbuf),
			 SS_DEFAULT_CMD_SLEEP_MS);

    if (status == SS_SUCCESS) {
        snprintf(algo_version, sizeof(algo_version),
            "%d.%d.%d", rxbuf[1], rxbuf[2], rxbuf[3]);
		LOG_INF("algo_version:%s\r\n", fw_version);
    }

    return &algo_version[0];
}

const char* SSInterface_get_ss_platform_name()
{
    uint8_t cmd_bytes[] = { SS_FAM_R_IDENTITY, SS_CMDIDX_PLATTYPE };
    uint8_t rxbuf[2];

    SS_STATUS status = read_cmd(
            &cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
            0, 0,
            &rxbuf[0], ARRAY_SIZE(rxbuf),
			SS_DEFAULT_CMD_SLEEP_MS);

    if (status == SS_SUCCESS) {
        if (rxbuf[1] == SS_PLAT_MAX3263X) {
			if (in_bootldr_mode() > 0) {
				plat_name = SS_BOOTLOADER_PLATFORM_MAX3263X;
			} else {
	            plat_name = SS_PLATFORM_MAX3263X;
			}
        } else if (rxbuf[1] == SS_PLAT_MAX32660) {
			if (in_bootldr_mode() > 0) {
				plat_name = SS_BOOTLOADER_PLATFORM_MAX32660;
			} else {
            	plat_name = SS_PLATFORM_MAX32660;
			}
        }
    }

    return plat_name;
}

SS_STATUS write_cmd(uint8_t *cmd_bytes, int cmd_bytes_len,
	uint8_t *data, int data_len,
    int sleep_ms)
{
    int total_len = data_len + cmd_bytes_len;

    if (total_len <= SS_SMALL_BUF_SIZE) {
        return write_cmd_small(cmd_bytes, cmd_bytes_len, data, data_len, sleep_ms);
    } else if (total_len <= SS_MED_BUF_SIZE) {
        return write_cmd_medium(cmd_bytes, cmd_bytes_len, data, data_len, sleep_ms);
    } else if (total_len <= SS_LARGE_BUF_SIZE) {
        return write_cmd_large(cmd_bytes, cmd_bytes_len, data, data_len, sleep_ms);
    } else {
        // assert_msg(true, "Tried to send I2C tx larger than maximum allowed size\n");
		LOG_ERR("Tried to send I2C tx larger than maximum allowed size\n");
        return SS_ERR_DATA_FORMAT; 
    }
}

SS_STATUS write_cmd_final(uint8_t *tx_buf, int tx_len, int sleep_ms)
{
	// LOG_INF("write_cmd: ");
	// for (int i = 0; i < tx_len; i++) {
	// 	LOG_INF("0x%02X ", tx_buf[i]);
	// }
	// LOG_INF("\r\n");
	LOG_HEXDUMP_DBG(tx_buf, tx_len, "write_cmd: ");

    // int ret = m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)tx_buf, tx_len);
	int ret = i2c_write_dt(i2cBus, tx_buf, tx_len);

	int retries = 4;
	while (ret != 0 && retries-- > 0) {
		LOG_ERR("i2c wr retry\r\n");
		k_sleep(K_MSEC(1));
    	// ret = m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)tx_buf, tx_len);
		ret = i2c_write_dt(i2cBus, tx_buf, tx_len);
	}

    if (ret != 0) {
    	LOG_ERR("m_i2cBus->write returned %d\r\n", ret);
        return SS_ERR_UNAVAILABLE;
    }

    // wait_ms(sleep_ms);
	k_sleep(K_MSEC(sleep_ms));

    char status_byte;
    // ret = m_i2cBus->read(SS_I2C_8BIT_SLAVE_ADDR, &status_byte, 1);
	ret = i2c_read_dt(i2cBus, &status_byte, 1);
	bool try_again = (status_byte == SS_ERR_TRY_AGAIN);
	while ((ret != 0 || try_again) 
			&& retries-- > 0) {
		LOG_INF("i2c rd retry\r\n");
		k_sleep(K_MSEC(sleep_ms));
    	// ret = m_i2cBus->read(SS_I2C_8BIT_SLAVE_ADDR, &status_byte, 1);
		ret = i2c_read_dt(i2cBus, &status_byte, 1);
		try_again = (status_byte == SS_ERR_TRY_AGAIN);
	}

    if (ret != 0 || try_again) {
    	LOG_ERR("m_i2cBus->read returned %d, ss status_byte %d\r\n", ret, status_byte);
        return SS_ERR_UNAVAILABLE;
    }

	LOG_DBG("status_byte: %d\r\n", status_byte);

	return (SS_STATUS)status_byte;
}

static SS_STATUS write_cmd_small(uint8_t *cmd_bytes, int cmd_bytes_len,
                       uint8_t *data, int data_len,
                       int sleep_ms)
{
    uint8_t write_buf[SS_SMALL_BUF_SIZE];
    memcpy(write_buf, cmd_bytes, cmd_bytes_len);
    memcpy(write_buf + cmd_bytes_len, data, data_len);

	SS_STATUS status = write_cmd_final(write_buf, cmd_bytes_len + data_len, sleep_ms);
	return status;
}

static SS_STATUS write_cmd_medium(uint8_t *cmd_bytes, int cmd_bytes_len,
                       uint8_t *data, int data_len,
                       int sleep_ms)
{
    uint8_t write_buf[SS_MED_BUF_SIZE];
    memcpy(write_buf, cmd_bytes, cmd_bytes_len);
    memcpy(write_buf + cmd_bytes_len, data, data_len);

	SS_STATUS status = write_cmd_final(write_buf, cmd_bytes_len + data_len, sleep_ms);
	return status;
}

static SS_STATUS write_cmd_large(uint8_t *cmd_bytes, int cmd_bytes_len,
                       uint8_t *data, int data_len,
                       int sleep_ms)
{
    uint8_t write_buf[SS_LARGE_BUF_SIZE];
    memcpy(write_buf, cmd_bytes, cmd_bytes_len);
    memcpy(write_buf + cmd_bytes_len, data, data_len);

	SS_STATUS status = write_cmd_final(write_buf, cmd_bytes_len + data_len, sleep_ms);
	return status;
}

SS_STATUS read_cmd(uint8_t *cmd_bytes, int cmd_bytes_len,
	uint8_t *data, int data_len,
	uint8_t *rxbuf, int rxbuf_sz,
    int sleep_ms)
{
	// LOG_INF("read_cmd: ");
	// for (int i = 0; i < cmd_bytes_len; i++) {
	// 	LOG_INF("0x%02X ", cmd_bytes[i]);
	// }
	// LOG_INF("\r\n");
	LOG_HEXDUMP_DBG(cmd_bytes, cmd_bytes_len, "read_cmd: ");

	int retries = 4;

    // int ret = m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)cmd_bytes, cmd_bytes_len, (data_len != 0));
	int ret = i2c_write_dt(i2cBus, cmd_bytes, cmd_bytes_len);
    if (data_len != 0) {
        // ret |= m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)data, data_len, false);
		ret |= i2c_write_dt(i2cBus, data, data_len);
    }

	while (ret != 0 && retries-- > 0) {
		LOG_ERR("i2c wr retry\r\n");
		k_sleep(K_MSEC(1));
    	// ret = m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)cmd_bytes, cmd_bytes_len, (data_len != 0));
		ret = i2c_write_dt(i2cBus, cmd_bytes, cmd_bytes_len);
	    if (data_len != 0) {
	        // ret |= m_i2cBus->write(SS_I2C_8BIT_SLAVE_ADDR, (char*)data, data_len, false);
			ret |= i2c_write_dt(i2cBus, data, data_len);
	    }
	}

    if (ret != 0) {
    	LOG_ERR("m_i2cBus->write returned %d\r\n", ret);
        return SS_ERR_UNAVAILABLE;
    }

    // wait_ms(sleep_ms);
	k_sleep(K_MSEC(sleep_ms));

    // ret = m_i2cBus->read(SS_I2C_8BIT_SLAVE_ADDR, (char*)rxbuf, rxbuf_sz);
	ret = i2c_read_dt(i2cBus, rxbuf, rxbuf_sz);
	bool try_again = (rxbuf[0] == SS_ERR_TRY_AGAIN);
	while ((ret != 0 || try_again) && retries-- > 0) {
		LOG_INF("i2c rd retry\r\n");
		// wait_ms(sleep_ms);
		k_sleep(K_MSEC(sleep_ms));
    	// ret = m_i2cBus->read(SS_I2C_8BIT_SLAVE_ADDR, (char*)rxbuf, rxbuf_sz);
		ret = i2c_read_dt(i2cBus, rxbuf, rxbuf_sz);
		try_again = (rxbuf[0] == SS_ERR_TRY_AGAIN);
	}
    if (ret != 0 || try_again) {
    	LOG_ERR("m_i2cBus->read returned %d, ss status_byte %d\r\n", ret, rxbuf[0]);
        return SS_ERR_UNAVAILABLE;
    }

	LOG_DBG("status_byte: %d\r\n", rxbuf[0]);
	// LOG_INF("data: ");
	// for (int i = 1; i < rxbuf_sz; i++) {
	// 	LOG_INF("0x%02X ", rxbuf[i]);
	// }
	// LOG_INF("\r\n");
	LOG_HEXDUMP_DBG(rxbuf, rxbuf_sz, "data: ");

    return (SS_STATUS)rxbuf[0];
}

SS_STATUS SSInterface_get_reg(int idx, uint8_t addr, uint32_t *val)
{
	// assert_msg((idx <= SS_MAX_SUPPORTED_SENSOR_NUM), "idx must be < SS_MAX_SUPPORTED_SENSOR_NUM, or update code to handle variable length idx values");

	uint8_t cmd_bytes[] = { SS_FAM_R_REGATTRIBS, (uint8_t)idx };
	uint8_t rx_reg_attribs[3] = {0};

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rx_reg_attribs[0], ARRAY_SIZE(rx_reg_attribs),
								SS_DEFAULT_CMD_SLEEP_MS);

	if (status != SS_SUCCESS)
		return status;

	int reg_width = rx_reg_attribs[1];

	uint8_t cmd_bytes2[] = { SS_FAM_R_READREG, (uint8_t)idx, addr };
	uint8_t rxbuf[5] = {0};

	status = read_cmd(&cmd_bytes2[0], ARRAY_SIZE(cmd_bytes2),
						0, 0,
						&rxbuf[0], reg_width + 1,
						SS_DEFAULT_CMD_SLEEP_MS);

	if (status == SS_SUCCESS) {
		*val = 0;
		for (int i = 0; i < reg_width; i++) {
			*val = (*val << 8) | rxbuf[i + 1];
		}
	}

	return status;
}

SS_STATUS SSInterface_set_reg(int idx, uint8_t addr, uint32_t val, int byte_size)
{
	// assert_msg((idx <= SS_MAX_SUPPORTED_SENSOR_NUM), "idx must be < SS_MAX_SUPPORTED_SENSOR_NUM, or update code to handle variable length idx values");

	uint8_t cmd_bytes[] = { SS_FAM_W_WRITEREG, (uint8_t)idx, addr };
	uint8_t data_bytes[4];
	for (int i = 0; i < byte_size; i++) {
		data_bytes[i] = (val >> (8 * (byte_size - 1)) & 0xFF);
	}

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								&data_bytes[0], byte_size,
								SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

#if REMOVE
// SS_STATUS SSInterface_dump_reg(int idx, addr_val_pair* reg_vals, int reg_vals_sz, int* num_regs)
// {
// 	//assert_msg((idx <= SS_MAX_SUPPORTED_SENSOR_NUM), "idx must be < SS_MAX_SUPPORTED_SENSOR_NUM, or update code to handle variable length idx values");

// 	uint8_t cmd_bytes[] = { SS_FAM_R_REGATTRIBS, (uint8_t)idx };
// 	uint8_t rx_reg_attribs[3] = {0};

// 	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
// 								0, 0,
// 								&rx_reg_attribs[0], ARRAY_SIZE(rx_reg_attribs),
// 								SS_DEFAULT_CMD_SLEEP_MS);

// 	if (status != SS_SUCCESS)
// 		return status;

// 	int reg_width = rx_reg_attribs[1];
// 	*num_regs = rx_reg_attribs[2];
// 	//assert_msg((*num_regs <= reg_vals_sz), "Need to increase reg_vals array to hold all dump_reg data");
// 	//assert_msg(((size_t)reg_width <= sizeof(uint32_t)), "IC returned register values greater than 4 bytes in width");

// 	int dump_reg_sz = (*num_regs) * (reg_width + 1) + 1; //+1 to reg_width for address, +1 for status byte

// 	uint8_t rxbuf[512];
// 	//assert_msg(((size_t)dump_reg_sz <= sizeof(rxbuf)), "Need to increase buffer size to receive dump_reg data");

// 	cmd_bytes[0] = SS_FAM_R_DUMPREG;
// 	status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
// 								0, 0,
// 								&rxbuf[0], dump_reg_sz, SS_DUMP_REG_SLEEP_MS);

// 	if (status != SS_SUCCESS)
// 		return status;

// 	//rxbuf format is [status][addr0](reg_width x [val0])[addr1](reg_width x [val1])...
// 	for (int reg = 0; reg < *num_regs; reg++) {
// 		reg_vals[reg].addr = rxbuf[(reg * (reg_width + 1)) + 1];
// 		uint32_t *val = &(reg_vals[reg].val);
// 		*val = 0;
// 		for (int byte = 0; byte < reg_width; byte++) {
// 			*val = (*val << 8) | rxbuf[(reg * (reg_width + 1)) + byte + 2];
// 		}
// 	}

// 	return SS_SUCCESS;
// }
#endif /*REMOVE*/

SS_STATUS SSInterface_enable_sensor(int idx, int mode, ss_data_req *data_req)
{
	// assert_msg((idx <= SS_MAX_SUPPORTED_SENSOR_NUM), "idx must be < SS_MAX_SUPPORTED_SENSOR_NUM, or update code to handle variable length idx values");
	// assert_msg((mode <= SS_MAX_SUPPORTED_MODE_NUM), "mode must be < SS_MAX_SUPPORTED_MODE_NUM, or update code to handle variable length mode values");
	// assert_msg((mode != 0), "Tried to enable sensor to mode 0, but mode 0 is disable");


	uint8_t cmd_bytes[] = { SS_FAM_W_SENSORMODE, (uint8_t)idx, (uint8_t)mode };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes), 0, 0, SS_ENABLE_SENSOR_SLEEP_MS);

	if (status == SS_SUCCESS) {
		sensor_enabled_mode[idx] = mode;
		sensor_data_reqs[idx] = data_req;
	}
	return status;
}

SS_STATUS SSInterface_disable_sensor(int idx)
{
	//assert_msg((idx <= SS_MAX_SUPPORTED_SENSOR_NUM), "idx must be < SS_MAX_SUPPORTED_SENSOR_NUM, or update code to handle variable length idx values");
	uint8_t cmd_bytes[] = { SS_FAM_W_SENSORMODE, (uint8_t)idx, 0 };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes), 0, 0, SS_ENABLE_SENSOR_SLEEP_MS);

	if (status == SS_SUCCESS) {
		sensor_enabled_mode[idx] = 0;
		sensor_data_reqs[idx] = 0;
	}

	return status;
}

SS_STATUS SSInterface_enable_algo(int idx, int mode, ss_data_req *data_req)
{
	// assert_msg((idx <= SS_MAX_SUPPORTED_ALGO_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_NUM, or update code to handle variable length idx values");
	// assert_msg((mode <= SS_MAX_SUPPORTED_MODE_NUM), "mode must be < SS_MAX_SUPPORTED_MODE_NUM, or update code to handle variable length mode values");
	// assert_msg((mode != 0), "Tried to enable algo to mode 0, but mode 0 is disable");

	uint8_t cmd_bytes[] = { SS_FAM_W_ALGOMODE, (uint8_t)idx, (uint8_t)mode };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes), 0, 0, 4*SS_ENABLE_SENSOR_SLEEP_MS);

	if (status == SS_SUCCESS) {
		algo_enabled_mode[idx] = mode;
		algo_data_reqs[idx] = data_req;
	}

	return status;
}

SS_STATUS SSInterface_disable_algo(int idx)
{
	//assert_msg((idx <= SS_MAX_SUPPORTED_ALGO_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_NUM, or update code to handle variable length idx values");
	uint8_t cmd_bytes[] = { SS_FAM_W_ALGOMODE, (uint8_t)idx, 0 };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes), 0, 0, SS_ENABLE_SENSOR_SLEEP_MS);

	if (status == SS_SUCCESS) {
		algo_enabled_mode[idx] = 0;
		algo_data_reqs[idx] = 0;
	}

	return status;
}

SS_STATUS SSInterface_set_algo_cfg(int algo_idx, int cfg_idx, uint8_t *cfg, int cfg_sz)
{
	//assert_msg((algo_idx <= SS_MAX_SUPPORTED_ALGO_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_NUM, or update code to handle variable length idx values");
	//assert_msg((cfg_idx <= SS_MAX_SUPPORTED_ALGO_CFG_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_CFG_NUM, or update code to handle variable length idx values");

	uint8_t cmd_bytes[] = { SS_FAM_W_ALGOCONFIG, (uint8_t)algo_idx, (uint8_t)cfg_idx };
	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								 cfg, cfg_sz, SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

SS_STATUS SSInterface_get_algo_cfg(int algo_idx, int cfg_idx, uint8_t *cfg, int cfg_sz)
{
	//assert_msg((algo_idx <= SS_MAX_SUPPORTED_ALGO_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_NUM, or update code to handle variable length idx values");
	//assert_msg((cfg_idx <= SS_MAX_SUPPORTED_ALGO_CFG_NUM), "idx must be < SS_MAX_SUPPORTED_ALGO_CFG_NUM, or update code to handle variable length idx values");

	uint8_t cmd_bytes[] = { SS_FAM_R_ALGOCONFIG, (uint8_t)algo_idx, (uint8_t)cfg_idx };
	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								cfg, cfg_sz,
								SS_DEFAULT_CMD_SLEEP_MS);

	return status;
}

SS_STATUS SSInterface_set_data_type(int data_type, bool sc_en)
{
	//assert_msg((data_type >= 0) && (data_type <= 3), "Invalid value for data_type");
	uint8_t cmd_bytes[] = { SS_FAM_W_COMMCHAN, SS_CMDIDX_OUTPUTMODE };
	uint8_t data_bytes[] = { (uint8_t)((sc_en ? SS_MASK_OUTPUTMODE_SC_EN : 0) |
							((data_type << SS_SHIFT_OUTPUTMODE_DATATYPE) & SS_MASK_OUTPUTMODE_DATATYPE)) };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								&data_bytes[0], ARRAY_SIZE(data_bytes),
								SS_DEFAULT_CMD_SLEEP_MS);

	m_data_type = data_type;
	m_sc_en = sc_en;

	return status;
}

SS_STATUS SSInterface_get_data_type(int *data_type, bool *sc_en)
{
	uint8_t cmd_bytes[] = { SS_FAM_R_COMMCHAN, SS_CMDIDX_OUTPUTMODE };
	uint8_t rxbuf[2] = {0};

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rxbuf[0], ARRAY_SIZE(rxbuf), (SS_DEFAULT_CMD_SLEEP_MS));
	if (status == SS_SUCCESS) {
		*data_type =
			(rxbuf[1] & SS_MASK_OUTPUTMODE_DATATYPE) >> SS_SHIFT_OUTPUTMODE_DATATYPE;
		*sc_en =
			(bool)((rxbuf[1] & SS_MASK_OUTPUTMODE_SC_EN) >> SS_SHIFT_OUTPUTMODE_SC_EN);
	}

	return status;
}

SS_STATUS SSInterface_set_fifo_thresh(int thresh)
{
	//assert_msg((thresh > 0 && thresh <= 255), "Invalid value for fifo a full threshold");
	uint8_t cmd_bytes[] = { SS_FAM_W_COMMCHAN, SS_CMDIDX_FIFOAFULL };
	uint8_t data_bytes[] = { (uint8_t)thresh };

	SS_STATUS status = write_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								&data_bytes[0], ARRAY_SIZE(data_bytes),
								SS_DEFAULT_CMD_SLEEP_MS);
	return status;
}

SS_STATUS SSInterface_get_fifo_thresh(int *thresh)
{
	uint8_t cmd_bytes[] = { SS_FAM_R_COMMCHAN, SS_CMDIDX_FIFOAFULL };
	uint8_t rxbuf[2] = {0};

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rxbuf[0], ARRAY_SIZE(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	if (status == SS_SUCCESS) {
		*thresh = rxbuf[1];
	}

	return status;
}

SS_STATUS SSInterface_ss_comm_check()
{
	uint8_t cmd_bytes[] = { SS_FAM_R_IDENTITY, SS_CMDIDX_PLATTYPE };
	uint8_t rxbuf[2];

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rxbuf[0], ARRAY_SIZE(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	int tries = 4;
	while (status == SS_ERR_TRY_AGAIN && tries--) {
		// wait_ms(1000);
		k_sleep(K_MSEC(1000));
		status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
							0, 0,
							&rxbuf[0], ARRAY_SIZE(rxbuf),
							SS_DEFAULT_CMD_SLEEP_MS);
	}

	return status;
}

static void fifo_sample_size(int data_type, int *sample_size)
{
	*sample_size = 0;

	if (data_type == SS_DATATYPE_RAW || data_type == SS_DATATYPE_BOTH) {
		for (int i = 0; i < SS_MAX_SUPPORTED_SENSOR_NUM; i++) {
			if (sensor_enabled_mode[i]) {
				//assert_msg(sensor_data_reqs[i], "no ss_data_req found for enabled sensor");
				*sample_size += sensor_data_reqs[i]->data_size;
			}
		}
	}

	if (data_type == SS_DATATYPE_ALGO || data_type == SS_DATATYPE_BOTH) {
		for (int i = 0; i < SS_MAX_SUPPORTED_ALGO_NUM; i++) {
			if (algo_enabled_mode[i]) {
				//assert_msg(algo_data_reqs[i], "no ss_data_req found for enabled algo");
				*sample_size += algo_data_reqs[i]->data_size;
			}
		}
	}
}

static SS_STATUS num_avail_samples(int *num_samples)
{
	uint8_t cmd_bytes[] = { SS_FAM_R_OUTPUTFIFO, SS_CMDIDX_OUT_NUMSAMPLES };
	uint8_t rxbuf[2] = {0};

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rxbuf[0], ARRAY_SIZE(rxbuf), 1);

	if (status == SS_SUCCESS) {
		*num_samples = rxbuf[1];
	}

	return status;
}

static SS_STATUS read_fifo_data( int num_samples, int sample_size, uint8_t* databuf, int databuf_sz)
{
	int bytes_to_read = num_samples * sample_size + 1; //+1 for status byte
	// assert_msg((bytes_to_read <= databuf_sz), "databuf too small");

	uint8_t cmd_bytes[] = { SS_FAM_R_OUTPUTFIFO, SS_CMDIDX_READFIFO };

	LOG_DBG("[reading %d bytes (%d samples)\r\n", bytes_to_read, num_samples);

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								databuf, bytes_to_read, 5);

	return status;
}

void SSInterface_ss_execute_once()
{

	if(m_irq_received_ == false)
		return;

	m_irq_received_ = false;
	static uint8_t databuf[512];
	uint8_t sample_count;
	uint8_t cmd_bytes[] = { SS_FAM_R_STATUS, SS_CMDIDX_STATUS };
	uint8_t rxbuf[2] = {0};

//	irq_evt.start();

	// irq_pin.disable_irq();
	// int ret = gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_DISABLE);
	// if (ret < 0) {
	// 	LOG_ERR("GPIO_INT_DISABLE failed");
	// 	return;
	// }

	SS_STATUS status = read_cmd(&cmd_bytes[0], ARRAY_SIZE(cmd_bytes),
								0, 0,
								&rxbuf[0], ARRAY_SIZE(rxbuf),
								SS_DEFAULT_CMD_SLEEP_MS);

	if (status != SS_SUCCESS) {
		LOG_ERR("Couldn't read status byte of SmartSensor!");
		// irq_pin.enable_irq();
		gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
//		irq_evt.stop();
		return;
	}

	if (rxbuf[1] & SS_MASK_STATUS_ERR) {
		LOG_ERR("SmartSensor status error: %d", rxbuf[1] & SS_MASK_STATUS_ERR);
	}
	if (rxbuf[1] & SS_MASK_STATUS_FIFO_OUT_OVR) {
		LOG_ERR("SmartSensor Output FIFO overflow!");
	}
	if (rxbuf[1] & SS_MASK_STATUS_FIFO_IN_OVR) {
		LOG_ERR("SmartSensor Input FIFO overflow!");
	}

	if (rxbuf[1] & SS_MASK_STATUS_DATA_RDY) {
		int num_samples = 1;
		status = num_avail_samples(&num_samples);
		if (status != SS_SUCCESS)
		{
			LOG_ERR("Couldn't read number of available samples in SmartSensor Output FIFO");
			// irq_pin.enable_irq();
			gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
//			irq_evt.stop();
			return;
		}

		int sample_size;
		fifo_sample_size(m_data_type, &sample_size);

		int bytes_to_read = num_samples * sample_size + 1; //+1 for status byte
		if ((uint32_t)bytes_to_read > sizeof(databuf)) {
			//Reduce number of samples to read to fit in buffer
			num_samples = (sizeof(databuf) - 1) / sample_size;
		}

		// wait_ms(5);
		k_sleep(K_MSEC(5));
		status = read_fifo_data(num_samples, sample_size, &databuf[0], sizeof(databuf));
		if (status != SS_SUCCESS)
		{
			LOG_ERR("Couldn't read from SmartSensor Output FIFO");
			// irq_pin.enable_irq();
			gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
//			irq_evt.stop();
			return;
		}

		LOG_DBG("read %d samples", num_samples);

		//Skip status byte
		uint8_t *data_ptr = &databuf[1];

		int i = 0;
		for (i = 0; i < num_samples; i++) {
			if (m_sc_en) {
				sample_count = *data_ptr++;
				LOG_INF("Received sample #%d", sample_count);
			}
				
			//Chop up data and send to modules with enabled sensors
			if (m_data_type == SS_DATATYPE_RAW || m_data_type == SS_DATATYPE_BOTH) {
				for (int i = 0; i < SS_MAX_SUPPORTED_SENSOR_NUM; i++) {
					if (sensor_enabled_mode[i]) {
						// assert_msg(sensor_data_reqs[i], "no ss_data_req found for enabled sensor");
						if (sensor_data_reqs[i] == NULL)
							LOG_ERR("no ss_data_req found for enabled sensor");
						sensor_data_reqs[i]->callback(data_ptr);
						data_ptr += sensor_data_reqs[i]->data_size;
					}
				}
			}
			if (m_data_type == SS_DATATYPE_ALGO || m_data_type == SS_DATATYPE_BOTH) {
				for (int i = 0; i < SS_MAX_SUPPORTED_ALGO_NUM; i++) {
					if (algo_enabled_mode[i]) {
						// assert_msg(algo_data_reqs[i], "no ss_data_req found for enabled algo");
						if (algo_data_reqs[i] == NULL)
							LOG_ERR("no ss_data_req found for enabled algo");
						algo_data_reqs[i]->callback(data_ptr);
						data_ptr += algo_data_reqs[i]->data_size;
					}
				}
			}
		}
	}
	// irq_pin.enable_irq();
	// ret = gpio_pin_interrupt_configure(irq_pin->port, irq_pin->pin, GPIO_INT_EDGE_FALLING);
	// if (ret < 0) {
	// 	LOG_ERR("GPIO_INT_EDGE_FALLING failed");
	// 	return;
	// }

//	irq_evt.stop();
}

void SSInterface_ss_clear_interrupt_flag()
{
	m_irq_received_ = false;
}

// void irq_handler()
static void irq_handler(const struct device *dev, 
						struct gpio_callback *cb,
						gpio_port_pins_t pins)
{
	m_irq_received_ = true;
}

#if REMOVE
// static void SSInterface_irq_handler_selftest(){
// 	mfio_int_happened = true;
// }

// bool SSInterface_reset_mfio_irq(){
// 	bool ret = mfio_int_happened;
// 	mfio_int_happened = false;
// 	irq_pin.disable_irq();
// 	irq_pin.fall(callback(this, &irq_handler));
// 	irq_pin.enable_irq();
// 	return ret;
// }
#endif /* REMOVE*/