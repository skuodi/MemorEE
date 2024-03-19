#ifndef _MEMOREE_PLATFORM_H_
#define _MEMOREE_PLATFORM_H_
#include <stdint.h>
#include "memoree.h"

/// @brief SPI interface handle
typedef struct 
{
  int port;         ///< Platform-specific peripheral identifier
  int cs_pin;       ///< SPI chip select pin
  void *dev_handle; ///< Memory device handle, optionally use as peripheral handle 
}memoree_spi_if_t;

/// @brief SPI transaction descriptor
typedef struct 
{
  uint8_t cmd_len;     ///< Command length in bits
  uint32_t cmd;        ///< Command sent MSB first
  uint8_t addr_len;    ///< Address length in bits
  uint32_t addr;       ///< Address, sent MSB first
  uint8_t dummy_len;   ///< Dummy length in bits
  uint32_t read_len;   ///< Read length in bytes
  uint8_t *read_buff;  
  uint32_t write_len;  ///< Write length in bytes
  uint8_t *write_buff; 
  uint32_t timeout_ms; ///< Timeout for the transaction in ms
}memoree_spi_transaction_t;

/// @brief Initialize an I2C peripheral
/// @param i2c_conf Interface configuration
/// @return memoree_interface_t object on success
/// @return NULL on fail
memoree_interface_t platform_i2c_init(memoree_i2c_conf_t *i2c_conf);

/// @brief Deinitialize an I2C peripheral and free allocated resources
/// @param port Platform-specific I2C port identifier
memoree_err_t platform_i2c_deinit(memoree_interface_t port);

/// @brief Send an address byte with the RW bit set to write and wait for acknowledgement
/// @param port Platform-specific I2C port identifier
/// @param addr 7-bit I2C address
memoree_err_t platform_i2c_ping(memoree_interface_t interface, uint8_t addr, uint32_t timeout_ms);

/// @brief Read \a read_size bytes
/// @param port Platform-specific I2C port identifier
/// @param addr 7-bit I2C address
/// @return Number of bytes read, on success
/// @return memoree_err_t error code, on failure
int32_t platform_i2c_read(memoree_interface_t interface, uint8_t addr, uint8_t *read_buff,
                          size_t read_size, size_t timeout_ms);

/// @brief Write \a write_size bytes
/// @param port Platform-specific I2C port identifier
/// @param addr 7-bit I2C address
/// @return Number of bytes written, on success
/// @return memoree_err_t error code, on failure
int32_t platform_i2c_write(memoree_interface_t interface, uint8_t addr, uint8_t *write_buff,
                           size_t write_size, size_t timeout_ms);

/// @brief Writes \a write_size bytes, performs an I2C repeated start, then reads \a read_size bytes
/// @param port Platform-specific I2C port identifier
/// @param addr 7-bit I2C address
memoree_err_t platform_i2c_write_read(memoree_interface_t interface, uint8_t addr, uint8_t *write_buff, size_t write_size,
                                uint8_t *read_buff, size_t read_size, size_t timeout_ms);

/// @brief Initialize an SPI peripheral
/// @param spi_conf Platform-specific SPI configuration
/// @return memoree_interface_t interface object, on success
/// @return NULL, on failure
memoree_interface_t platform_spi_init(memoree_spi_conf_t *spi_conf);

/// @brief Deinitialize an SPI peripheral and free allocated resources
memoree_err_t platform_spi_deinit(memoree_spi_if_t *interface);

/// @brief Write or read data on the SPI bus depending on transaction settings.
/// @param spi_t SPI transaction information
memoree_err_t platform_spi_write_read(memoree_spi_if_t *interface, memoree_spi_transaction_t *spi_t);

/// @brief Millisecond delay implementation
void platform_ms_delay(uint32_t ms);

#endif