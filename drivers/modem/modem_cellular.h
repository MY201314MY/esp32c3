#ifndef ZEPHYR_DRIVERS_MODEM_MODEM_CELLULAR_H_
#define ZEPHYR_DRIVERS_MODEM_MODEM_CELLULAR_H_

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/cellular/cellular.h>
#include <zephyr/cellular/chat.h>
#include <zephyr/cellular/cmux.h>
#include <zephyr/cellular/pipe.h>
#include <zephyr/cellular/pipelink.h>
#include <zephyr/cellular/ppp.h>
#include <zephyr/cellular/backend/uart.h>
#include <zephyr/net/ppp.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/atomic.h>

#define MODEM_CELLULAR_PERIODIC_SCRIPT_TIMEOUT \
	K_SECONDS(CONFIG_MODEM_CELLULAR_PERIODIC_SCRIPT_SECONDS)

#define MODEM_CELLULAR_DATA_IMEI_LEN         (16)
#define MODEM_CELLULAR_DATA_MODEL_ID_LEN     (65)
#define MODEM_CELLULAR_DATA_IMSI_LEN         (23)
#define MODEM_CELLULAR_DATA_ICCID_LEN        (22)
#define MODEM_CELLULAR_DATA_MANUFACTURER_LEN (65)
#define MODEM_CELLULAR_DATA_FW_VERSION_LEN   (65)

#define MODEM_CELLULAR_RESERVED_DLCIS        (2)

/* Magic constants */
#define CSQ_RSSI_UNKNOWN		     (99)
#define CESQ_RSRP_UNKNOWN		     (255)
#define CESQ_RSRQ_UNKNOWN		     (255)

/* Magic numbers to units conversions */
#define CSQ_RSSI_TO_DB(v) (-113 + (2 * (rssi)))
#define CESQ_RSRP_TO_DB(v) (-140 + (v))
#define CESQ_RSRQ_TO_DB(v) (-20 + ((v) / 2))

struct modem_cellular_data {
	/* UART backend */
	struct modem_pipe *uart_pipe;
	struct modem_backend_uart uart_backend;
	uint8_t uart_backend_receive_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];
	uint8_t uart_backend_transmit_buf[CONFIG_MODEM_CELLULAR_UART_BUFFER_SIZES];

	/* CMUX */
	struct modem_cmux cmux;
	uint8_t cmux_receive_buf[CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];
	uint8_t cmux_transmit_buf[2 * CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];

	struct modem_cmux_dlci dlci1;
	struct modem_cmux_dlci dlci2;
	struct modem_pipe *dlci1_pipe;
	struct modem_pipe *dlci2_pipe;
	uint8_t dlci1_receive_buf[CONFIG_MODEM_CELLULAR_CMUX_MAX_FRAME_SIZE];
	/* DLCI 2 is only used for chat scripts. */
	uint8_t dlci2_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZES];

	/* Modem chat */
	struct modem_chat chat;
	uint8_t chat_receive_buf[CONFIG_MODEM_CELLULAR_CHAT_BUFFER_SIZES];
	uint8_t *chat_delimiter;
	uint8_t *chat_filter;
	uint8_t *chat_argv[32];

	/* Status */
	enum cellular_registration_status registration_status_gsm;
	enum cellular_registration_status registration_status_gprs;
	enum cellular_registration_status registration_status_lte;
	uint8_t rssi;
	uint8_t rsrp;
	uint8_t rsrq;
	uint8_t imei[MODEM_CELLULAR_DATA_IMEI_LEN];
	uint8_t model_id[MODEM_CELLULAR_DATA_MODEL_ID_LEN];
	uint8_t imsi[MODEM_CELLULAR_DATA_IMSI_LEN];
	uint8_t manufacturer[MODEM_CELLULAR_DATA_MANUFACTURER_LEN];
	uint8_t fw_version[MODEM_CELLULAR_DATA_FW_VERSION_LEN];

	/* PPP */
	struct modem_ppp *ppp;

	enum modem_cellular_state state;
	const struct device *dev;
	struct k_work_delayable timeout_work;

	/* Power management */
	struct k_sem suspended_sem;
	struct {
        /* AT Command Operate */
	    struct k_sem sem;
		struct k_sem sem_user_config;
        enum modem_chat_script_result result;
    } operate;

	/* Event dispatcher */
	struct k_work event_dispatch_work;
	uint8_t event_buf[8];
	struct ring_buf event_rb;
	struct k_mutex event_rb_lock;
};


struct modem_cellular_config {
	const struct device *uart;
	struct gpio_dt_spec power_gpio;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec extern_power_gpio;
	uint16_t power_pulse_duration_ms;
	uint16_t reset_pulse_duration_ms;
	uint16_t startup_time_ms;
	uint16_t shutdown_time_ms;
};

#endif