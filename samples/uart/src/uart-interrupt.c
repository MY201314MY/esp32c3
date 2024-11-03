#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart, LOG_LEVEL_DBG);

static uint8_t uart_rxbuffer[1024] = {0};
static struct ring_buf rx_rb;

static uint8_t uart_txbuffer[1024] = {0};
static struct ring_buf tx_rb;

const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));

static void cb_handler_rx(const struct device *dev)
{
	uint8_t rxbuffer[32] = {0};

	int size = uart_fifo_read(dev, rxbuffer, sizeof(rxbuffer));
	if(size<=0)
	{
		LOG_ERR("failed to read uart: %d", size);
		return;
	}

	ring_buf_put(&rx_rb, rxbuffer, size);

	LOG_HEXDUMP_DBG(rxbuffer, size, "RX");
}

static void cb_handler_tx(const struct device *dev)
{
	uint8_t txbuffer[32] = {0};

	uint32_t cnt = ring_buf_get(&tx_rb, txbuffer, sizeof(txbuffer));

	if(cnt>0)
	{
		uart_fifo_fill(dev, txbuffer, cnt);
	}
	else if(uart_irq_tx_complete(dev))
	{
		uart_irq_tx_disable(dev);
	}
}

static void uart_cb_handler(const struct device *dev, void *user_data)
{
	if (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

		if (uart_irq_rx_ready(dev)) {
			cb_handler_rx(dev);
		}

		if (uart_irq_tx_ready(dev)) {
			cb_handler_tx(dev);
		}
	}
}

static void tc_uart_rx_thread(void *p1, void *p2, void *p3)
{
	ring_buf_init(&rx_rb, sizeof(uart_rxbuffer), uart_rxbuffer);
	ring_buf_init(&tx_rb, sizeof(uart_txbuffer), uart_txbuffer);

	uart_irq_callback_user_data_set(uart0, uart_cb_handler, NULL);
    uart_irq_rx_enable(uart0);
	uart_irq_tx_disable(uart0);

	while(1)
    {
        k_sleep(K_MSEC(1000));
    }
}

static int tc_uart_tx(const struct device *uart, char *txbuffer, size_t size)
{
	ring_buf_put(&tx_rb, txbuffer, size);
	uart_irq_tx_enable(uart);

	return 0;
}

K_THREAD_DEFINE(uart_fifo, 4096, tc_uart_rx_thread, NULL, NULL, NULL, 99, 0, 1);

int _example_uart_tx(const struct shell *sh, size_t argc, char *argv[])
{
	if(argc != 2)
	{
		LOG_ERR("argument error!");
		return -EINVAL;
	}

	tc_uart_tx(uart0, argv[1], strlen(argv[1]));
	
	return 0;
}

int _example_uart_rx(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rxbuffer[1024]={0};

	uint32_t cnt = ring_buf_get(&rx_rb, rxbuffer, sizeof(rxbuffer));

	LOG_INF("rx size : %d", cnt);

	LOG_HEXDUMP_DBG(rxbuffer, cnt, "RX");
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tc_uart_commands,
	SHELL_CMD(tx, NULL,
		"example uart tx",
		_example_uart_tx),
	SHELL_CMD(rx, NULL,
		"example uart rx",
		_example_uart_rx),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(uart, &tc_uart_commands,
		   "example for uart-test", NULL);