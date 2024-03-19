#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "memoree.h"

#define SPI_DO_PIN 2
#define SPI_SCK_PIN 11
#define SPI_DI_PIN 10
#define SPI_CS_PIN 38
#define SPI_SPEED 4000000
#define SPI_PORT SPI3_HOST

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

  uint64_t timestamp = esp_timer_get_time();

  for (int i = 0; i < iterations; i++)
  {
    int bytes_read = memoree_read(mem, i * buff_size, buff, buff_size, buff_size / (memory.speed / 8000));
    // printf("memoree_read() got %d for timeout: %ld\n", bytes_read, buff_size * (memory.speed / 8000));
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
  
  timestamp = esp_timer_get_time() - timestamp;

  printf("\n!!!!!Memory dumped :%ld bytes in %lld.%lld ms!!!!!\n", memory.size, timestamp / 1000, timestamp % 1000);

  return true;
}

void gpio_init(void)
{
  gpio_set_level(47, 1);
  gpio_set_level(3, 1);
  gpio_config_t en_cfg = {
      .pin_bit_mask = BIT64(47) | BIT64(3),
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&en_cfg);
}

void app_main(void)
{
  gpio_init();
  memoree_spi_conf_t spi_conf =
      {
          .port = SPI_PORT,
          .mode = 0,
          .cs_pin = SPI_CS_PIN,
          .di_pin = SPI_DI_PIN,
          .do_pin = SPI_DO_PIN,
          .sck_pin = SPI_SCK_PIN,
          .hd_pin = -1,
          .wp_pin = -1,
          .speed = SPI_SPEED,
      };

  gpio_config_t en_cfg = {
      .pin_bit_mask = BIT64(SPI_DI_PIN),
      .mode = GPIO_MODE_INPUT,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
  };
    gpio_config(&en_cfg);

  memoree_t mem = memoree_init(MEMOREE_VARIANT_25XX_SFDP, &spi_conf);
  if (!mem)
  {
    printf("Memory init failed!!\n");
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  vTaskDelay(pdMS_TO_TICKS(1000));
  memdump(mem, 64, 1, 16);

  memoree_deinit(mem, true);

  while (1)
  {

    // sfdp_param_t param;

    // if (memoree_get_sfdp(mem, &param, 100) == MEMOREE_ERR_OK)
    // {
    //   printf("\n!!!GOT SFDP INFO!!\n");
    //   printf("SFDP Header version: %d.%d\n", (param.header_ver >> 8 & 0xFF), (param.header_ver & 0xFF));
    //   printf("Header count: %d\n", param.header_cnt);
    //   printf("SFDP table version: %d.%d\n", (param.fparam_ver >> 8 & 0xFF), (param.fparam_ver & 0xFF));
    //   printf("SFDP table size: %d bytes\n", param.fparam_size);
    //   printf("SFDP table address: 0x%04lX\n\n", param.fparam_ptr);
    //   printf("Flash size: %lld bits = %lld bytes\n", param.size * 8, param.size);
    //   printf("Address length: %d bytes\n", param.addr_bytes);
    //   printf("Write granularity: %d\n", param.write_size);
    //   printf("DDR: %s\n", (param.dtr_support) ? "Supported" : "Not supported");
    //   printf("4K sector erase: %s\n", (param.erase4k_opcode) ? "Supported" : "Not supported");
    //   if (param.erase4k_opcode)
    //     printf("Sector erase opcode: 0x%02X\n", param.erase4k_opcode);
    // }
    // else
    //   printf("!!GET SFDP INFO FAILED!!");

    vTaskDelay(pdMS_TO_TICKS(1000));

    // memoree_write_byte(mem, 0, 0xAA, 100);
  }
}
