/*
 * Copyright (c) 2024 Acme CPU
 *
 * Created on: 4-Jul-2023
 *     Author: Rohan Dey (rohan@acmecpu.com)
 */

#include <stdint.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(app_dfu);

#define UART_RX_BUFFER_SIZE		CONFIG_STM32BL_UART_RX_BUFFER_SIZE

struct rx_data {
	void *fifo_reserved;   	/* 1st word reserved for use by FIFO */
	uint8_t data;
};

struct stm32bl_uart_data {
	/* UAR device */
	const struct device *dev;

	/* apps rx fifo */
	struct k_fifo rx_fifo;

	/* variable to maintain the present size of the rx_fifo */
	uint32_t rx_fifo_size;
};

static struct stm32bl_uart_data m_sud;

static void uart_cb_handler(const struct device *dev, void *user_data)
{
	struct stm32bl_uart_data *sud = (struct stm32bl_uart_data *)user_data;

	while (uart_irq_update(sud->dev) && uart_irq_is_pending(sud->dev)) {
		if (uart_irq_rx_ready(sud->dev)) {
			uint8_t c;
			if (uart_fifo_read(sud->dev, &c, 1) != 1) {
				LOG_ERR("Failed to read UART");
//				return;
			}
			struct rx_data *rxd = (struct rx_data*)calloc(1, sizeof(struct rx_data));
			if (rxd != NULL) {
				sud->rx_fifo_size++;
				rxd->data = c;
				k_fifo_put(&sud->rx_fifo, rxd);
			}
		}
	}
}

void app_dfu_stm32bl_uart_ms_delay(uint32_t ms_delay)
{
	k_sleep(K_MSEC(ms_delay));
}

uint32_t app_dfu_stm32bl_uart_rx_fifo_size_get()
{
	return m_sud.rx_fifo_size;
}

int app_dfu_stm32bl_uart_write_bytes(const void *data, int len)
{
	const uint8_t *u8p;
	const struct device* dev = m_sud.dev;
	if (dev == NULL) {
		return ENXIO;
	}

	u8p = data;
	int bytes_sent = len;
	while (len--) {
		uart_poll_out(dev, *u8p++);
	}

	return bytes_sent;
}

int app_dfu_stm32bl_uart_read_bytes(void* buf, int length, int ms_to_wait)
{
	struct rx_data *rxd = NULL;
	struct stm32bl_uart_data *sud = &m_sud;
	int count = 0;
	if (buf == NULL)
		return -1;

	for (count = 0; count < length; count++) {
		if (ms_to_wait == 0xFFFFFFFF)
			rxd = k_fifo_get(&sud->rx_fifo, K_FOREVER);
		else
			rxd = k_fifo_get(&sud->rx_fifo, K_MSEC(ms_to_wait));
		memcpy((uint8_t *)buf + count, &rxd->data, 1);
		free(rxd);
		sud->rx_fifo_size--;
	}

	return count;
}



int app_dfu_stm32bl_uart_open(int baud_rate, int data_bits, int parity, int stop_bits,
		int flow_ctrl, int source_clk)
{
	UNUSED(baud_rate);
	UNUSED(data_bits);
	UNUSED(parity);
	UNUSED(stop_bits);
	UNUSED(flow_ctrl);
	UNUSED(source_clk);
	return 0;
}

int app_dfu_stm32bl_uart_close()
{
	return 0;
}

int app_dfu_stm32bl_uart_configure()
{
	int ret = 0;
	uint8_t c;
	struct uart_config uart_cfg;

	const struct device* dev = DEVICE_DT_GET(DT_ALIAS(bldc_drv_serial));

	if (dev != NULL) {
		/* disable tx rx interrupts */
		uart_irq_rx_disable(dev);
		uart_irq_tx_disable(dev);

		/* hardware configurations */
		uart_cfg.baudrate = CONFIG_STM32BL_UART_BAUD;
		uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
		uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
		uart_cfg.parity = UART_CFG_PARITY_EVEN;
		uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;

		m_sud.dev = dev;	// save the device instance

		/* configure uart */
		if (uart_configure(dev, &uart_cfg) != 0) {
			return -EINVAL;
		}

		/* set the irq handler function for uart interface */
		uart_irq_callback_user_data_set(dev, uart_cb_handler, &m_sud);

		/*  drain uart driver's rx fifo */
		while (uart_fifo_read(dev, &c, 1)) {
			continue;
		}

		/* Initialize app's rx fifo */
		k_fifo_init(&m_sud.rx_fifo);
		m_sud.rx_fifo_size = 0;

		/* enable uart rx interrupts */
		uart_irq_rx_enable(dev);
	}

	return ret;
}
