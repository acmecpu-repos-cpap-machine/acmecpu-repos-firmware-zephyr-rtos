/*
 * Copyright (c) 2021 Acme CPU
 *
 * Author: Rohan Dey (rohan@acmecpu.com)
 */
//#define CONFIG_BLOWER_MOTOR_A101 1
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_blower);

#include "acpu_c201_modules.h"
#include "bsp_blower/bsp_blower.h"
#if (CONFIG_TPS55340)
	#include "tps55340.h"
#endif
#include "app_settings/app_settings.h"

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
#include "app_uart_m2m_com/app_bldc_drv/app_bldc_drv_cmds.h"
#include "app_uart_m2m_com/c20x_m2m_cmds.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif
#include "app_uart_m2m_com/app_uart_m2m_callback.h"
#elif CONFIG_BLOWER_MOTOR_A2
//#include "bsp_fan.h"
#endif

static BSP_BLOWER_RUNNING_STATUS m_blower_runstat = BLOWER_NOT_RUNNING;
#if (CONFIG_BLOWER_MOTOR_A2)
static uint8_t m_blower_duty = 0;	/* blower speed in percentage of maximum supported speed */
#endif
struct k_sem blower_lock;

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)

typedef enum {
	CB_BLW_STATE_SET_GET=0,
	CB_BLW_SPEED,
	CB_BLW_SPEED_RAMP,
	CB_BLW_RUNSTAT,
	CB_BLW_FAULT_ACK,
	CB_BLW_POWER,

	CB_NB
} READ_CB;
#define NUM_RESP_CB		(CB_NB)

static struct app_uart_m2m_callback m_cbdata_blw_state_set_get;
static struct app_uart_m2m_callback m_cbdata_blw_spd;
static struct app_uart_m2m_callback m_cbdata_blw_spdRamp;
static struct app_uart_m2m_callback m_cbdata_blw_runstat;
static struct app_uart_m2m_callback m_cbdata_blw_faultAck;
static struct app_uart_m2m_callback m_cbdata_blw_power;

static uint8_t m_blw_set_resp = BSP_BLOWER_FAIL;	// 0 ok, 1 fail
static uint8_t m_blw_state = BLOWER_OFF;
static int32_t m_acq_speed_rpm = 0;
static uint16_t m_faults = MC_NO_FAULTS;
static uint16_t m_power_avg_w = 0;
static uint16_t m_power_inst_w = 0;

struct resp_cb_data {
	bool resp_available;
	struct k_sem lock;
};

static struct resp_cb_data m_resp_cb[NUM_RESP_CB];

#define RESP_AVAILABLE_LOOP_DELAY	(1)
#define RESP_AVAILABLE_TIMEOUT		(2000)
static int lock_wait_until_timeout(struct k_sem *lock)
{
	int ret = k_sem_take(lock, K_MSEC(RESP_AVAILABLE_TIMEOUT));
	if (ret == -EAGAIN) {
		return -1;
	}
	return 0;
}

static void cb_handler_blw_state_set_get(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data) {
	if (cmd == C20X_M2M_CMD_BLOWER_STATE) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");
		tok = strtok(NULL, "\n");
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0)			m_blw_set_resp = BSP_BLOWER_OK;
			else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) 	m_blw_set_resp = BSP_BLOWER_FAIL;
			else if (strcmp(tok, "1") == 0)					m_blw_state = BLOWER_ON;
			else if (strcmp(tok, "0") == 0)					m_blw_state = BLOWER_OFF;

			m_resp_cb[CB_BLW_STATE_SET_GET].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_STATE_SET_GET].lock);
		}
	}
}

static void cb_handler_blw_speed_set_get(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data) {
	if (cmd == C20X_M2M_CMD_BLOWER_SPEED_RPM) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");
		tok = strtok(NULL, "\n");
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0)			m_blw_set_resp = BSP_BLOWER_OK;
			else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) 	m_blw_set_resp = BSP_BLOWER_FAIL;
			else {	/* response of speed get */
				m_acq_speed_rpm = atoi(tok);
			}
			m_resp_cb[CB_BLW_SPEED].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_SPEED].lock);
		}
	}
}

static void cb_handler_blw_runstat(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data) {
	if (cmd == C20X_M2M_CMD_BLOWER_RUNSTAT) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}

		char *tok = strtok(frame->payload, ",");	/* cmd */
		tok = strtok(NULL, ",");					/* faults */
		if (tok != NULL) {
			m_faults = atoi(tok);

			tok = strtok(NULL, "\n");					/* speed */
			if (tok != NULL)
				m_acq_speed_rpm = atoi(tok);

			m_resp_cb[CB_BLW_RUNSTAT].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_RUNSTAT].lock);
		}
	}
}

static void cb_handler_blw_faultAck(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data) {
	if (cmd == C20X_M2M_CMD_BLOWER_FLTACK) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");
		tok = strtok(NULL, "\n");
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0)			m_blw_set_resp = BSP_BLOWER_OK;
			else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) 	m_blw_set_resp = BSP_BLOWER_FAIL;
			m_resp_cb[CB_BLW_FAULT_ACK].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_FAULT_ACK].lock);
		}
	}
}

static void cb_handler_blw_spdRamp(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data) {
	if (cmd == C20X_M2M_CMD_BLOWER_SPEED_RAMP) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}
		char *tok = strtok(frame->payload, ",");
		tok = strtok(NULL, "\n");
		if (tok != NULL) {
			if (strcmp(tok, M2M_CMD_RESP_OK) == 0)			m_blw_set_resp = BSP_BLOWER_OK;
			else if (strcmp(tok, M2M_CMD_RESP_ERR) == 0) 	m_blw_set_resp = BSP_BLOWER_FAIL;
			m_resp_cb[CB_BLW_SPEED_RAMP].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_SPEED_RAMP].lock);
		}
	}
}

static void cb_handler_blw_power(struct app_uart_m2m_callback *cb, uint16_t cmd, void *data)
{
	if (cmd == C20X_M2M_CMD_BLOWER_POWER) {
		struct m2m_frame_t *frame = (struct m2m_frame_t *)data;

		if (frame->type != UART_M2M_FRAME_SINGLE_RESP) {
			LOG_ERR("Invalid frame type!");
			return;
		}

		if (frame->payload_len <= 0) {
			LOG_ERR("Invalid payload length");
			return;
		}

		char *tok = strtok(frame->payload, ",");	/* cmd */
		tok = strtok(NULL, ",");					/* power avg */
		if (tok != NULL) {
			m_power_avg_w = atoi(tok);

			tok = strtok(NULL, "\n");					/* power inst */
			if (tok != NULL)
				m_power_inst_w = atoi(tok);

			m_resp_cb[CB_BLW_POWER].resp_available = true;
			k_sem_give(&m_resp_cb[CB_BLW_POWER].lock);
		}
	}
}

#define BLW_STATE_GET	0xBD
static int blower_state_set_get(uint8_t state_set_get)
{
	int ret = 0;
	m_blw_set_resp = BSP_BLOWER_UNKNOWN;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	if (state_set_get == BLW_STATE_GET) {
		frame.payload_len = sprintf((char*)frame.payload, "%d%s%s%s",
									C20X_M2M_CMD_BLOWER_STATE,
									M2M_CMD_PAYLOAD_DELIM,
									M2M_CMD_PAYLOAD_GET_CHAR,
									M2M_CMD_PAYLOAD_TERM);
	} else {
		frame.payload_len = sprintf((char*)frame.payload, "%d%s%s%s",
									C20X_M2M_CMD_BLOWER_STATE,
									M2M_CMD_PAYLOAD_DELIM,
									state_set_get ? "1" : "0",
									M2M_CMD_PAYLOAD_TERM);
	}

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);

	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_STATE_SET_GET].lock) < 0)
		return -1;

	if (state_set_get == BLW_STATE_GET) {
		if (m_blw_state == BLOWER_ON)	m_blower_runstat = BLOWER_RUNNING;
		if (m_blw_state == BLOWER_OFF)	m_blower_runstat = BLOWER_NOT_RUNNING;
		ret = 0;
	} else {
		if (m_blw_set_resp == BSP_BLOWER_OK)	ret = 0;
		else if (m_blw_set_resp == BSP_BLOWER_FAIL)	ret = -1;
	}
	return ret;
}

#define BLW_SPEED_SET	1
#define BLW_SPEED_GET	0
static int blower_speed_set_get(int32_t speed_rpm_in, bool set_get, int32_t *speed_rpm_out)
{
	int ret = 0;
	m_blw_set_resp = BSP_BLOWER_UNKNOWN;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	if (set_get == BLW_SPEED_GET) {
		frame.payload_len = sprintf((char*)frame.payload, "%d%s%s%s",
									C20X_M2M_CMD_BLOWER_SPEED_RPM,
									M2M_CMD_PAYLOAD_DELIM,
									M2M_CMD_PAYLOAD_GET_CHAR,
									M2M_CMD_PAYLOAD_TERM);
	} else {
		frame.payload_len = sprintf((char*)frame.payload, "%d%s%d%s",
									C20X_M2M_CMD_BLOWER_SPEED_RPM,
									M2M_CMD_PAYLOAD_DELIM,
									speed_rpm_in,
									M2M_CMD_PAYLOAD_TERM);
	}

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
//	int64_t start = k_uptime_get();

	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_SPEED].lock) < 0)
		return -1;

//	LOG_INF("delta = %ld", k_uptime_delta(&start));

	if (set_get == BLW_SPEED_GET) {
		if (speed_rpm_out != NULL) {
			*speed_rpm_out = m_acq_speed_rpm;
			ret = 0;
		} else ret = -1;
	} else {
		if (m_blw_set_resp == BSP_BLOWER_OK)	ret = 0;
		else if (m_blw_set_resp == BSP_BLOWER_FAIL)	ret = -1;
	}
	return ret;
}

int blower_spdRamp_set(int32_t speed_rpm, uint32_t ramp_ms) {
	int ret = 0;
	m_blw_set_resp = BSP_BLOWER_UNKNOWN;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = sprintf((char*)frame.payload, "%d%s%d%s%d%s",	/* 24,-15000,500\n */
								C20X_M2M_CMD_BLOWER_SPEED_RAMP,
								M2M_CMD_PAYLOAD_DELIM,
								speed_rpm,
								M2M_CMD_PAYLOAD_DELIM,
								ramp_ms,
								M2M_CMD_PAYLOAD_TERM);

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX+frame.payload_len+1;
	uint32_t sdata_len=0;
	uint8_t *serialized_buffer = (uint8_t *) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_SPEED_RAMP].lock) < 0)
		return -1;

	if (m_blw_set_resp == BSP_BLOWER_OK)	ret = 0;
	else if (m_blw_set_resp == BSP_BLOWER_FAIL)	ret = -1;

	return ret;
}

int blower_fault_ack() {
	int ret = 0;
	m_blw_set_resp = BSP_BLOWER_UNKNOWN;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = sprintf((char*) frame.payload, "%d%s%d%s",
									C20X_M2M_CMD_BLOWER_FLTACK,
									M2M_CMD_PAYLOAD_DELIM, 1,
									M2M_CMD_PAYLOAD_TERM);

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame.payload_len + 1;
	uint32_t sdata_len = 0;
	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
//	int64_t start = k_uptime_get();

	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_FAULT_ACK].lock) < 0)
		return -1;
//	LOG_INF("delta = %ld", k_uptime_delta(&start));

	if (m_blw_set_resp == BSP_BLOWER_OK) ret = 0;
	else if (m_blw_set_resp == BSP_BLOWER_FAIL) ret = -1;

	return ret;
}

int blower_runstat_get(uint16_t *fault, int32_t *speed_rpm) {
	int ret = 0;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = sprintf((char*) frame.payload, "%d%s%s%s",
									C20X_M2M_CMD_BLOWER_RUNSTAT,
									M2M_CMD_PAYLOAD_DELIM,
									M2M_CMD_PAYLOAD_GET_CHAR,
									M2M_CMD_PAYLOAD_TERM);

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame.payload_len + 1;
	uint32_t sdata_len = 0;
	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
//	int64_t start = k_uptime_get();

	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_RUNSTAT].lock) < 0)
		return -1;
//	LOG_INF("delta = %ld", k_uptime_delta(&start));

	*fault = m_faults;
	*speed_rpm = m_acq_speed_rpm;

	return ret;
}

int blower_power_get(uint16_t *pow_avg_w, uint16_t *pow_inst_w)
{
	int ret = 0;

	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));

	lib_m2m_frame_header_single_req_make(&frame);

	frame.payload_len = sprintf((char*) frame.payload, "%d%s%s%s",
									C20X_M2M_CMD_BLOWER_POWER,
									M2M_CMD_PAYLOAD_DELIM,
									M2M_CMD_PAYLOAD_GET_CHAR,
									M2M_CMD_PAYLOAD_TERM);

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame.payload_len + 1;
	uint32_t sdata_len = 0;
	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
//	int64_t start = k_uptime_get();

	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);

	/* wait for response until timeout */
	if (lock_wait_until_timeout(&m_resp_cb[CB_BLW_POWER].lock) < 0)
		return -1;
//	LOG_INF("delta = %ld", k_uptime_delta(&start));

	*pow_avg_w = m_power_avg_w;
	*pow_inst_w = m_power_inst_w;

	return ret;
}
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */

int bsp_blower_init() {
	int ret = 0;

	k_sem_init(&blower_lock, 1, 1);

#if (CONFIG_BLOWER_MOTOR_A2)
#if (SETTINGS_NEEDED)
	/* retrieve the last blower duty from the persistent memory */
	if (app_settings_load_single(SETTINGS_KEY_FULL_BLOWER_DUTY, &m_blower_duty, sizeof(m_blower_duty)) != 0) {
		m_blower_duty = 25;
	}
#endif
#endif

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	/* register callback handlers for each blower commands */
	app_uart_m2m_callback_add(&m_cbdata_blw_state_set_get, cb_handler_blw_state_set_get, C20X_M2M_CMD_BLOWER_STATE);
	k_sem_init(&m_resp_cb[CB_BLW_STATE_SET_GET].lock, 0, 1);

	app_uart_m2m_callback_add(&m_cbdata_blw_spd, cb_handler_blw_speed_set_get, C20X_M2M_CMD_BLOWER_SPEED_RPM);
	k_sem_init(&m_resp_cb[CB_BLW_SPEED].lock, 0, 1);

	app_uart_m2m_callback_add(&m_cbdata_blw_runstat, cb_handler_blw_runstat, C20X_M2M_CMD_BLOWER_RUNSTAT);
	k_sem_init(&m_resp_cb[CB_BLW_RUNSTAT].lock, 0, 1);

	app_uart_m2m_callback_add(&m_cbdata_blw_faultAck, cb_handler_blw_faultAck, C20X_M2M_CMD_BLOWER_FLTACK);
	k_sem_init(&m_resp_cb[CB_BLW_FAULT_ACK].lock, 0, 1);

	app_uart_m2m_callback_add(&m_cbdata_blw_spdRamp, cb_handler_blw_spdRamp, C20X_M2M_CMD_BLOWER_SPEED_RAMP);
	k_sem_init(&m_resp_cb[CB_BLW_SPEED_RAMP].lock, 0, 1);

	app_uart_m2m_callback_add(&m_cbdata_blw_power, cb_handler_blw_power, C20X_M2M_CMD_BLOWER_POWER);
	k_sem_init(&m_resp_cb[CB_BLW_POWER].lock, 0, 1);

#endif /*(CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */
	return ret;
}

int bsp_blower_reset() {
	int ret = -1;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
#if 1
	const struct device *dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(bldc_rst), gpios));//device_get_binding(DT_GPIO_LABEL(DT_NODELABEL(bldc_rst), gpios));
	if (!dev) {
		LOG_ERR("Device %s not found", dev->name);
		return -ENXIO;
	}
	gpio_pin_t pin = DT_GPIO_PIN(DT_NODELABEL(bldc_rst), gpios);
	gpio_flags_t flags = DT_GPIO_FLAGS(DT_NODELABEL(bldc_rst), gpios);

	ret = gpio_pin_configure(dev, pin, (flags | GPIO_OUTPUT));
	if (ret != 0) {
		LOG_ERR("Failed to configure enable pin %d (%d)", pin, ret);
		return -1;
	}

	/* apply reset pulse */
	ret = gpio_pin_set(dev, pin, 0);
	if (ret != 0) {
		LOG_ERR("Error setting enable GPIO");
		return -1;
	}

	k_sleep(K_USEC(100));
	ret = gpio_pin_set(dev, pin, 1);

/*
	k_sleep(K_MSEC(100));

	ret = gpio_pin_configure(dev, pin, (GPIO_INPUT));
	if (ret != 0) {
		LOG_ERR("Failed to configure enable pin %d (%d)", pin, ret);
		return -1;
	}
*/
#else
	k_sleep(K_MSEC(1000));	// wait for the driver to boot
	/* make and send command to bldc driver */
	struct m2m_frame_t frame;
	memset(&frame, 0x00, sizeof(frame));
	lib_m2m_frame_header_single_req_make(&frame);
	frame.payload_len = sprintf((char*) frame.payload, "%d%s%d%s",
									C20X_M2M_CMD_ID_DO_SW_RST,
									M2M_CMD_PAYLOAD_DELIM, 1,
									M2M_CMD_PAYLOAD_TERM);

	/* serialize the frame and send */
	/* buffer size = frame header size + pay load size + 1 NULL char */
	uint32_t sbuf_len = UART_M2M_HEADER_SIZE_MAX + frame.payload_len + 1;
	uint32_t sdata_len = 0;
	uint8_t *serialized_buffer = (uint8_t*) calloc(1, sbuf_len);
	if (serialized_buffer == NULL) {
		LOG_ERR("%s calloc failed!", __func__);
		free(serialized_buffer);
		return -1;
	}
	lib_m2m_frame_serialize(serialized_buffer, sbuf_len, &frame, &sdata_len);
	ret = app_bldc_drv_cmd_send(serialized_buffer, sdata_len);
	free(serialized_buffer);
#endif
#endif

	return ret;
}

int bsp_blower_on() {
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A2)
#if (CONFIG_TPS55340)
	/* provide motor voltage */
	const struct device *boost_dev = device_get_binding(ACPU_C201_MOD_NAME_BOOST);
	if (!boost_dev) {
		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BOOST);
		return -ENXIO;
	}
	struct tps55340_driver_api *boost_api = (struct tps55340_driver_api*) boost_dev->api;

	k_sem_take(&blower_lock, K_FOREVER);

	ret = boost_api->enable(boost_dev);
	if (ret != 0) {
		LOG_ERR("Unable to enable %s, ret = %d", ACPU_C201_MOD_NAME_BOOST, ret);
		k_sem_give(&blower_lock);
		return ret;
	}
	k_sem_give(&blower_lock);
#endif	/*(CONFIG_TPS55340)*/
#endif	/*(CONFIG_BLOWER_MOTOR_A2)*/

#if (CONFIG_BLOWER_MOTOR_A2)
	m_blower_runstat = BLOWER_RUNNING;
#elif (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_state_set_get(BLOWER_ON);
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */

	return ret;
}

int bsp_blower_off() {
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A2)
#if (CONFIG_TPS55340)
	const struct device *boost_dev = device_get_binding(ACPU_C201_MOD_NAME_BOOST);
	if (!boost_dev) {
		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BOOST);
		return -ENXIO;
	}
	struct tps55340_driver_api *boost_api = (struct tps55340_driver_api*) boost_dev->api;

	k_sem_take(&blower_lock, K_FOREVER);

	ret = boost_api->disable(boost_dev);
	if (ret != 0) {
		LOG_ERR("Unable to disable %s, ret = %d", ACPU_C201_MOD_NAME_BOOST, ret);
		k_sem_give(&blower_lock);
		return ret;
	}

	k_sem_give(&blower_lock);
#endif	/*(CONFIG_TPS55340)*/
#endif	/*(CONFIG_BLOWER_MOTOR_A2)*/

#if (CONFIG_BLOWER_MOTOR_A2)
	m_blower_runstat = BLOWER_NOT_RUNNING;
#elif (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_state_set_get(BLOWER_OFF);
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */
	return ret;
}

int bsp_blower_oper_voltage_set(float blower_voltage) {
	int ret = 0;
#if (CONFIG_TPS55340)
	const struct device *boost_dev = device_get_binding(ACPU_C201_MOD_NAME_BOOST);
	if (!boost_dev) {
		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BOOST);
		return -ENXIO;
	}
	struct tps55340_driver_api *boost_api = (struct tps55340_driver_api*) boost_dev->api;

	k_sem_take(&blower_lock, K_FOREVER);

	ret = boost_api->tps55340_output_voltage_set(boost_dev, blower_voltage);
	if (ret != 0) {
		LOG_ERR("Unable to set voltage %d mV to %s, ret = %d",
				(uint32_t)(blower_voltage * 1000), ACPU_C201_MOD_NAME_BOOST, ret);
		goto err;
	}

err:
	k_sem_give(&blower_lock);
#endif /*(CONFIG_TPS55340)*/
	return ret;
}

int bsp_blower_oper_voltage_get(int32_t *blower_voltage) {
	int ret = 0;
#if (CONFIG_TPS55340)
	const struct device *boost_dev = device_get_binding(ACPU_C201_MOD_NAME_BOOST);
	if (!boost_dev) {
		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BOOST);
		return -ENXIO;
	}
	struct tps55340_driver_api *boost_api = (struct tps55340_driver_api*) boost_dev->api;

	k_sem_take(&blower_lock, K_FOREVER);

	ret = boost_api->tps55340_output_voltage_get(boost_dev, blower_voltage);
	if (ret != 0) {
		LOG_ERR("Unable to get voltage from %s, ret = %d", ACPU_C201_MOD_NAME_BOOST, ret);
		goto err;
	}

err:
	k_sem_give(&blower_lock);
#endif /*(CONFIG_TPS55340)*/
	return ret;
}

int bsp_blower_is_running() {
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	blower_state_set_get(BLW_STATE_GET);
#endif
	return m_blower_runstat;
}

int bsp_blower_duty_cycle_set(uint8_t duty_percent) {
	int ret = 0;

	if ((duty_percent) > 100) {
		LOG_ERR("Invalid duty cycle percentage %d", duty_percent);
		return -EINVAL;
	}

#if CONFIG_BLOWER_MOTOR_A101

#elif CONFIG_BLOWER_MOTOR_A102

#elif CONFIG_BLOWER_MOTOR_A2
//	const struct device *bsp_fan_dev = device_get_binding(ACPU_C201_MOD_NAME_BLOWER);
//	if (!bsp_fan_dev) {
//		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BLOWER);
//		return -ENXIO;
//	}
//	struct bsp_fan_api *bsp_fan_api = (struct bsp_fan_api*) bsp_fan_dev->api;
//
//	k_sem_take(&blower_lock, K_FOREVER);
//
//	m_blower_duty = duty_percent;
//	ret = bsp_fan_api->speed_ctrl_pwm(bsp_fan_dev, m_blower_duty);
//	if (ret != 0) {
//		LOG_ERR("Unable to set duty cycle %d, ret = %d", m_blower_duty, ret);
//	}
//
//	k_sem_give(&blower_lock);
#endif

	return ret;
}

int bsp_blower_speed_inc(uint8_t percent_inc) {
	int ret = 0;

#if CONFIG_BLOWER_MOTOR_A101

#elif CONFIG_BLOWER_MOTOR_A102

#elif CONFIG_BLOWER_MOTOR_A2
//	const struct device *bsp_fan_dev = device_get_binding(ACPU_C201_MOD_NAME_BLOWER);
//	if (!bsp_fan_dev) {
//		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BLOWER);
//		return -ENXIO;
//	}
//	struct bsp_fan_api *bsp_fan_api = (struct bsp_fan_api*) bsp_fan_dev->api;
//
//	k_sem_take(&blower_lock, K_FOREVER);
//
//	m_blower_duty += percent_inc;
//	if ((m_blower_duty) > 100) {
//		m_blower_duty = 100;
//	}
//
//	ret = bsp_fan_api->speed_ctrl_pwm(bsp_fan_dev, m_blower_duty);
//
//	k_sem_give(&blower_lock);
#endif

	return ret;
}

int bsp_blower_speed_dec(uint8_t percent_dec) {
	int ret = 0;

#if CONFIG_BLOWER_MOTOR_A101
#elif CONFIG_BLOWER_MOTOR_A102
#elif CONFIG_BLOWER_MOTOR_A2
//	const struct device *bsp_fan_dev = device_get_binding(ACPU_C201_MOD_NAME_BLOWER);
//	if (!bsp_fan_dev) {
//		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BLOWER);
//		return -ENXIO;
//	}
//	struct bsp_fan_api *bsp_fan_api = (struct bsp_fan_api*) bsp_fan_dev->api;
//
//	k_sem_take(&blower_lock, K_FOREVER);
//
//	if (m_blower_duty <= percent_dec) {
//		m_blower_duty = 0;
//	} else {
//		m_blower_duty -= percent_dec;
//	}
//
//	ret = bsp_fan_api->speed_ctrl_pwm(bsp_fan_dev, m_blower_duty);
//
//	k_sem_give(&blower_lock);
#endif
	return ret;
}

int bsp_blower_speed_set(int32_t speed_rpm) {
	int ret = 0;

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_speed_set_get(speed_rpm, BLW_SPEED_SET, NULL);
#elif CONFIG_BLOWER_MOTOR_A2

#endif

	return ret;
}

int bsp_blower_speed_get(uint8_t *speed_percent, uint32_t *speed_hz, int32_t *speed_rpm) {
	int ret = 0;

#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)

	ret = blower_speed_set_get(0, BLW_SPEED_GET, speed_rpm);

#elif CONFIG_BLOWER_MOTOR_A2
//	const struct device *bsp_fan_dev = device_get_binding(ACPU_C201_MOD_NAME_BLOWER);
//	if (!bsp_fan_dev) {
//		LOG_ERR("Device %s not found", ACPU_C201_MOD_NAME_BLOWER);
//		return -ENXIO;
//	}
//	struct bsp_fan_api *bsp_fan_api = (struct bsp_fan_api*) bsp_fan_dev->api;
//
//	k_sem_take(&blower_lock, K_FOREVER);
//
//	if (speed_percent != NULL) {
//		*speed_percent = m_blower_duty;
//	}
//
//	if ((speed_hz != NULL) && (speed_rpm != NULL)) {
//		uint32_t fan_freq_hz = bsp_fan_api->fan_freq_get(bsp_fan_dev);
//
//		/* TODO: verify frequency and rpm relation from motor datasheet */
//		uint32_t fan_rpm = fan_freq_hz * 60;
//		*speed_hz = fan_freq_hz;
//		*speed_rpm = fan_rpm;
//	}
//	k_sem_give(&blower_lock);
#endif
	return ret;
}

int bsp_blower_runstat_get(uint16_t *fault, int32_t *speed_rpm) {
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_runstat_get(fault, speed_rpm);
#endif
	return ret;
}

int bsp_blower_fault_ack() {
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_fault_ack();
#endif
	return ret;
}

int bsp_blower_spdRamp_set(int32_t speed_rpm, uint32_t ramp_ms) {
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_spdRamp_set(speed_rpm, ramp_ms);
#endif
	return ret;
}

int bsp_blower_power_get(uint16_t *pow_avg_w, uint16_t *pow_inst_w)
{
	int ret = 0;
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = blower_power_get(pow_avg_w, pow_inst_w);
#endif
	return ret;
}
