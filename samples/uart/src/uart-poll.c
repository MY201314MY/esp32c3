#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart, LOG_LEVEL_DBG);

static uint8_t rxbuffer[1024] = {0};
static struct ring_buf rx_rb;

const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));

static void tc_uart_rx_thread(void *p1, void *p2, void *p3)
{
	uint8_t ch = '\0';

	ring_buf_init(&rx_rb, sizeof(rxbuffer), rxbuffer);
    
	while(1)
    {
        while (!uart_poll_in(uart0, &ch)) {
		    ring_buf_put(&rx_rb, (uint8_t *)&ch, 1);
	    }
        k_sleep(K_MSEC(1));
    }
}

static int tc_uart_tx(const struct device *uart, char *txbuffer, size_t size)
{

	for (size_t i = 0; i < size; i++) {
		uart_poll_out(uart, txbuffer[i]);
	}

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