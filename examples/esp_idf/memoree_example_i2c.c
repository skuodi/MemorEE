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
void i2c_detect(void)
{
  esp_err_t ack_state;

  printf("\n\n-[ I2C Detect ]-\r\n\n   ");

  for (uint8_t i = 0; i < 16; i++)
    printf(" %02x ", i);
  printf("\n");

  for (uint8_t j = 0; j < 8; j++)
  {
    printf("%02x:", j << 4);

    for (uint8_t k = 0; k < 16; k++)
    {
      ack_state = _check_address(j << 4 | k);
      if (ack_state == ESP_OK)
        printf(" %02x ", (j << 4 | k));
      else if (ack_state == ESP_FAIL) /// No ACK Received At That Address
        printf(" -- ");
      else
        printf(" xx "); /// Miscellaneous error
    }
    printf("\n");
  }
  printf("\n-[ Scan Done ]-\n\n");
}

/// @brief Dump the contents of the entire memoree_t
/// @param buff_size Size of chunks used to read data
/// @param iter_delay_ms Millisecond delay between reading of data chunks
/// @param line_width Number of bytes to print out in one line
bool memdump(memoree_t mem, uint16_t buff_size, uint32_t iter_delay_ms, uint8_t line_width)
{
  memoree_info_t memory;
  memoree_get_info(mem, &memory);
  buff_size = (memory.size < buff_size) ? memory.size : buff_size;

  uint8_t buff[buff_size];
  int iterations = memory.size / buff_size;
  char line_buff[256];

  snprintf(line_buff, sizeof(line_buff), "        ");
  for (int header = 0; header < line_width; header++)
    snprintf(line_buff + strlen(line_buff), sizeof(line_buff), "0x%02X ", header);
  printf("%s\n", line_buff);

  for (int i = 0; i < iterations; i++)
  {
    int bytes_read = memoree_read(mem, i * buff_size, buff, buff_size, buff_size * (memory.speed / 8000));
    if (bytes_read > 0)
    {
      for (int j = 0; j < (buff_size / line_width); j++)
      {
        snprintf(line_buff, sizeof(line_buff), "0x%04X: ", (i * buff_size) + (j * line_width));
        for (int k = 0; k < line_width; k++)
          snprintf(line_buff + strlen(line_buff), sizeof(line_buff), "0x%02X ", buff[j]);
        printf("%s\n", line_buff);
      }
    }
    else
    {
      printf("Memory read at 0x%04X: failed\n", i);
      return false;
    }
  }
  printf("\n!!!!!Memory dumped :%ld bytes!!!!!\n", memory.size);

  return true;
}

void app_main(void)
{
  memoree_i2c_conf_t i2c_conf =
      {
          .addr = 0x52,
          .port = I2C_PORT,
          .sda_pin = I2C_SDA_PIN,
          .scl_pin = I2C_SCL_PIN,
          .speed = I2C_SPEED,
      };

  memoree_t mem = memoree_init(MEMOREE_VARIANT_24XX256, &i2c_conf);
  i2c_detect();

  if (mem && memoree_ping(mem, 1000))
  {
    int high_time = 250, low_time = 150; /// Adjust the I2C clock duty cycle so that it's closer to 50%
    ESP_ERROR_CHECK(i2c_set_period(I2C_PORT, high_time, low_time));
    printf("Memory init success!!\nAddress: 0x%02X\nHigh time: %d\nLow time: %d\n", i2c_conf.addr, high_time, low_time);
  }
  else
  {
    printf("Memory init 0x%02X fail!!\n", i2c_conf.addr);
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // memdump(mem, 1024, 1, 16);

  // vTaskDelay(pdMS_TO_TICKS(1000));

  // if (memoree_erase(mem, 0xFF))
  //   printf("Memory erase success!!\n");
  // else
  // {
  //   printf("Memory erase failed!!\n");
  //   while (1)
  //     vTaskDelay(pdMS_TO_TICKS(1000));
  // }

  vTaskDelay(pdMS_TO_TICKS(1000));

  memdump(mem, 1024, 1, 16);

  memoree_deinit(mem, true);
}
