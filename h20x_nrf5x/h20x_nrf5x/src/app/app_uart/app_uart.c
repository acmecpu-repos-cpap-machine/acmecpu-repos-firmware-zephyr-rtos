/*
 * Copyright (c) 2022 Acme CPU
 *
 *  Created on: 7-Dec-2022
 *      Author: Rohan Dey (rohan@acmecpu.cpm)
 */

// #include <zephyr.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app_uart);

#include <string.h>

/* wifi modem pins*/
#define NET_RST_PIN		DT_GPIO_PIN(DT_NODELABEL(net_rst), gpios)
#define NET_RST_FLAGS	(GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(net_rst), gpios))
#define NET_WKUP_PIN		DT_GPIO_PIN(DT_NODELABEL(net_wake), gpios)
#define NET_WKUP_FLAGS	(GPIO_PUSH_PULL | DT_GPIO_FLAGS(DT_NODELABEL(net_wake), gpios))

#define MSG_SIZE 32
#define UART_DEVICE_NODE    DT_ALIAS(wifi_serial)
static const struct device *uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* queue to store up to 10 messages (aligned to 4-byte boundary) */
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

/* receive buffer used in UART ISR callback */
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

void serial_cb(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	while (uart_irq_rx_ready(uart_dev)) {

		uart_fifo_read(uart_dev, &c, 1);

		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			/* terminate string */
			rx_buf[rx_buf_pos] = '\0';

			/* if queue is full, message is silently dropped */
			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			/* reset the buffer (it was copied to the msgq) */
			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
		/* else: characters beyond buffer size are dropped */
	}
}

static int app_uart_configure()
{
    // const char *dev_name = DT_PROP(DT_ALIAS(wifibt_serial), label);
    const struct device* dev = uart_dev; //device_get_binding(dev_name);
    
    struct uart_config uart_cfg;
    uart_cfg.baudrate = DT_PROP(DT_ALIAS(wifi_serial), current_speed);
    if (DT_PROP(DT_ALIAS(wifi_serial), hw_flow_control))	uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	else												    uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
	uart_cfg.parity = UART_CFG_PARITY_NONE;
	uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;

	/* configure uart */
	if (uart_configure(dev, &uart_cfg) != 0) {
		return -EINVAL;
	}

	/* disable tx rx interrupts */
	uart_irq_rx_disable(dev);
	uart_irq_tx_disable(dev);

	/* set the irq handler function for wifi bt uart interface */
	uart_irq_callback_user_data_set(dev, serial_cb, NULL);

	/*  drain rx fifo */
    uint8_t c;
	while (uart_fifo_read(dev, &c, 1)) {
		continue;
	}

	/* init work queue */
	// k_work_init(&m2m_work, uart_cb_work_handler);

	/* initialize semaphore used to copy rx data back to the application queue */
	// k_sem_init(&m_uart_cfg_tbl[uart_app_id].copy_sem, 1, 1);

	/* enable uart rx interrupts */
	uart_irq_rx_enable(dev);

    return 0;
}

void app_uart_get_and_print()
{
    char tx_buf[MSG_SIZE];

	/* indefinitely wait for input from the user */
	while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        LOG_INF("%s", tx_buf);
		// print_uart("Echo: ");
		// print_uart(tx_buf);
		// print_uart("\r\n");
        const char *u8p = "Oi from nrf53";
        size_t len = strlen(u8p);
    	while (len--) {
		    uart_poll_out(uart_dev, *u8p++);
	    }
	}
}

int app_uart_m2m_com_init()
{
   	if (!device_is_ready(uart_dev)) {
		printk("UART device not found!");
		return -1;
	}

    app_uart_configure();

    /* reset the wifi modem */
	int ret = 0;
	const struct device *netrst_dev = DEVICE_DT_GET(DT_GPIO_CTLR(DT_NODELABEL(net_rst), gpios));
	if (netrst_dev == NULL) {
		// LOG_DBG("Device not found: %s", USB_DSEL_DEV_NAME);
		LOG_ERR("Device not found: %p", netrst_dev);
		return -1;
	}
	ret = gpio_pin_configure(netrst_dev, NET_RST_PIN, (GPIO_OUTPUT | NET_RST_FLAGS));
	if (ret < 0) {
		LOG_ERR("gpio_pin_configure failed");
		return ret;
	}
    gpio_pin_set(netrst_dev, NET_RST_PIN, 1);   /* active low pin, 1 = LOW, 0 = HIGH */
    k_sleep(K_MSEC(100));
    gpio_pin_set(netrst_dev, NET_RST_PIN, 0);   /* active low pin, 1 = LOW, 0 = HIGH */

    return 0;
}