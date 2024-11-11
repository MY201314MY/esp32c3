/*Copyright (C) Telit Communications S.p.A. Italy - All Rights Reserved.*/
/*    See LICENSE file in the project root for full license information.     */

#ifndef __INCLUDE_IOTS_DRIVERS_MEX10G1_H__
#define __INCLUDE_IOTS_DRIVERS_MEX10G1_H__

#include <zephyr/kernel.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/ring_buffer.h>

/* Global declarations ------------------------------------------------------ */
typedef void ( *modem_transparent_callback )( struct ring_buf *ring );
/* Global typedefs ---------------------------------------------------------- */
struct tc_modem_data
{
  const struct device *dev;

  struct k_mutex rx_mutex;
  struct k_mutex tx_mutex;

  /* UART backend */
  struct ring_buf rx_rb;
  uint8_t rxbuffer[512];
  struct ring_buf tx_rb;
  uint8_t txbuffer[512];

  struct
  {
    bool flag;
    uint8_t rxbuffer[2048];
    struct ring_buf ring;
    struct k_work work;
    modem_transparent_callback callback;
  } transparent;
};

struct tc_modem_config
{
  const struct device *uart;
  const struct gpio_dt_spec power_gpio;
  const struct gpio_dt_spec extern_power_gpio;
  const uint16_t power_pulse_duration_ms;
  const uint16_t startup_time_ms;
  const uint16_t shutdown_time_ms;
};

/* Global functions --------------------------------------------------------- */
/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem power on/off.

  @param[in] power
    1 power on
    0 power off

  @param[in] timeout
    timeout

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_set_power( int power, k_timeout_t timeout );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    get modem power status.

  @param[out] power
    the pointer to get power status.

  @param[in] timeout
    timeout

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_get_power( int *power, k_timeout_t timeout );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem receive.

  @param[out] rxbuffer
    the address of buffer to read
  @param[in] size
    the size of buffer to read
  @param[in] timeout
    timeout

  @return
    >0 the length has been read.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_receive( uint8_t *rxbuffer, size_t size, k_timeout_t timeout );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem transmit.

  @param[in] txbuffer
    the address of buffer to send
  @param[in] size
    the size of buffer to send
  @param[in] timeout
    timeout

  @return
    >0 the length has been written.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_transmit( uint8_t *txbuffer, size_t size, k_timeout_t timeout );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    reset the receive ringbuf of modem.

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/

int tc_modem_reset_receive_buffer(void);

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem transparent init.

  @param[in] transparent_handler
    the callback once receive something from modem.

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_transparent_init( modem_transparent_callback callback );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem transparent falg set.

  @param[in] flag
    true enable transparent
    false disable transparent

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/

int tc_modem_set_transparent( bool flag );

/*-----------------------------------------------------------------------------------------------*/
/*!
  @brief
    modem transparent transmit.

  @param[in] txbuffer
    the address of buffer to send
  @param[in] size
    the size of buffer to send

  @return
    0 Success.
    see zephyr error numbers.
*/
/*-----------------------------------------------------------------------------------------------*/
int tc_modem_transparent_transmit( uint8_t *txbuffer, size_t size );

int tc_modem_run_at_command( uint8_t *command, uint8_t *response, size_t size, k_timeout_t timeout );

#endif /* __INCLUDE_IOTS_DRIVERS_MEX10G1_H__ */