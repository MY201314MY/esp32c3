/*
 * Copyright (c) 2021 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(et6416, LOG_LEVEL_DBG);
#define DT_DRV_COMPAT             tc_et6416

/* Register definitions */
/* Register definitions */
#define REG_INPUT_VALUE               0x00
#define REG_OUTPUT                    0x02
#define REG_POLARITY                  0x04
#define REG_DIRECTION                 0x06

#define SUPPORTED_FLAGS (GPIO_INPUT | GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW |\
			GPIO_OUTPUT_INIT_HIGH | GPIO_ACTIVE_HIGH | GPIO_ACTIVE_LOW)

/** Configuration data*/
struct gpio_et6416_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	/** Controller I2C DT specification */
	struct i2c_dt_spec i2c;
};

/** Runtime driver data */
struct gpio_et6416_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	struct {
		uint16_t input;
		uint16_t output;
		uint16_t dir;
	} reg_cache;

	struct k_sem lock;
};

/**
 * @brief Read the port of certain register function.
 *
 * @param dev Device struct of the ET6416.
 * @param reg Register to read.
 * @param cache Pointer to the cache to be updated after successful read.
 *
 * @return 0 if successful, failed otherwise.
 */
static int read_port_regs(const struct device *dev, uint8_t reg, uint16_t *cache)
{
	const struct gpio_et6416_config *const config = dev->config;
	uint8_t port_data;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &port_data);
	if (ret != 0) {
		LOG_ERR("Error reading register 0x%02X (%d)", reg, ret);
		return ret;
	}

	*cache = port_data;
	ret = i2c_reg_read_byte_dt(&config->i2c, reg+1, &port_data);
	if (ret != 0) {
		LOG_ERR("Error reading register 0x02%X (%d)", reg, ret);
		return ret;
	}

	*cache = (((uint16_t)port_data)<<8) | ((*cache)&0xFF);
	LOG_DBG("Read: REG[0x%X] = 0x%04X", reg, *cache);

	return ret;
}

/**
 *  @brief Write to the port registers of certain register function.
 *
 * @param dev Device struct of the ET6416.
 * @param reg Register to write into. Possible values:  REG_DEVICE_ID_CTRL,
 * REG_OUTPUT, REG_DIRECTION, REG_PUD_SEL, REG_PUD_EN, REG_OUTPUT_HIGH_Z and
 * REG_INPUT_DEFAULT.
 * @param cache Pointer to the cache to be updated after successful write.
 * @param value New value to set.
 *
 * @return 0 if successful, failed otherwise.
 */
static int write_port_regs(const struct device *dev, uint8_t reg,
			   uint16_t *cache, uint16_t value)
{
	const struct gpio_et6416_config *const config = dev->config;
	uint16_t port_data;
	int ret = 0;

	
	port_data = ((value>>0) & 0x00FF);
	ret = i2c_reg_write_byte_dt(&config->i2c, reg, port_data);
	if (ret != 0) {
		LOG_ERR("error writing to register 0x%X (%d)",
			reg, ret);
		return ret;
	}

	port_data = ((value>>8) & 0x00FF);
	ret = i2c_reg_write_byte_dt(&config->i2c, reg+1, port_data);
	if (ret != 0) {
		LOG_ERR("error writing to register 0x%X (%d)",
			reg, ret);
		return ret;
	}
	*cache = value;
	LOG_DBG("Write: REG[0x%X] = 0x%X", reg, *cache);
	

	return ret;
}

static inline int update_input_regs(const struct device *dev, uint16_t *buf)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	int ret = read_port_regs(dev, REG_INPUT_VALUE,
				 &drv_data->reg_cache.input);
	*buf = drv_data->reg_cache.input;

	return ret;
}

static inline int update_output_regs(const struct device *dev, uint16_t value)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;

	LOG_DBG("value:0x%04X", value);

	return write_port_regs(dev, REG_OUTPUT,
			&drv_data->reg_cache.output, value);
}

static inline int update_direction_regs(const struct device *dev, uint16_t value)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;

	LOG_DBG("value:0x%04X", value);

	return write_port_regs(dev, REG_DIRECTION,
			&drv_data->reg_cache.dir, value);
}

static int setup_pin_dir(const struct device *dev, uint32_t pin, int flags)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	uint16_t reg_dir = drv_data->reg_cache.dir;
	uint16_t reg_out = drv_data->reg_cache.output;
	int ret;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	/* Update the driver data to the actual situation of the ET6416 */
	if (flags & GPIO_OUTPUT) {
		reg_dir &= ~BIT(pin);
	} else if (flags & GPIO_INPUT) {
		reg_dir |= BIT(pin);
	} else {
		reg_dir |= BIT(pin);
	}

	ret = update_output_regs(dev, reg_out);
	if (ret != 0) {
		return ret;
	}
	ret = update_direction_regs(dev, reg_dir);
	return ret;
}

static int gpio_et6416_pin_config(const struct device *dev, gpio_pin_t pin,
				   gpio_flags_t flags)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	int ret;

	/* Check if supported flag is set */
	if ((flags & ~SUPPORTED_FLAGS) != 0) {
		return -ENOTSUP;
	}

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}
	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = setup_pin_dir(dev, pin, flags);
	if (ret != 0) {
		LOG_ERR("error setting pin direction (%d)", ret);
		goto done;
	}

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_et6416_port_get_raw(const struct device *dev, uint32_t *value)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	uint16_t buf = 0;
	int ret = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = update_input_regs(dev, &buf);
	if (ret != 0) {
		goto done;
	}
	*value = buf;

done:
	k_sem_give(&drv_data->lock);
	return ret;
}

static int gpio_et6416_port_set_masked_raw(const struct device *dev,
						uint32_t mask, uint32_t value)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	uint16_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out = (reg_out & ~mask) | (mask & value);

	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

static int gpio_et6416_port_set_bits_raw(const struct device *dev,
					  uint32_t mask)
{
	return gpio_et6416_port_set_masked_raw(dev, mask, mask);
}

static int gpio_et6416_port_clear_bits_raw(const struct device *dev,
						uint32_t mask)
{
	return gpio_et6416_port_set_masked_raw(dev, mask, 0);
}

static int gpio_et6416_port_toggle_bits(const struct device *dev,
					 uint32_t mask)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	uint16_t reg_out;
	int ret;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_out = drv_data->reg_cache.output;
	reg_out ^= mask;
	ret = update_output_regs(dev, reg_out);

	k_sem_give(&drv_data->lock);

	return ret;
}

int gpio_et6416_init(const struct device *dev)
{
	struct gpio_et6416_drv_data *const drv_data =
		(struct gpio_et6416_drv_data *const)dev->data;
	const struct gpio_et6416_config *const config = dev->config;
	uint16_t reg_dir;
	int ret;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("%s is not ready", config->i2c.bus->name);
		return -ENODEV;
	}

	k_sem_init(&drv_data->lock, 1, 1);

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	reg_dir = drv_data->reg_cache.dir;
	ret = update_direction_regs(dev, reg_dir);

	k_sem_give(&drv_data->lock);

	return 0;
}

static const struct gpio_driver_api gpio_fxl_driver = {
	.pin_configure = gpio_et6416_pin_config,
	.port_get_raw = gpio_et6416_port_get_raw,
	.port_set_masked_raw = gpio_et6416_port_set_masked_raw,
	.port_set_bits_raw = gpio_et6416_port_set_bits_raw,
	.port_clear_bits_raw = gpio_et6416_port_clear_bits_raw,
	.port_toggle_bits = gpio_et6416_port_toggle_bits,
};

#define GPIO_ET6416_DEVICE_INSTANCE(inst)                                     \
	static const struct gpio_et6416_config gpio_et6416_##inst##_cfg = {  \
		.common = {                                                    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),\
		},                                                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst)                              \
	};                                                                     \
\
	static struct gpio_et6416_drv_data gpio_et6416_##inst##_drvdata = {  \
		.reg_cache = {                                                 \
			.input = 0x0,                                          \
			.output = 0xFFFF,                                        \
			.dir = 0xFFFF,                                            \
		}                                                              \
	};                                                                     \
\
	DEVICE_DT_INST_DEFINE(inst, gpio_et6416_init, NULL,                   \
		&gpio_et6416_##inst##_drvdata,                                \
		&gpio_et6416_##inst##_cfg, POST_KERNEL,                       \
		CONFIG_GPIO_ET6416_INIT_PRIORITY,                             \
		&gpio_fxl_driver);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ET6416_DEVICE_INSTANCE)
