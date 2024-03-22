#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "memoree_platform.h"
#include "memoree.h"

#define MAX(x, y) ((x > y) ? x : y)

#define MEMOREE_PLATFORM_I2C_MAX_SPEED 1000000
#define MEMOREE_PLATFORM_SPI_MAX_SPEED 40000000

#define I2C_RW_READ 0x01   ///< I2C RW bit read mode
#define I2C_RW_WRITE 0x00  ///< I2C RW bit write mode
#define I2C_CHECK_ACK 0x01 ///< I2C controller will check ack from target
#define I2C_NO_ACK 0x00    ///< I2C controller will not check ack from target
#define I2C_ACK 0x00       ///< I2C ack value
#define I2C_NACK 0x01      ///< I2C nack value

void platform_ms_delay(uint32_t ms)
{
  vTaskDelay(pdMS_TO_TICKS(ms));
}

/// I2C functions
memoree_interface_t platform_i2c_init(memoree_i2c_conf_t *i2c_conf)
{
  if (!i2c_conf || i2c_conf->port >= SOC_I2C_NUM || i2c_conf->speed > MEMOREE_PLATFORM_I2C_MAX_SPEED)
    return NULL;

  i2c_config_t config = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = i2c_conf->sda_pin,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = i2c_conf->scl_pin,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = i2c_conf->speed,
      .clk_flags = 0,
  };

  int err = i2c_param_config(i2c_conf->port, &config);
  if (err != 0)
    return NULL;

  err = i2c_driver_install(i2c_conf->port, I2C_MODE_MASTER, 0, 0, 0);
  if (err != 0)
    return NULL;

  /// Adjust the I2C clock duty cycle so that it is closer to 50%
  err = i2c_set_period(i2c_conf->port, 250, 200);
  if (err != 0)
    return NULL;

  memoree_interface_t interface = malloc(sizeof(int));
  if (interface)
    *((int *)interface) = i2c_conf->port;
  else
    i2c_driver_delete(i2c_conf->port);

  return interface;
}

memoree_err_t platform_i2c_deinit(memoree_interface_t interface)
{
  if (!interface)
    return MEMOREE_ERR_INVALID_ARG;

  int i2c_port = *((int *)interface);

  if (i2c_driver_delete(i2c_port) != ESP_OK)
    return MEMOREE_ERR_FAIL;

  free(interface);
  return MEMOREE_ERR_OK;
}

int32_t platform_i2c_read(memoree_interface_t interface, uint8_t addr, uint8_t *read_buff,
                          size_t read_size, size_t timeout_ms)
{
  if (!interface)
    return MEMOREE_ERR_INVALID_ARG;

  i2c_port_t i2c_num = *(int *)interface;
  int ret = i2c_master_read_from_device(i2c_num, addr, read_buff, read_size, pdMS_TO_TICKS(timeout_ms));

  return (ret == ESP_OK) ? read_size : MEMOREE_ERR_FAIL;
}

int32_t platform_i2c_write(memoree_interface_t interface, uint8_t addr, uint8_t *write_buff,
                           size_t write_size, size_t timeout_ms)
{
  if (!interface)
    return MEMOREE_ERR_INVALID_ARG;

  i2c_port_t i2c_num = *(int *)interface;

  int ret;

  /// Retry as long as a timeout has not occured
  int64_t now = esp_timer_get_time();
  do
  {
    ret = i2c_master_write_to_device(i2c_num, addr, write_buff, write_size, pdMS_TO_TICKS(timeout_ms));
  } while (ret != ESP_OK && (esp_timer_get_time() < now + timeout_ms * 1000));

  return (ret == ESP_OK) ? write_size : (esp_timer_get_time() > now + timeout_ms * 1000) ? MEMOREE_ERR_TIMEOUT
                                                                                         : MEMOREE_ERR_FAIL;
}

memoree_err_t platform_i2c_write_read(memoree_interface_t interface, uint8_t addr, uint8_t *write_buff, size_t write_size,
                                      uint8_t *read_buff, size_t read_size, size_t timeout_ms)
{
  if (!interface)
    return MEMOREE_ERR_INVALID_ARG;

  i2c_port_t i2c_num = *(int *)interface;

  int ret = i2c_master_write_read_device(i2c_num, addr, write_buff, write_size, read_buff, read_size, pdMS_TO_TICKS(timeout_ms));
  return (ret == ESP_OK) ? MEMOREE_ERR_OK : MEMOREE_ERR_FAIL;
}

memoree_err_t platform_i2c_ping(memoree_interface_t interface, uint8_t addr, uint32_t timeout_ms)
{
  if (!interface)
    return MEMOREE_ERR_INVALID_ARG;

  i2c_port_t i2c_num = *(int *)interface;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (addr << 1) | I2C_RW_WRITE, I2C_CHECK_ACK);
  i2c_master_stop(cmd);
  int ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(timeout_ms));
  i2c_cmd_link_delete(cmd);
  return (ret == ESP_OK) ? MEMOREE_ERR_OK : MEMOREE_ERR_FAIL;
}

///////////////////////////////SPI FUNCTIONS

memoree_interface_t platform_spi_init(memoree_spi_conf_t *spi_conf)
{
  if (!spi_conf || spi_conf->port >= SPI_HOST_MAX)
    return NULL;

  spi_bus_config_t bus_conf = {
      .mosi_io_num = spi_conf->do_pin,
      .miso_io_num = spi_conf->di_pin,
      .sclk_io_num = spi_conf->sck_pin,
      .quadhd_io_num = spi_conf->hd_pin,
      .quadwp_io_num = spi_conf->wp_pin,
      .max_transfer_sz = 4096,
      .flags = 0,
  };

  esp_err_t err = spi_bus_initialize(spi_conf->port, &bus_conf, SPI_DMA_DISABLED);

  // neither is initialization successful nor is the port already initialized
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    return NULL;

  memoree_interface_t interface = NULL;

  spi_device_handle_t dev_handle;
  spi_device_interface_config_t mem_device;
  memset(&mem_device, 0, sizeof(spi_device_interface_config_t));

  mem_device.mode = spi_conf->mode;
  mem_device.clock_speed_hz = spi_conf->speed;
  mem_device.spics_io_num = spi_conf->cs_pin;
  mem_device.queue_size = 5;

  if (spi_bus_add_device(spi_conf->port, &mem_device, &dev_handle) != ESP_OK)
  {
    spi_bus_free(spi_conf->port);
    return NULL;
  }

  interface = malloc(sizeof(memoree_spi_if_t));
  if (!interface)
  {
    spi_bus_remove_device(dev_handle);
    spi_bus_free(spi_conf->port);
    return NULL;
  }
  gpio_set_level(spi_conf->cs_pin, 1);
  gpio_config_t en_cfg = {
      .pin_bit_mask = BIT64(spi_conf->cs_pin),
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&en_cfg);

  ((memoree_spi_if_t *)interface)->port = spi_conf->port;
  ((memoree_spi_if_t *)interface)->cs_pin = spi_conf->cs_pin;
  ((memoree_spi_if_t *)interface)->dev_handle = (void *)dev_handle;

  return interface;
}

memoree_err_t platform_spi_deinit(memoree_spi_if_t *interface)
{
  if (!interface || !interface->dev_handle)
    return MEMOREE_ERR_INVALID_ARG;

  esp_err_t err = spi_bus_remove_device(interface->dev_handle);
  if (err != ESP_OK)
    return MEMOREE_ERR_FAIL;

  err = spi_bus_free(interface->port);
  if (err != ESP_OK)
    return MEMOREE_ERR_FAIL;

  free(interface);
  return MEMOREE_ERR_OK;
}

memoree_err_t platform_spi_write_read(memoree_spi_if_t *interface, memoree_spi_transaction_t *spi_t)
{
  if (!interface || !spi_t || (spi_t->write_len && !spi_t->write_buff) || (spi_t->read_len && !spi_t->read_buff))
    return MEMOREE_ERR_INVALID_ARG;

  if (!spi_t->write_len && spi_t->read_len)
    memset(spi_t->read_buff, 0, spi_t->read_len);

  int ret;

  /// esp-idf requires that the write length is set to whichever is longer between the read and write lengths, even in a read-only operation
  spi_transaction_ext_t trans_desc = {
      .base.addr = spi_t->addr,
      .base.cmd = spi_t->cmd,
      .base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR | SPI_TRANS_VARIABLE_DUMMY,
      .base.length = MAX(spi_t->write_len, spi_t->read_len) * 8,
      .base.rxlength = spi_t->read_len * 8,
      .base.tx_buffer = (spi_t->read_len > spi_t->write_len) && !spi_t->write_buff ? spi_t->read_buff : spi_t->write_buff,
      .base.rx_buffer = spi_t->read_buff,
      .command_bits = spi_t->cmd_len,
      .address_bits = spi_t->addr_len,
      .dummy_bits = spi_t->dummy_len,
  };

  gpio_set_level(interface->cs_pin, 0);
  ret = spi_device_transmit((spi_device_handle_t)interface->dev_handle, (spi_transaction_t *)&trans_desc);
  gpio_set_level(interface->cs_pin, 1);

  return (ret == 0) ? MEMOREE_ERR_OK : MEMOREE_ERR_FAIL;
}
