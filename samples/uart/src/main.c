/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
	
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);
	
	return 0;
}

static void hello_world_thread_entry(void *p1, void *p2, void *p3)
{
	while(1)
    {
        k_sleep(K_MSEC(1000));
		//LOG_ERR("hello");
    }
}

K_THREAD_DEFINE(hello_world_tid, 4096, hello_world_thread_entry, NULL, NULL, NULL, 99, 0, 1);

static int _example_thread_suspend(const struct shell *sh, size_t argc, char *argv[])
{
	k_thread_suspend(hello_world_tid);

	LOG_INF("suspend.");
	
	return 0;
}

static int _example_thread_resume(const struct shell *sh, size_t argc, char *argv[])
{
	LOG_INF("resume.");
	k_thread_resume(hello_world_tid);
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(thread_commands,
	SHELL_CMD(suspend, NULL,
		"example thread suspend",
		_example_thread_suspend),
	SHELL_CMD(resume, NULL,
		"example thread resume",
		_example_thread_resume),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(thread, &thread_commands,
		   "example for thread-test", NULL);