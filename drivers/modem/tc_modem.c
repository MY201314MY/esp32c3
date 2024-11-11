/*Copyright (C) Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

/* Include files ================================================================================*/

#include <drivers/modem/tc_modem.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER( cellular, LOG_LEVEL_DBG );

/* Local function prototypes ====================================================================*/
static int modem_cellular_init( const struct device *dev );

static const struct device *modem = DEVICE_DT_GET( DT_ALIAS( modem ) );

static void cb_handler_rx(const struct device *dev)
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )modem->config;
	uint8_t rxbuffer[32] = {0};

	int size = uart_fifo_read(dev, rxbuffer, sizeof(rxbuffer));
	if(size<=0)
	{
		LOG_ERR("failed to read uart: %d", size);
		return;
	}

  int cnt = ring_buf_put(&data->rx_rb, rxbuffer, size);

  if( cnt != size )
  {
    LOG_WRN( "modem receive ring buffer overrun!" );
  }

	LOG_HEXDUMP_DBG(rxbuffer, size, "RX");
}

static void cb_handler_tx(const struct device *dev)
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )modem->config;
	uint8_t txbuffer[32] = {0};

	uint32_t cnt = ring_buf_get(&data->tx_rb, txbuffer, sizeof(txbuffer));

	if(cnt>0)
	{
		uart_fifo_fill(config->uart, txbuffer, cnt);
	}
	else if(uart_irq_tx_complete(config->uart))
	{
		uart_irq_tx_disable(config->uart);
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

static void modem_cellular_transparent_handler( struct k_work *item )
{
  struct tc_modem_data *data =
    CONTAINER_OF( item, struct tc_modem_data, transparent.work );

  if( data->transparent.callback != NULL )
  {
    data->transparent.callback( &data->transparent.ring);
  }
}

static int modem_cellular_init( const struct device *dev )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )dev->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )dev->config;
  data->dev = dev;
  k_mutex_init( &data->rx_mutex );
  k_mutex_init( &data->tx_mutex );

  LOG_INF("addr: %p", config->uart);

  LOG_DBG( "modem rx buffer size : %d bytes", ARRAY_SIZE( data->rxbuffer ) );
  LOG_DBG( "modem tx buffer size : %d bytes", ARRAY_SIZE( data->txbuffer ) );
  {
    ring_buf_init( &data->rx_rb, sizeof( data->rxbuffer ),
                   data->rxbuffer );

    ring_buf_init( &data->tx_rb, sizeof( data->txbuffer ),
                   data->txbuffer );
    
    ring_buf_init( &data->transparent.ring, sizeof( data->transparent.rxbuffer ),
                   data->transparent.rxbuffer );

    k_work_init( &data->transparent.work, modem_cellular_transparent_handler );
  }

  uart_irq_callback_user_data_set(config->uart, uart_cb_handler, NULL);
  uart_irq_rx_enable(config->uart);
	uart_irq_tx_disable(config->uart);

  pm_device_init_suspended( dev );
  return 0;
}

int tc_modem_transparent_transmit( uint8_t *txbuffer, size_t size )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )modem->config;

  int ret = ring_buf_put(&data->tx_rb, txbuffer, size);
	uart_irq_tx_enable(config->uart);

  LOG_INF("uart : %p", config->uart);

  return ret;
}

#define MODEM_CELLULAR_INST_NAME(name, inst) \
  _CONCAT(_CONCAT(_CONCAT(name, _), DT_DRV_COMPAT), inst)

#define MODEM_CELLULAR_DEVICE_TELIT_CELLULAR_MODEM(inst)                                                  \
  \
  static struct tc_modem_data MODEM_CELLULAR_INST_NAME(data, inst) = {                 \
};                                                                                          \
  \
  static struct tc_modem_config MODEM_CELLULAR_INST_NAME(config, inst) = {             \
              .uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
              .power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_power_gpios, {}),                 \
              .extern_power_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, mdm_extern_power_gpios, {}),                 \
              .power_pulse_duration_ms = 5000,                                                   \
              .startup_time_ms = 5000,                                                          \
              .shutdown_time_ms = 5000,                                                          \
};                                                                                         \
  \
  PM_DEVICE_DT_INST_DEFINE(inst, NULL);                                  \
  \
  DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
                        &MODEM_CELLULAR_INST_NAME(data, inst),                               \
                        &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99, NULL);

#define DT_DRV_COMPAT telit_cellular_modem
DT_INST_FOREACH_STATUS_OKAY( MODEM_CELLULAR_DEVICE_TELIT_CELLULAR_MODEM )
#undef DT_DRV_COMPAT