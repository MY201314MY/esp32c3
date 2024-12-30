#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(timer, LOG_LEVEL_DBG);

struct k_timer trip_upload_timer;

static void tc_uart_rx_thread(void *p1, void *p2, void *p3)
{

	while(1)
    {
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(uart_fifo, 4096, tc_uart_rx_thread, NULL, NULL, NULL, 99, 0, 1);

static void trip_timer_handler(struct k_timer *t_id)
{
	LOG_INF("callback");
}

int _example_timer_start(const struct shell *sh, size_t argc, char *argv[])
{
    static bool init = false;
    if(init == false)
    {
        init = true;
	    k_timer_init(&trip_upload_timer, trip_timer_handler, NULL);
        k_timer_start(&trip_upload_timer, K_NO_WAIT, K_SECONDS(5));

    }
	return 0;
}

int _example_timer_set(const struct shell *sh, size_t argc, char *argv[])
{
    
    
	return 0;
}

int _example_timer_stop(const struct shell *sh, size_t argc, char *argv[])
{
    k_timer_stop(&trip_upload_timer);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tc_timer_commands,
SHELL_CMD(start, NULL,
		"example timer start",
		_example_timer_start),
        SHELL_CMD(set, NULL,
		"example timer set",
		_example_timer_set),
	SHELL_CMD(stop, NULL,
		"example timer stop",
		_example_timer_stop),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(timer, &tc_timer_commands,
		   "example for uart-test", NULL);