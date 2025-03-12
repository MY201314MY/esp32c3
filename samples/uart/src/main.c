/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <stdlib.h>


#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);


static const struct device *const wdt = DEVICE_DT_GET(DT_ALIAS(watchdog0));
static int wdt_channel_id;
#define WDT_MAX_WINDOW  8000U
#define WDT_MIN_WINDOW  0U


static void wdt_callback(const struct device *wdt_dev, int channel_id)
{
	printk("watchdog channel %d, ready to reset...\n", channel_id);
}

int tc_watch_dog_init(void)
{
	int ret;

	if (!device_is_ready(wdt)) {
		printk("%s: device not ready.\n", wdt->name);
		return -ENODEV;
	}

	struct wdt_timeout_cfg wdt_config = {
		/* Reset SoC when watchdog timer expires. */
		.flags = WDT_FLAG_RESET_SOC,

		/* Expire watchdog after max window */
		.window.min = WDT_MIN_WINDOW,
		.window.max = WDT_MAX_WINDOW,
	};

	/* Set up watchdog callback. */
	wdt_config.callback = wdt_callback;

	wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	if (wdt_channel_id == -ENOTSUP) {
		/* IWDG driver for STM32 doesn't support callback */
		printk("Callback support rejected, continuing anyway\n");
		wdt_config.callback = NULL;
		wdt_channel_id = wdt_install_timeout(wdt, &wdt_config);
	}
	if (wdt_channel_id < 0) {
		printk("Watchdog install error\n");
		return 0;
	}

	ret = wdt_setup(wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (ret < 0) {
		printk("Watchdog setup error\n");
		return 0;
	}

	return 0;
}

void tc_watch_dog_feed()
{
	wdt_feed(wdt, wdt_channel_id);
}

int main(void)
{
	k_sleep(K_SECONDS(2));
	tc_watch_dog_init();
	
	LOG_INF("Hello World! %s\n", CONFIG_BOARD_TARGET);

	while(1)
	{
		tc_watch_dog_feed();
		k_sleep(K_MSEC(1000));
	}

	return 0;
}

static uint8_t name[128];

int _example_modem_operation(const struct shell *sh, size_t argc, char *argv[])
{
	int operation = atoi(argv[1]);
	LOG_ERR("operation : %d", operation);
	if(1 == operation)
	{
/*
	[00:00:06.313,000] <err> main: operation : 1
	[00:00:06.316,000] <err> os: 
	[00:00:06.317,000] <err> os:  mcause: 7, Store/AMO access fault
	[00:00:06.319,000] <err> os:   mtval: 0
	[00:00:06.321,000] <err> os:      a0: 00000000    t0: 00000009
	[00:00:06.323,000] <err> os:      a1: 0000000e    t1: 40383a0e
	[00:00:06.325,000] <err> os:      a2: 00000000    t2: 00000009
	[00:00:06.327,000] <err> os:      a3: 3c0134c4    t3: 0000002a
	[00:00:06.330,000] <err> os:      a4: 00000001    t4: 0000002e
	[00:00:06.332,000] <err> os:      a5: 00000001    t5: 0000007f
	[00:00:06.334,000] <err> os:      a6: 00000068    t6: 00000010
	[00:00:06.337,000] <err> os:      a7: 0000006a
	[00:00:06.338,000] <err> os:      sp: 3fc8cc70
	[00:00:06.340,000] <err> os:      ra: 4200005e
	[00:00:06.342,000] <err> os:    mepc: 42000064
	[00:00:06.344,000] <err> os: mstatus: 00001880
	[00:00:06.346,000] <err> os: 
	[00:00:06.347,000] <err> os: call trace:
	[00:00:06.349,000] <err> os:       0: sp: 3fc8cc70 ra: 42000064
	[00:00:06.351,000] <err> os:       1: sp: 3fc8cc80 ra: 4200291c
	[00:00:06.354,000] <err> os:       2: sp: 3fc8ccc8 ra: 42000024
	[00:00:06.356,000] <err> os:       3: sp: 3fc8cd54 ra: 42003000
	[00:00:06.359,000] <err> os:       4: sp: 3fc8cd60 ra: 42002b3e
	[00:00:06.361,000] <err> os:       5: sp: 3fc8cd90 ra: 420036d4
	[00:00:06.364,000] <err> os:       6: sp: 3fc8cda0 ra: 42003f2a
	[00:00:06.366,000] <err> os:       7: sp: 3fc8ce40 ra: 42002000
	[00:00:06.369,000] <err> os: 
	[00:00:06.370,000] <err> os: >>> ZEPHYR FATAL ERROR 0: CPU exception on CPU 0
	[00:00:06.373,000] <err> os: Current thread: 0x3fc8f7b8 (shell_uart)
	[00:00:06.375,000] <err> os: Halting system
*/
		*((uint32_t *)NULL) = 1;
	}else if(2 == operation)
	{
		/* block */
		memset(name, 0, sizeof(name));
		LOG_WRN("memset to zero.");
	}

	
	return 0;
}

int _example_reboot(const struct shell *sh, size_t argc, char *argv[])
{
	LOG_WRN("REBOOT");
	k_sleep(K_SECONDS(2));
	sys_reboot(SYS_REBOOT_COLD);
	
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tc_modem_commands,
	SHELL_CMD(operation, NULL,
		"example for modem operation",
		_example_modem_operation),
	SHELL_CMD(reboot, NULL,
		"example reboot",
		_example_reboot),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(modem, &tc_modem_commands,
		   "example for uart-test", NULL);