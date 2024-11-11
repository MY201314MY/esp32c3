/*Copyright (C) Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

/* Include files ================================================================================*/

#include <drivers/modem/tc_modem.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER( cellular, LOG_LEVEL_DBG );

/* Local function prototypes ====================================================================*/
static int tc_modem_pm_action( const struct device *dev, enum pm_device_action action );
static void modem_cellular_transparent_handler( struct k_work *item );
static int modem_cellular_init( const struct device *dev );

static bool modem_cellular_gpio_enabled( const struct gpio_dt_spec *gpio );

static const struct device *modem = DEVICE_DT_GET( DT_ALIAS( modem ) );
static const struct device *testdev = NULL;
#if CONFIG_PM_DEVICE
static int tc_modem_pm_action( const struct device *dev, enum pm_device_action action )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )dev->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )dev->config;
  int ret = 0;

  switch( action )
  {
    case PM_DEVICE_ACTION_RESUME:

      if( modem_cellular_gpio_enabled( &config->extern_power_gpio ) )
      {
        gpio_pin_configure_dt( &config->extern_power_gpio, GPIO_OUTPUT_INACTIVE );
        k_sleep( K_SECONDS( 1 ) );
        gpio_pin_configure_dt( &config->extern_power_gpio, GPIO_OUTPUT_ACTIVE );
      }else{
        LOG_DBG( "modem extern power pin is not configured" );
      }

      if( modem_cellular_gpio_enabled( &config->power_gpio ) )
      {
        gpio_pin_configure_dt( &config->power_gpio, GPIO_OUTPUT_ACTIVE );
        k_sleep( K_SECONDS( 5 ) );
        gpio_pin_configure_dt( &config->power_gpio, GPIO_OUTPUT_INACTIVE );
        k_sleep( K_SECONDS( 5 ) );
      }else{
        LOG_DBG( "modem power pin is not configured" );
      }
      break;

    case PM_DEVICE_ACTION_SUSPEND:
      if( modem_cellular_gpio_enabled( &config->power_gpio ) )
      {
        gpio_pin_configure_dt( &config->power_gpio, GPIO_OUTPUT_ACTIVE );
        k_sleep( K_SECONDS( 5 ) );
        gpio_pin_configure_dt( &config->power_gpio, GPIO_OUTPUT_INACTIVE );
      }
      if( modem_cellular_gpio_enabled( &config->extern_power_gpio ) )
      {
        gpio_pin_configure_dt( &config->extern_power_gpio, GPIO_OUTPUT_INACTIVE );
        k_sleep( K_SECONDS( 1 ) );
      }

      break;

    default:
      ret = -ENOTSUP;
      break;
  }

  return ret;
}
#endif /* CONFIG_PM_DEVICE */

static void modem_cellular_transparent_handler( struct k_work *item )
{
  struct tc_modem_data *data =
    CONTAINER_OF( item, struct tc_modem_data, transparent.work );

  if( data->transparent.callback != NULL )
  {
    data->transparent.callback( &data->transparent.ring);
  }
}

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

  int cnt = -1;
  if( data->transparent.flag == true )
  {
    cnt = ring_buf_put(&data->transparent.ring, rxbuffer, size);
    k_work_submit( &data->transparent.work );
  }else{
    cnt = ring_buf_put(&data->rx_rb, rxbuffer, size);
  }

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

static bool modem_cellular_gpio_enabled( const struct gpio_dt_spec *gpio )
{
  return gpio->port != NULL;
}

static int modem_cellular_init( const struct device *dev )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )dev->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )dev->config;
  data->dev = dev;
  k_mutex_init( &data->rx_mutex );
  k_mutex_init( &data->tx_mutex );

  LOG_INF("addr: %p", config->uart);

  //modem = dev;
  testdev = dev;
  if(modem_cellular_gpio_enabled( &config->power_gpio ) )
  {
    gpio_pin_configure_dt( &config->power_gpio, GPIO_OUTPUT_INACTIVE );
  }
  if(modem_cellular_gpio_enabled( &config->extern_power_gpio ) )
  {
    gpio_pin_configure_dt( &config->extern_power_gpio, GPIO_OUTPUT_INACTIVE );
  }
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

int tc_modem_transparent_init( modem_transparent_callback callback )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  
  data->transparent.flag = false;
  data->transparent.callback = callback;
  return 0;
}


int tc_modem_set_transparent( bool flag )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;

  if( flag == true )
  {
    data->transparent.flag = true;
  }
  else
  {
    data->transparent.flag = false;
  }

  return 0;
}

int tc_modem_transparent_transmit( uint8_t *txbuffer, size_t size )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  struct tc_modem_config *config = ( struct tc_modem_config * )modem->config;
  struct tc_modem_config *tdev = ( struct tc_modem_config * )testdev->config;

  LOG_INF("maddr: %p", config->uart);
  LOG_INF("daddr: %p", tdev->uart);
  int ret = ring_buf_put(&data->tx_rb, txbuffer, size);
	uart_irq_tx_enable(config->uart);

  return ret;
}

int tc_modem_run_at_command( uint8_t *command, uint8_t *response, size_t size,
                          k_timeout_t timeout )
{
  /* if the response is NULL, the response will be ignored */
  if( command ==  NULL )
  {
    return -EINVAL;
  }
  else
  {
    ;
  }

  if( modem == NULL )
  {
    LOG_ERR( "NULL device" );
    return -1;
  }

  int64_t backup = k_uptime_get();
  uint8_t rxbuffer[512] = {0};
  size_t offset = 0;
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  enum pm_device_state state;
  int ret = pm_device_state_get( modem, &state );

  if( state != PM_DEVICE_STATE_ACTIVE )
  {
    return -ENXIO;
  }
  else
  {
    ;
  }

  if( k_mutex_lock( &data->rx_mutex, timeout ) == 0 )
  {
    ring_buf_reset( &data->rx_rb );
    memset( rxbuffer, 0, sizeof( rxbuffer ) );
    LOG_HEXDUMP_INF( command, strlen( command ), "TX" );
    ret = tc_modem_transmit( command, strlen( command ), timeout );

    while( 1 )
    {
      while( ring_buf_is_empty( &data->rx_rb ) == false )
      {
        ret = ring_buf_get( &data->rx_rb, rxbuffer + offset, sizeof( rxbuffer ) - offset );
        LOG_HEXDUMP_INF( rxbuffer + offset, ret, "RX" );
        offset += ret;

        if( strstr( rxbuffer, "\r\nOK\r\n" ) != NULL )
        {
          if( response != NULL )
          {
            if( ret < size )
            {
              memcpy( response, rxbuffer, ret );
            }
            else
            {
              memcpy( response, rxbuffer, size - 1 );
              response[ size - 1 ] = '\0';
            }
          }
          else
          {
            ;
          }

          k_mutex_unlock( &data->rx_mutex );
          return 0;
        }
        else
        {
          ;
        }

        if( strstr( rxbuffer, "\r\nERROR\r\n" ) != NULL )
        {
          k_mutex_unlock( &data->rx_mutex );
          return -EAGAIN;
        }
        else
        {
          ;
        }
      }

      if( ( k_uptime_get() - backup ) > ( timeout.ticks * 1000 ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC )
      {
        k_mutex_unlock( &data->rx_mutex );
        return -ETIMEDOUT;
      }
      else
      {
        ;
      }

      k_sleep( K_MSEC( 200 ) );
    }
  }
  else
  {
    return -EBUSY;
  }

  return 0;
}

int tc_modem_set_power( int power, k_timeout_t timeout )
{
  /*
    0 : power off
    1 : power on
  */
  if( ( power != 0 ) && ( power != 1 ) )
  {
    return -EINVAL;
  }
  else
  {
    ;
  }

  int ret = -1;
  ret = pm_device_action_run( modem,
                              power == 0 ? PM_DEVICE_ACTION_SUSPEND : PM_DEVICE_ACTION_RESUME );
  return ret;
}

int tc_modem_get_power( int *power, k_timeout_t timeout )
{
  enum pm_device_state state = PM_DEVICE_STATE_SUSPENDED;
  if( power == NULL )
  {
    return -EINVAL;
  }
  else
  {
    *power = 0;
  }


  int ret = pm_device_state_get( modem, &state );
  
  if( ret == 0 )
  {
    if( state == PM_DEVICE_STATE_ACTIVE )
    {
      *power = 1;
    }
    else{
      *power = 0;
    }
  }

  return ret;
}

int tc_modem_reset_receive_buffer( void )
{
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;
  ring_buf_reset( &data->rx_rb );
  return 0;
}

int tc_modem_transmit( uint8_t *txbuffer, size_t size, k_timeout_t timeout )
{
  if( ( txbuffer == NULL ) || ( timeout.ticks <= 0 ) )
  {
    return -EINVAL;
  }
  else
  {
    ;
  }

  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;

  enum pm_device_state state;

  int ret = pm_device_state_get( modem, &state );

  if( state != PM_DEVICE_STATE_ACTIVE )
  {
    return -ENXIO;
  }
  else
  {
    ;
  }

  if( k_mutex_lock( &data->tx_mutex, timeout ) == 0 )
  {
    int64_t backup = k_uptime_get();
    ret = 0;

    while( ret < size )
    {
      ret += tc_modem_transparent_transmit( txbuffer + ret, size - ret );

      if( ( k_uptime_get() - backup ) > ( timeout.ticks * 1000 ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC )
      {
        break;
      }
      else
      {
        ;
      }

      k_sleep( K_MSEC( 20 ) );
    }

    k_mutex_unlock( &data->tx_mutex );
  }
  else
  {
    ret = -EBUSY;
  }

  return ret;
}

int tc_modem_receive( uint8_t *rxbuffer, size_t size, k_timeout_t timeout )
{
  uint32_t ret = 0;
  size_t offset = 0;
  int64_t backup = k_uptime_get();
  struct tc_modem_data *data = ( struct tc_modem_data * )modem->data;

  if( ( rxbuffer == NULL ) || ( timeout.ticks <= 0 ) )
  {
    return -EINVAL;
  }
  else
  {
    ;
  }

  if( k_mutex_lock( &data->rx_mutex, timeout ) == 0 )
  {
    while( 1 )
    {
      while( ring_buf_is_empty( &data->rx_rb ) == false )
      {
        ret = ring_buf_get( &data->rx_rb, rxbuffer + offset, 1 );
        offset += 1;

        if( offset >= size )
        {
          k_mutex_unlock( &data->rx_mutex );
          return offset;
        }
        else
        {
          ;
        }

        if( strstr( rxbuffer, "\r\nOK\r\n" ) != NULL || strstr( rxbuffer, "\r\nERROR\r\n" ) != NULL )
        {
          k_mutex_unlock( &data->rx_mutex );
          return offset;
        }
        else
        {
          ;
        }
      }

      if( ( k_uptime_get() - backup ) > ( timeout.ticks * 1000 ) / CONFIG_SYS_CLOCK_TICKS_PER_SEC )
      {
        k_mutex_unlock( &data->rx_mutex );
        break;
      }
      else
      {
        ;
      }

      k_sleep( K_MSEC( 20 ) );
    }
  }
  else
  {
    return -EBUSY;
  }

  return offset;
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
  PM_DEVICE_DT_INST_DEFINE(inst, tc_modem_pm_action);                                  \
  \
  DEVICE_DT_INST_DEFINE(inst, modem_cellular_init, PM_DEVICE_DT_INST_GET(inst),              \
                        &MODEM_CELLULAR_INST_NAME(data, inst),                               \
                        &MODEM_CELLULAR_INST_NAME(config, inst), POST_KERNEL, 99, NULL);

#define DT_DRV_COMPAT telit_cellular_modem
DT_INST_FOREACH_STATUS_OKAY( MODEM_CELLULAR_DEVICE_TELIT_CELLULAR_MODEM )
#undef DT_DRV_COMPAT