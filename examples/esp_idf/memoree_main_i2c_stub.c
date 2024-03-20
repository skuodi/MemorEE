#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "memoree.h"

#define I2C_SDA_PIN GPIO_NUM_9
#define I2C_SCL_PIN GPIO_NUM_11
#define I2C_SPEED 400000
#define I2C_PORT I2C_NUM_0

/// @brief Utility function to check for acknowledge at the given address
/// @param address 7-bit I2C address to check
/// @return 0 on success
esp_err_t _check_address(uint8_t address)
{
  esp_err_t ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, address << 1 | I2C_MASTER_WRITE, 1);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  return ret;
}

/// @brief Scan the entire 7-bit address range, checking for acknowledge
/// @return 7-bit address of the last detected I2C device, on success.
/// @note If the highest bit (bit 7) of the return value is set, no device was detected
uint8_t i2c_detect(void)
{
  esp_err_t ack_state;

  uint8_t mem_addr = 0x80;

  printf("\n\n-[ I2C Detect @ %d Hz ]-\r\n\n   ", I2C_SPEED);

  for (uint8_t i = 0; i < 16; i++)
    printf(" %02x ", i);

  for (uint8_t j = 0; j < 8; j++)
  {
    printf("\n%02x:", j << 4);

    for (uint8_t k = 0; k < 16; k++)
    {
      ack_state = _check_address(j << 4 | k);
      if (ack_state == ESP_OK) /// ACK Received
      {
        printf(" %02X ", (j << 4 | k));
        mem_addr = (j << 4 | k);
      }
      else if (ack_state == ESP_FAIL) /// NACK Received
        printf(" -- ");
      else
        printf(" xx "); /// Miscellaneous error
    }
  }
  printf("\n\n-[ Scan Done ]-\n\n");
  return mem_addr;
}

void app_main(void)
{
  memoree_i2c_conf_t i2c_conf =
      {
          .port = I2C_PORT,
          .sda_pin = I2C_SDA_PIN,
          .scl_pin = I2C_SCL_PIN,
          .speed = I2C_SPEED,
      };

  memoree_t mem = memoree_init(MEMOREE_VARIANT_STUB_I2C, &i2c_conf);

  i2c_conf.addr = i2c_detect();

  if (i2c_conf.addr & 0x08)
  {
    printf("No I2C device detected!!\n");
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  if (mem && memoree_ping(mem, 1000))
  {
    int high_time = 200, low_time = 200; /// Adjust the I2C clock duty cycle so that it's closer to 50%
    ESP_ERROR_CHECK(i2c_set_period(I2C_PORT, high_time, low_time));
    printf("Memoree init success!!\nAddress: 0x%02X\nHigh time: %d\nLow time: %d\n", i2c_conf.addr, high_time, low_time);
  }
  else
  {
    printf("Memoree init 0x%02X fail!!\n", i2c_conf.addr);
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  uint8_t buff[20];
  memset(buff, 0x00, sizeof(buff));

  /// Perform a read of 20 bytes from a 24LC256 device
  memoree_stub_transaction_t t = {
      .addr = i2c_conf.addr,
      .addr_len = 8,
      .write_len = 3,
      .write_buff = buff,
      .read_len = sizeof(buff),
      .read_buff = buff,
      .timeout_ms = 100,
  };

  if (memoree_stub_write_read(mem, &t) == MEMOREE_ERR_OK)
    printf("Memory stub read success!!\n");
  else
    printf("Memory stub read failed!!\n");

  memoree_deinit(mem, true);
}
