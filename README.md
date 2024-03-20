## MemorEE

Minimal, versatile, portable library for reading from/writing to SPI and I2C memory ICs. 
This library is mainly aimed at quick and dirty flash dumping/editing operations, as opposed to usage as a driver for applications where high reliablility and maximum throughput (such as XIP) and therefore does not explicitly support a wide variety of chips.

## Features
- Automatic address translation so that writes are page aligned.
- Optional address wrapping on overflow
- Automatic memory size detection for SFDP memories
- Transparent read and write operations that provide direct access to the underlying peripheral interface (i.e. I2C or SPI).

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

## Usage

Using this library in your esp-idf project requires 3 steps:

1. Clone this repo into your project directory,
  
  ```
  git clone https://github.com/skuodi/memoree    
  ```
  or as add it to your git-indexed project as a submodule:
  
  ```
  git submodule add https://github.com/skuodi/memoree    
  ```

  and set it up as an esp-idf component by adding  a `CMakeLists.txt` file to the parent directory of this directory with the following contents,

  ```sh

  idf_component_register(SRCS "memoree.c" "platform/memoree_espidf.c"
                      INCLUDE_DIRS "." "platform"
                      REQUIRES driver esp_timer)

  ```

  so that the parent directory of this library is structured as follows.

  ```sh

    memoree
    ├── CMakeLists.txt
    ├── docs
    ├── Doxyfile
    ├── examples
    ├── LICENSE
    ├── memoree.c
    ├── memoree.h
    ├── platform
    └── README.md

  ```

2. Include the [memoree.h](memoree.h) in your application, and initialize the interface parameters according to the pin and interface port conventions for your platform.

For `W25Q16CV`, an SPI flash chip,

  ```c

    #include "memoree.h"

    ...

    memoree_spi_conf_t spi_conf =
        {
            .port = SPI2_HOST,
            .mode = 0,
            .cs_pin = GPIO_NUM_9,
            .di_pin = GPIO_NUM_10,
            .do_pin = GPIO_NUM_11,
            .sck_pin = GPIO_NUM_12,
            .hd_pin = -1,           // Platform specific, indicates this pin is unused
            .wp_pin = -1,           // Platform specific, indicates this pin is unused
            .speed = 4000000,
        };

    memoree_t mem = memoree_init(MEMOREE_VARIANT_25XX_SFDP, &spi_conf);
      
  ```
and for `24LC256`, an I2C EEPROM,

  ```c

    memoree_i2c_conf_t i2c_conf =
          {
              .port = I2C_NUM_0,
              .sda_pin = GPIO_NUM_9,
              .scl_pin = GPIO_NUM_11,
              .speed = 400000,
          };

    memoree_t mem = memoree_init(MEMOREE_VARIANT_24XX256, &i2c_conf);

  ```

or directly accessing the I2C interface:

  ```c
    memoree_t mem = memoree_init(MEMOREE_VARIANT_STUB_I2C, &i2c_conf);

  ```

3. Set up and perform a transaction. For example, to read 20 bytes from a 24LC256 device
  - using a built-in definition of the 24LC256 device

  ```c

    uint8_t buff[20];
    memset(buff, 0x00, sizeof(buff));

    if (memoree_read(mem, 0, buff, siezof(buff), 100) == MEMOREE_ERR_OK)
      printf("Memory read success!!\n");
    else
      printf("Memory read failed!!\n");

  ```

  - or via direct access to the I2C bus:

  ```c

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


  ```

-  You can get information about your device configuration by calling `memoree_get_info()`

### Sample output

The [memoree_example_i2c_stub.c](examples/esp_idf/memoree_example_i2c_stub.c) example reads from an I2C memory device giving the following output after running `idf.py monitor`.

```

  -[ I2C Detect @ 400000 Hz ]-

      00  01  02  03  04  05  06  07  08  09  0a  0b  0c  0d  0e  0f 
  00: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  10: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  20: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  30: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  40: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  50: 50  --  xx  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  60: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 
  70: --  --  --  --  --  --  --  --  --  --  --  --  --  --  --  -- 

  -[ Scan Done ]-

  Memoree init success!!
  Address: 0x50
  High time: 200
  Low time: 200
  Memory stub read success!!

```

## Porting

Create an implementation of the functions in [memoree_platform.h](platform/memoree_platform.h) specific to your target platform (and name it memoree_\<your_platform\>.c where `<your_platform>` is a the name of the target platform)

## License

[MIT](./LICENSE)