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
    uint8_t rxbuffer[256];
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

int tc_modem_transparent_transmit( uint8_t *txbuffer, size_t size );

#endif /* __INCLUDE_IOTS_DRIVERS_MEX10G1_H__ */