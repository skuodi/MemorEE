## MemorEE

Minimal, versatile, portable library for reading from/writing to SPI and I2C memory ICs. 
This library is mainly aimed at quick and dirty flash dumping/editing operations, as opposed to usage as a driver for applications where high reliablility and maximum throughput (such as XIP) and therefore does not explicitly support a wide variety of chips.

## Features
- Transparent read and write operations
- Automatic address page translation 
- Optional address wrapping on overflow
- Automatic memory size detection for SFDP memories

## Supported devices
- I2C EEPROMs
  - 24XX02
  - 24XX04
  - 24XX08
  - 24XX16
  - 24XX32
  - 24XX256 (tested)
  - 24XX1024
  
- SPI EEPROMs
  - 93C46 (tested)
  - 93C56
  - 93C66
  - 93C76
  - 93C86
  
- SFDP-compatible SPI flash memories (25XX)

## Sample output
![]()
![]()

## Porting

1. Create an implementation of the functions in [memoree_platform.h](platform/memoree_platform.h) specific to your target platform (and name it memoree_\<your_platform\>.c where `<your_platform>` is a the name of the target platform)
2. In your application, define the interface of your memory IC by initializing the interface parameters according to the pin and interface port conventions for your platform.

For example, for `24LC256` with on ESP32,
  ```c
    memoree_i2c_conf_t i2c_conf =
        {
            .addr = 0x50,
            .port = I2C_NUM_0,
            .sda_pin = GPIO_NUM_10,
            .scl_pin = GPIO_NUM_12,
            .speed = 100000,
        };

    memoree_t mem = memoree_init(MEMOREE_VARIANT_24XX256, &i2c_conf);
    
  ```
and for `W25Q16CV`,

  ```c
    memoree_spi_conf_t spi_conf =
        {
            .port = SPI2_HOST,
            .mode = 0,
            .cs_pin = GPIO_NUM_9,
            .di_pin = GPIO_NUM_10,
            .do_pin = GPIO_NUM_11,
            .sck_pin = GPIO_NUM_12,
            .hd_pin = -1,     // Platform specific, indicates this pin is unused
            .wp_pin = -1,// Platform specific, indicates this pin is unused
            .speed = 4000000,
        };

    memoree_t mem = memoree_init(MEMOREE_VARIANT_25XX_SFDP, &spi_conf);
    
```

3. You can get information about your memory chip by calling `memoree_get_info()`

4. Read from your memory chip with `memoree_read()` and write with `memoree_write()`

## License

[MIT](./LICENSE)