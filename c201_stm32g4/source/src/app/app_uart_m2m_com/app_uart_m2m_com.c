/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 28-Apr-2022
 *      Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_uart_m2m);

#include <string.h>
#include <stdlib.h>

#include "acpu_c201_modules.h"
#include "app_uart_m2m_com_priv.h"
#include "app_uart_m2m_com/app_wifi_bt/app_wifi_bt.h"
#include "app_uart_m2m_com/app_bldc_drv/app_bldc_drv.h"
//#include "app_uart_m2m_com/app_uart_m2m_com_frame.h"
#if (CONFIG_LIB_M2M_FRAME)
#include "lib_m2m_frame/lib_m2m_frame.h"
#endif


#if RING_BUF_TEST
	#include <zephyr/sys/ring_buffer.h>
#endif

static struct bsp_uart_serial_config m_uart_cfg_tbl[UART_M2M_APP_ID_MAX];
static struct app_uart_map m_au_map[UART_M2M_APP_ID_MAX] = APP_TO_UART_MAP_INITIALIZER;


/* Work queue handlers */
static void uart_cb_work_handler(struct k_work *work) {
	struct bsp_uart_serial_config *cfg = CONTAINER_OF(work, struct bsp_uart_serial_config, m2m_work);
	if (cfg == NULL) {
		LOG_ERR("Failed to obtain context pointer?");
		return;
	}

	/* send the received data to the application */
	k_sem_take(&cfg->copy_sem, K_FOREVER);
	if (cfg->au_map->app_cb != NULL)
#if NEW_BUF_TEST
		cfg->au_map->app_cb(cfg->app_buf.buf, cfg->app_buf.len);
#elif RING_BUF_TEST

#else
		cfg->au_map->app_cb(&cfg->uart_buf[0], cfg->uart_buf_ctr);
#endif
	k_sem_give(&cfg->copy_sem);
}


/*
 * A byte has been received from a serial port. We just store it in the buffer
 * for processing when a complete packet has been received.
 */
static void cb_handler_rx(struct bsp_uart_serial_config *cfg)
{
	uint8_t c;
	static uint8_t type = 0;
	static uint16_t sof = 0x00;
	static uint8_t sof_byte = 0;

//	if (type == 3) {
//		uart_fifo_read(cfg->au_map->dev, &c, 1);
//		return;
//	}
	if (cfg->tx_type == TRANSACT_SOF_DETECT) {
		if (uart_fifo_read(cfg->au_map->dev, &c, 1) != 1) {
			LOG_ERR("UART-ISR: Failed to read UART");
			return;
		}

		if ((sof_byte == 0) && (c == UART_M2M_SOF_BYTE_1)) {
			sof_byte = 1;
			sof = c;
//			sof = sof << 8;
		} else if ((sof_byte == 1)  && (c == UART_M2M_SOF_BYTE_2)) {
			sof_byte = 0;
			uint16_t sof2 = c;
			sof = (sof2 << 8) | sof;
		}

		if (sof == UART_M2M_START_OF_FRAME) {
			/* Restart a new frame */
			LOG_DBG("UART-ISR: SOF = 0x%x, sof_byte = %d", sof, sof_byte);
			memset(cfg->uart_buf, 0x00, sizeof(cfg->uart_buf));
			cfg->uart_buf_ptr = &cfg->uart_buf[0];
			memcpy(cfg->uart_buf_ptr, &sof, sizeof(sof));
			cfg->uart_buf_ptr += sizeof(sof);
			cfg->uart_buf_ctr = sizeof(sof);
			cfg->tx_type = TRANSACT_SINGLE_BYTE;
		}
		LOG_DBG("UART-ISR: rx = 0x%x", c);
	} else if (cfg->tx_type == TRANSACT_SINGLE_BYTE) {
		if (uart_fifo_read(cfg->au_map->dev, &c, 1) != 1) {
			LOG_ERR("UART-ISR: Failed to read UART");
			return;
		}

		if (cfg->uart_buf_ctr < UART_M2M_FRAME_SIZE_MAX) {
			*cfg->uart_buf_ptr++ = c;
			cfg->uart_buf_ctr++;
		} else {
			LOG_ERR("UART-ISR: parse error!");
		}

		if (cfg->uart_buf_ctr == UART_M2M_HEADER_SIZE_MAX) {
			memcpy(&cfg->bulk_sz, (cfg->uart_buf_ptr - sizeof(cfg->bulk_sz)), sizeof(cfg->bulk_sz));

			LOG_HEXDUMP_DBG(cfg->uart_buf, UART_M2M_HEADER_SIZE_MAX, "uart_buf");

			if (cfg->bulk_sz > (UART_M2M_FRAME_SIZE_MAX - UART_M2M_HEADER_SIZE_MAX)) {
				LOG_ERR("UART-ISR: Payload = %d", cfg->bulk_sz);
				cfg->tx_type = TRANSACT_SOF_DETECT;	// packet is corrupted, check SOF of next packet
				return;
			}

			cfg->tx_type = TRANSACT_BULK;
			type = cfg->uart_buf[1];
		}
	} else if (cfg->tx_type == TRANSACT_BULK) {
		int n = -1;
		n = uart_fifo_read(cfg->au_map->dev, cfg->uart_buf_ptr, cfg->bulk_sz);

		cfg->uart_buf_ptr += n;
		cfg->uart_buf_ctr += n;

		/* calculate balance data to be read */
		cfg->bulk_sz = cfg->bulk_sz - n;

		if (cfg->bulk_sz == 0) {
			/* end of transaction */
			cfg->tx_type = TRANSACT_SOF_DETECT;
			sof_byte = 0;
			LOG_DBG("UART-ISR: sof_byte = %d", sof_byte);

#if NEW_BUF_TEST
			/* copy the data into the application buffer */
			cfg->app_buf.buf = (char*) calloc(1, cfg->uart_buf_ctr);
			if (cfg->app_buf.buf == NULL) {
				LOG_ERR("UART-ISR: %s calloc failed", __func__);
				return;
			}
			memcpy(cfg->app_buf.buf, cfg->uart_buf, cfg->uart_buf_ctr);
			cfg->app_buf.len = cfg->uart_buf_ctr;

			/* send data to app cb*/
			if (cfg->au_map->app_cb != NULL) {
				cfg->au_map->app_cb(cfg->app_buf.buf, cfg->app_buf.len);
			}
#elif RING_BUF_TEST
//			if (type == 3) return;

			int len = ring_buf_put(&cfg->rb, cfg->uart_buf, cfg->uart_buf_ctr);
			if (len != cfg->uart_buf_ctr) {
				LOG_WRN("could not put all bytes into the ring buffer, req = %d, put = %d", cfg->uart_buf_ctr, len);
				uart_irq_rx_disable(cfg->au_map->dev);
				return;
			}
			LOG_INF("ring_buf_put = %d", len);
			if (cfg->au_map->app_cb != NULL) {
//				cfg->au_map->app_cb(&cfg->rb, len);
				cfg->au_map->app_cb(cfg, len);
			}
#else
			/* submit work queue to process the data */
			k_work_submit(&cfg->m2m_work);
#endif
		}
	} else {
		LOG_ERR("UART-ISR: tx type error!");
	}
}

/*
static void cb_handler_tx(struct modbus_context *ctx)
{
	struct modbus_serial_config *cfg = ctx->cfg;
	int n;

	if (cfg->uart_buf_ctr > 0) {
		n = uart_fifo_fill(cfg->dev, cfg->uart_buf_ptr,
				   cfg->uart_buf_ctr);
		cfg->uart_buf_ctr -= n;
		cfg->uart_buf_ptr += n;
	} else {
		 Disable transmission
		cfg->uart_buf_ptr = &cfg->uart_buf[0];
		modbus_serial_tx_off(ctx);
		modbus_serial_rx_on(ctx);
	}
}
*/

static void uart_cb_handler(const struct device *dev, void *app_data) {
	struct bsp_uart_serial_config *cfg = (struct bsp_uart_serial_config *)app_data;

	if (uart_irq_update(cfg->au_map->dev) && uart_irq_is_pending(cfg->au_map->dev)) {

		if (uart_irq_rx_ready(cfg->au_map->dev)) {
			cb_handler_rx(cfg);
		}

/*
		if (uart_irq_tx_ready(cfg->au_map->dev)) {
			cb_handler_tx(cfg);
		}
*/
	}
}

int app_uart_m2m_send(UART_M2M_APP_ID uart_app_id, const void *data, int len)
{
	const uint8_t *u8p;
	const struct device* dev = m_uart_cfg_tbl[uart_app_id].au_map->dev;
	if (dev == NULL) {
		return ENXIO;
	}

	u8p = data;
	while (len--) {
		uart_poll_out(dev, *u8p++);
	}

	return 0;
}

int app_uart_m2m_configure(UART_M2M_APP_ID uart_app_id, app_uart_m2m_cb_t app_cb) {
	int ret = 0;
	uint8_t c;
	struct uart_config uart_cfg;

	/* get the device pointer of the appropriate uart interface and store it */
//	const char *dev_name = m_uart_cfg_tbl[uart_app_id].au_map->dev_name;
//	const struct device* dev = device_get_binding(dev_name);
//	m_uart_cfg_tbl[uart_app_id].au_map->dev = dev;
//	m_uart_cfg_tbl[uart_app_id].au_map->app_cb = app_cb;

	const struct device* dev;
	switch (uart_app_id) {
	case UART_M2M_APP_ID_WIFI_BT:
	{
		dev = DEVICE_DT_GET(DT_ALIAS(wifibt_serial));
		m_uart_cfg_tbl[uart_app_id].au_map->dev = dev;
		m_uart_cfg_tbl[uart_app_id].au_map->app_cb = app_cb;

		/* get device label for wifi bt uart interface */
//		dev_name = m_uart_cfg_tbl[UART_M2M_APP_ID_WIFI_BT].au_map->dev_name; //ACPU_C201_MOD_NAME_WIFI_BT_IF;
		/* get the device instance for wifi bt uart interface */
//		dev = device_get_binding(dev_name);

		/* do uart settings */
//		uart_cfg.baudrate = DT_PROP(DT_NODELABEL(usart3), current_speed);
		uart_cfg.baudrate = DT_PROP(DT_ALIAS(wifibt_serial), current_speed);
//		if (DT_PROP(DT_NODELABEL(usart3), hw_flow_control))	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
		if (DT_PROP(DT_ALIAS(wifibt_serial), hw_flow_control))	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
		else												uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
		uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
		uart_cfg.parity = UART_CFG_PARITY_NONE;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
	}
		break;
#if (UART_M2M_APP_NUM > 1)
	case UART_COM_MOTOR_DRV:
	{
		dev = DEVICE_DT_GET(DT_ALIAS(bldc_drv_serial));
		m_uart_cfg_tbl[uart_app_id].au_map->dev = dev;
		m_uart_cfg_tbl[uart_app_id].au_map->app_cb = app_cb;

		uart_cfg.baudrate = DT_PROP(DT_ALIAS(bldc_drv_serial), current_speed);
		if (DT_PROP(DT_ALIAS(bldc_drv_serial), hw_flow_control))	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
		else												uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
		uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
		uart_cfg.parity = UART_CFG_PARITY_NONE;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
	}
		break;
#endif
/*
	case UART_COM_LTE_MODEM:
		break;
*/
	default:
		dev = NULL;
		ret = -1;
		break;
	}

	if (dev != NULL) {
		/* configure uart */
		if (uart_configure(dev, &uart_cfg) != 0) {
			return -EINVAL;
		}

		/* disable tx rx interrupts */
		uart_irq_rx_disable(dev);
		uart_irq_tx_disable(dev);

		/* set the irq handler function for wifi bt uart interface */
		uart_irq_callback_user_data_set(dev, uart_cb_handler, &m_uart_cfg_tbl[uart_app_id]);

		/*  drain rx fifo */
		while (uart_fifo_read(dev, &c, 1)) {
			continue;
		}

		/* init work queue */
		k_work_init(&m_uart_cfg_tbl[uart_app_id].m2m_work, uart_cb_work_handler);

		/* initialize semaphore used to copy rx data back to the application queue */
		k_sem_init(&m_uart_cfg_tbl[uart_app_id].copy_sem, 1, 1);

		/* enable uart rx interrupts */
		uart_irq_rx_enable(dev);
	}

	return ret;
}

int app_uart_m2m_com_init() {
	int ret = 0;

	for (int i=0; i<UART_M2M_APP_ID_MAX; i++) {
		m_uart_cfg_tbl[i].au_map = &m_au_map[i];
		m_uart_cfg_tbl[i].tx_type = TRANSACT_SOF_DETECT;
		m_uart_cfg_tbl[i].bulk_sz = 0;
		memset(m_uart_cfg_tbl[i].uart_buf, 0, sizeof(m_uart_cfg_tbl[i].uart_buf));
#if RING_BUF_TEST
		ring_buf_init(&m_uart_cfg_tbl[i].rb, RING_BUF_BYTES, m_uart_cfg_tbl[i].rbuffer);
#endif
	}

	/* initialize the m2m applications */
#if (UART_M2M_APP_NUM > 0)
	ret = app_wifi_bt_init();
	if (ret < 0)	return ret;
#endif

#if (UART_M2M_APP_NUM > 1)
#if (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102)
	ret = app_bldc_drv_init();
	if (ret < 0)	return ret;
#endif	/* (CONFIG_BLOWER_MOTOR_A101 || CONFIG_BLOWER_MOTOR_A102) */
#endif	/* (UART_M2M_APP_NUM > 1) */

	return ret;
}
