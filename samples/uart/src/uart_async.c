/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/uart.h>

LOG_MODULE_REGISTER(async, LOG_LEVEL_DBG);

#define BUF_SIZE 2048
static K_MEM_SLAB_DEFINE(uart_slab, BUF_SIZE, 3, 4);

uint8_t txbuffer[2048]="123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
static void uart_async_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	struct device *uart = user_data;
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_INF("Tx sent %d bytes", evt->data.tx.len);
		break;

	case UART_TX_ABORTED:
		LOG_ERR("Tx aborted");
		break;

	case UART_RX_RDY:
		LOG_INF("Received data %d bytes", evt->data.rx.len);
		LOG_HEXDUMP_DBG(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len, "recv");
		break;

	case UART_RX_BUF_REQUEST:
	{
		uint8_t *buf;

		err = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
		__ASSERT(err == 0, "Failed to allocate slab");

		err = uart_rx_buf_rsp(uart, buf, BUF_SIZE);
		__ASSERT(err == 0, "Failed to provide new buffer");
		break;
	}

	case UART_RX_BUF_RELEASED:
		k_mem_slab_free(&uart_slab, (void *)evt->data.rx_buf.buf);
		break;

	case UART_RX_DISABLED:
		break;

	case UART_RX_STOPPED:
		break;
	}
}
#define ESP32_UART_TRANSMIT_LENGTH_MAX 128 /* DON'T CHANGE. */

static void async(const struct device *lpuart)
{
	int err;
	uint8_t *buf;
    uint32_t offset = 0;
    uint32_t length = 0;
    uint32_t remain = strlen(txbuffer);


	err = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
	LOG_DBG("L:%d -- error:%d\r\n", __LINE__, err);

	err = uart_callback_set(lpuart, uart_async_callback, (void *)lpuart);
	LOG_DBG("L:%d -- error:%d\r\n", __LINE__, err);

	err = uart_rx_enable(lpuart, buf, BUF_SIZE, 10000);
	LOG_DBG("L:%d -- error:%d\r\n", __LINE__, err);

	while (1) {
        offset = 0;
		for(;;)
        {
            if(remain - offset>=ESP32_UART_TRANSMIT_LENGTH_MAX)
            {
                length = ESP32_UART_TRANSMIT_LENGTH_MAX;
            }else{
                length = remain-offset;
            }
            LOG_INF("tx length : %d", length);
            err = uart_tx(lpuart, txbuffer+offset, length, 10000);
            LOG_DBG("L:%d -- error:%d\r\n", __LINE__, err);

            offset += length;
            if(offset>=remain)
            {
                break;
            }
        }

        k_sleep(K_MSEC(5000));
	}
}

int uart_async_init(void)
{
	const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

	async(uart);	

	return 0;
}