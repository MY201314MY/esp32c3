/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define LED0 DT_ALIAS(led_3)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0,gpios);

int main(void)
{
	k_sleep(K_SECONDS(10));

	bool led_state = true;
	
	LOG_INF("Hello World! %s\n", CONFIG_BOARD_TARGET);

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		LOG_INF("LED state: %s\n", led_state ? "ON" : "OFF");
		k_sleep(K_SECONDS(5));
	}

	
	return 0;
}

int _example_modem_operation(const struct shell *sh, size_t argc, char *argv[])
{
	static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
	uint8_t value = 0;
	uint8_t addr = 0x20;
	int ret = 0;


	for(uint8_t i=0;i<8;i++)
	{
		ret = i2c_burst_read(dev, addr, i, &value, 1);
		LOG_INF("ret:%d -- reg:0x%02X -- value=0x%02X", ret, i, value);
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