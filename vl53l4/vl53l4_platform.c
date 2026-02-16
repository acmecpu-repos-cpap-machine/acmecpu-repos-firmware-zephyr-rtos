/* vl53l4_platform.c - Zephyr customization of ST vl53l4x library.
 */

/*
 * Copyright (c) 2023 Prosaris Solutions Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vl53l4_platform.h"
#include "vl53l4_platform_log.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(VL53L4X, CONFIG_SENSOR_LOG_LEVEL);

VL53L4_Error VL53L4_WriteMulti(VL53L4_Dev_t *pdev, uint16_t reg,
		uint8_t *pdata, uint32_t count)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;
	int32_t status_int = 0;
	uint8_t buffer[count + 2];

	/* To be able to write to the 16-bit registers/addresses on the vl53l4x */
	buffer[1] = (uint8_t)(reg & 0x00ff);
	buffer[0] = (uint8_t)((reg & 0xff00) >> 8);

	memcpy(&buffer[2], pdata, count);

	status_int = i2c_write_dt(pdev->i2c, buffer, count + 2);

	if (status_int < 0) {
		status = VL53L4_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to write");
	}

	return status;
}

VL53L4_Error VL53L4_ReadMulti(VL53L4_Dev_t *pdev, uint16_t reg,
		uint8_t *pdata, uint32_t count)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;
	int32_t status_int = 0;

	reg = sys_cpu_to_be16(reg);

	status_int = i2c_write_read_dt(pdev->i2c, (uint8_t *)(&reg), 2, pdata, count);

	if (status_int < 0) {
		status = VL53L4_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to read");
		return -EIO;
	}

	return status;
}

VL53L4_Error VL53L4_WrByte(VL53L4_Dev_t *pdev, uint16_t reg, uint8_t data)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	status = VL53L4_WriteMulti(pdev, reg, &data, 1);

	return status;
}

VL53L4_Error VL53L4_WrWord(VL53L4_Dev_t *pdev, uint16_t reg, uint16_t data)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	data = sys_cpu_to_be16(data);

	status = VL53L4_WriteMulti(pdev, reg, (uint8_t *)(&data), VL53L4_BYTES_PER_WORD);

	return status;
}

VL53L4_Error VL53L4_WrDWord(VL53L4_Dev_t *pdev, uint16_t reg, uint32_t data)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	data = sys_cpu_to_be32(data);

	status = VL53L4_WriteMulti(pdev, reg, (uint8_t *)(&data), VL53L4_BYTES_PER_DWORD);

	return status;
}

VL53L4_Error VL53L4_RdByte(VL53L4_Dev_t *pdev, uint16_t reg, uint8_t *pdata)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	status = VL53L4_ReadMulti(pdev, reg, pdata, 1);

	return status;
}

VL53L4_Error VL53L4_RdWord(VL53L4_Dev_t *pdev, uint16_t reg, uint16_t *pdata)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	status = VL53L4_ReadMulti(pdev, reg, (uint8_t *)pdata, 2);
	*pdata = sys_be16_to_cpu(*pdata);

	return status;
}

VL53L4_Error VL53L4_RdDWord(VL53L4_Dev_t *pdev, uint16_t reg, uint32_t *pdata)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;

	status = VL53L4_ReadMulti(pdev, reg, (uint8_t *)pdata, 4);
	*pdata = sys_be32_to_cpu(*pdata);

	return status;
}

VL53L4_Error VL53L4_WaitUs(VL53L4_Dev_t *pdev, int32_t wait_us)
{
	k_sleep(K_USEC(wait_us));
	return VL53L4_ERROR_NONE;
}

VL53L4_Error VL53L4_WaitMs(VL53L4_Dev_t *pdev, int32_t wait_ms)
{
	return VL53L4_WaitUs(pdev, wait_ms * 1000);
}

VL53L4_Error VL53L4_GetTickCount(uint32_t *ptick_count_ms)
{
	*ptick_count_ms = k_uptime_get_32();
	return VL53L4_ERROR_NONE;
}

VL53L4_Error VL53L4_WaitValueMaskEx(VL53L4_Dev_t *dev, uint32_t timeout, uint16_t i, uint8_t val,
	uint8_t mask, uint32_t delay)
{
	VL53L4_Error status = VL53L4_ERROR_NONE;
	uint32_t start_time_ms = 0;
	uint32_t current_time_ms = 0;
	uint8_t byte_val = 0;
	uint8_t found = 0;

	/* calculate time limit in absolute time */
	VL53L4_GetTickCount(&start_time_ms);
	dev->new_data_ready_poll_duration_ms = 0;

	/* wait until val is found, timeout reached on error occurred */
	while ((status == VL53L4_ERROR_NONE) &&
	       (dev->new_data_ready_poll_duration_ms < timeout) &&
	       (found == 0)) {
		status = VL53L4_RdByte(dev, i, &byte_val);

		if ((byte_val & mask) == val) {
			found = 1;
		}

		if ((status == VL53L4_ERROR_NONE) && (found == 0) && (delay > 0)) {
			/* Allow for other threads to run */
			status = VL53L4_WaitMs(dev, delay);
		}

		/* Update polling time (Compare difference rather than absolute to
		 * negate 32bit wrap around issue)
		 */
		VL53L4_GetTickCount(&current_time_ms);
		dev->new_data_ready_poll_duration_ms = current_time_ms - start_time_ms;
	}

	if (found == 0 && status == VL53L4_ERROR_NONE) {
		status = VL53L4_ERROR_TIME_OUT;
	}

	return status;
}
