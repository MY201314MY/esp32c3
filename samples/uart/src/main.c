/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <drivers/modem/tc_modem.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
	
	LOG_INF("Hello World! %s\n", CONFIG_BOARD_TARGET);
	
	return 0;
}

int _example_modem_operation(const struct shell *sh, size_t argc, char *argv[])
{
	if(argc != 2)
	{
		return 0;
	}else
	{
		if(!strcmp(argv[1], "0"))
		{
			LOG_INF("operation 0");
			tc_modem_set_power(1, K_SECONDS(10));
		}
		else if(!strcmp(argv[1], "1"))
		{
			LOG_INF("operation 1");
			tc_modem_transparent_transmit("hello", strlen("hello"));
		}
	}
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tc_modem_commands,
	SHELL_CMD(operation, NULL,
		"example for modem operation",
		_example_modem_operation),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &tc_modem_commands,
		   "example for uart-test", NULL);