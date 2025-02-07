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

LOG_MODULE_REGISTER(async, LOG_LEVEL_INF);

#define BUF_SIZE 512
static K_MEM_SLAB_DEFINE(uart_slab, BUF_SIZE, 3, 4);

static uint8_t uart_rxbuffer[2048] = {0};
static struct ring_buf uart_rx_rb;

uint8_t txbuffer[2048]="9012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";

static void uart_async_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct device *uart = user_data;
	int ret;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("Tx sent %d bytes", evt->data.tx.len);
		break;

	case UART_TX_ABORTED:
		LOG_ERR("Tx aborted");
		break;

	case UART_RX_RDY:
		LOG_INF("Received data %d bytes", evt->data.rx.len);
		LOG_HEXDUMP_DBG(evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len, "recv");
		
		if(ring_buf_put(&uart_rx_rb, evt->data.rx.buf + evt->data.rx.offset, evt->data.rx.len) != evt->data.rx.len)
		{
			LOG_WRN("ring buffer overflow.");
		}
		
		break;

	case UART_RX_BUF_REQUEST:
	{
		uint8_t *buf;

		ret = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
		__ASSERT(ret == 0, "Failed to allocate slab");

		ret = uart_rx_buf_rsp(uart, buf, BUF_SIZE);
		__ASSERT(ret == 0, "Failed to provide new buffer");
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
#define UART_ASYNC_TRANSMIT_LENGTH_MAX 64 /* DON'T CHANGE. */

static int uart_async_transmit(const struct device *dev, uint8_t *buffer, size_t size)
{
	int ret;
    size_t offset = 0;
    size_t length = 0;

	if(dev == NULL || buffer == NULL)
	{
		return -EINVAL;
	}

	for(;;)
	{
		if(size - offset >= UART_ASYNC_TRANSMIT_LENGTH_MAX)
		{
			length = UART_ASYNC_TRANSMIT_LENGTH_MAX;
		}else{
			length = size-offset;
		}

		if(length>0)
		{
			LOG_INF("tx length : %d", length);
			ret = uart_tx(dev, txbuffer+offset, length, 10000);
			LOG_DBG("L:%d -- error:%d\r\n", __LINE__, ret);

			offset += length;
			if(offset>=size)
			{
				break;
			}
		}else{
			break;
		}
	}

	return offset;
}

uint8_t templete[2048] = {0};

int uart_async_init(void)
{
	uint32_t index = 0;
	uint8_t *buf;
	const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

	ring_buf_init(&uart_rx_rb, sizeof(uart_rxbuffer), uart_rxbuffer);
	
	int ret = k_mem_slab_alloc(&uart_slab, (void **)&buf, K_NO_WAIT);
	if(ret)
	{
		LOG_DBG("L:%d -- error:%d\r\n", __LINE__, ret);
	}

	ret = uart_callback_set(uart, uart_async_callback, (void *)uart);
	if(ret)
	{
		LOG_DBG("L:%d -- error:%d\r\n", __LINE__, ret);
	}

	ret = uart_rx_enable(uart, buf, BUF_SIZE, 10000);
	if(ret)
	{
		LOG_DBG("L:%d -- error:%d\r\n", __LINE__, ret);
	}
	k_sleep(K_SECONDS(3));
	LOG_INF("ASYNC TX-RX TEST.");
	while(1)
	{
		LOG_INF("test index : %d", index++);
		uart_async_transmit(uart, txbuffer, strlen(txbuffer));
		uint32_t count = ring_buf_get(&uart_rx_rb, templete, sizeof(templete));

		LOG_INF("count : %d", count);

		if(count!=strlen(txbuffer))
		{
			LOG_ERR("size not the same.");
			break;
		}

		if(strcmp(templete, txbuffer) != 0)
		{
			LOG_ERR("content not the same.");

			LOG_HEXDUMP_DBG(txbuffer, strlen(txbuffer), "TX");

			LOG_HEXDUMP_DBG(templete, strlen(templete), "RX");

			break;
		}

		k_sleep(K_SECONDS(5));
	}
	return 0;
}