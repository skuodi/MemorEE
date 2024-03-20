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

#define SPI_DO_PIN 11
#define SPI_SCK_PIN 2
#define SPI_DI_PIN 39
#define SPI_CS_PIN 1
#define SPI_SPEED 1000000
#define SPI_PORT SPI3_HOST

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

  memoree_t mem = memoree_init(MEMOREE_VARIANT_STUB_SPI, &spi_conf);
  if (!mem)
  {
    printf("Memory init failed!!\n");
    while (1)
      vTaskDelay(pdMS_TO_TICKS(1000));
  }

  uint8_t buff[20];
  memset(buff, 0xAA, sizeof(buff));

  memoree_stub_transaction_t t = {
      .write_len = sizeof(buff),
      .write_buff = buff,
      .timeout_ms = 100,
  };

  if (memoree_stub_write_read(mem, &t) == MEMOREE_ERR_OK)
    printf("Memory stub write success!!\n");
  else
    printf("Memory stub write failed!!\n");

  memoree_deinit(mem, true);

}
