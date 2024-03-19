#ifndef _MEMOREE_H_
#define _MEMOREE_H_

/**
 * @file    memoree.h
 * @author  skuodi
 * @date
 */

#include <stdint.h>
#include <stdbool.h>

#define MEMOREE_I2C_BASE_ADDRESS ((0b1010 << 3) & 0xFF)

#define MEMOREE_I2C_MAX_SPEED 400000
#define MEMOREE_SPI_93X_MAX_SPEED 2000000
#define MEMOREE_SPI_MAX_SPEED 40000000

/// Commands for 93XX SPI flash memories
/// Commands are variable in length and the number of bits defined here must match the number of bits sent during the command phase of communication
#define MEMOREE_CMD_93CXX_READ 0b110   ///< Read Data from Memory
#define MEMOREE_CMD_93CXX_WRITE 0b101  ///< Write Data to Memory
#define MEMOREE_CMD_93CXX_WEN 0b10011  ///< Write Enable
#define MEMOREE_CMD_93CXX_WDS 0b10000  ///< Write Disable
#define MEMOREE_CMD_93CXX_ERASE 0b111  ///< Erase Byte or Word
#define MEMOREE_CMD_93CXX_ERAL 0b10010 ///< Erase All Memory
#define MEMOREE_CMD_93CXX_WRAL 0b10001 ///< Write All Memory with same Data

/// Commands for 25XX SPI flash memories
#define MEMOREE_CMD_25XX_WREN 0x06 ///< Set Write Enable Latch
#define MEMOREE_CMD_25XX_WRDI 0x04 ///< Reset Write Enable Latch
#define MEMOREE_CMD_25XX_RDSR 0x05 ///< Read Status Register
#define MEMOREE_CMD_25XX_WRSR 0x01 ///< Write Status Register
#define MEMOREE_CMD_25XX_READ 0x03 ///< Read Data from Memory Array
#define MEMOREE_CMD_25XX_PP 0x02   ///< Program Data Into Memory Array
#define MEMOREE_CMD_25XX_RDID 0x9F ///< Read Manufacturer and Product ID
#define MEMOREE_CMD_25XX_SFDP 0x5A ///< Read JEDEC serial flash discovery parameters

typedef enum
{
  MEMOREE_ERR_OK,                       ///< Success
  MEMOREE_ERR_FAIL = -1,                ///< Miscellaneous failure
  MEMOREE_ERR_MEM = -2,                 ///< Memory allocation failed
  MEMOREE_ERR_INVALID_ARG = -3,         ///< Invalid parameters passed
  MEMOREE_ERR_TIMEOUT = -4,             ///< Operation timeout
  MEMOREE_ERR_SFDP_NOT_SUPPORTED = -5,  ///< SPI flash does not support SFDP
  MEMOREE_ERR_SFDP_INVALID_HEADER = -6, ///< SPI flash SFDP header is corrupted
  MEMOREE_ERR_SFDP_INVALID_TABLE = -7,  ///< SPI flash SFDP flash parameter table is corrupted
} memoree_err_t;

typedef void *memoree_interface_t;

/// @brief Supported memory IC part numbers
/// @todo Handle ORG pin on 93CXX variants
/// @todo Implement memory protection enable/disable
typedef enum
{
  MEMOREE_VARIANT_24XX02,
  MEMOREE_VARIANT_24XX04,
  MEMOREE_VARIANT_24XX08,
  MEMOREE_VARIANT_24XX16,
  MEMOREE_VARIANT_24XX32,
  MEMOREE_VARIANT_24XX64,
  MEMOREE_VARIANT_24XX128,
  MEMOREE_VARIANT_24XX256,
  MEMOREE_VARIANT_24XX512,
  MEMOREE_VARIANT_24XX1024, ///< For 24XX1024 or 24XX1025
  MEMOREE_VARIANT_I2C_MAX,
  MEMOREE_VARIANT_93C46,
  MEMOREE_VARIANT_93C56,
  MEMOREE_VARIANT_93C66,
  MEMOREE_VARIANT_93C76,
  MEMOREE_VARIANT_93C86,
  MEMOREE_VARIANT_93CXX_MAX,
  MEMOREE_VARIANT_25XX_SFDP, ///< For SPI flash implementing Serial Flash Description Parameters (SFDP)
  MEMOREE_VARIANT_MAX,
} memoree_variant_t;

/// @brief Information extracted from the Serial Flash Discovery Parameters table of an SFDP-capable SPI flash memory
typedef struct
{
  uint16_t header_ver;    ///< SFDP Header version (MAJOR(15:8) | MINOR(7:0))
  uint8_t header_cnt;     ///< Number of parameter headers
  uint16_t fparam_ver;    ///< SFDP Flash parameters version (MAJOR(15:8) | MINOR(7:0))
  uint16_t fparam_size;   ///< Flash parameter table size in bytes
  uint32_t fparam_ptr;    ///< Flash parameter table memory location for use with the SFDP read command
  uint8_t write_size;     ///< Write Granularity
  uint8_t wen_opcode;     ///< Write Enable Opcode Select for Writing to Volatile Status Register
  uint8_t erase4k_opcode;   ///< 4 Kilobyte Erase Opcode
  uint8_t addr_bytes;     ///< Number of bytes used in addressing flash array read, write and erase
  bool dtr_support;       ///< Whether double transfer rate is supported
  uint8_t min_sector;     ///< Minimum erasable sector size
  uint8_t min_sec_opcode; ///< Opcode to erase minimum erasable sector
  uint8_t max_sector;     ///< Maximum erasable sector size
  uint8_t max_sec_opcode; ///< Opcode to erase maximum erasable sector
  uint64_t size;          ///< Flash memory size in bytes

} sfdp_param_t;

/// @brief Supported peripheral interfaces
typedef enum
{
  MEMOREE_TYPE_I2C,
  MEMOREE_TYPE_SPI
} memoree_type_t;

typedef struct
{
  int port;       ///< Platform-specific identifier for the I2C peripheral used
  uint32_t speed; ///< Interface speed in Hz
  int sda_pin;    ///< Data pin
  int scl_pin;    ///< Clock pin
  uint8_t addr;   ///< Memory IC I2C 7-bit address
} memoree_i2c_conf_t;

typedef struct
{
  int port;       ///< Platform-specific identifier for the SPI peripheral used
  uint32_t speed; ///< Interface speed in Hz
  int do_pin;     ///< Controller data out pin
  int sck_pin;    ///< Clock pin
  int di_pin;     ///< Controller data in pin
  int cs_pin;     ///< Chip select pin
  int hd_pin;     ///< Hold pin, used as ORG pin for 93CXX
  int wp_pin;     ///< Write protect pin
  int mode;       ///< SPI mode
} memoree_spi_conf_t;

typedef enum
{
  MEMOREE_PROTECTION_NONE,
  MEMOREE_PROTECTION_READ,
  MEMOREE_PROTECTION_WRITE,
} memoree_protection_t;

/// @brief Memoree object
typedef struct memoree *memoree_t;

/// @brief Configuration information for a \link memoree_t \endlink object
typedef struct
{
  memoree_type_t type;         ///< Serial interface type
  memoree_variant_t variant;   ///< Part number
  uint32_t size;               ///< Size in bytes, must be a power of 2
  uint32_t speed;              ///< Interface speed
  uint8_t addr_len;            ///< Number of bits used in the address phase of a read/write operation
  uint8_t addr;                ///< 7-bit address (for I2C ICs)
  uint16_t page_size;          ///< Page size in bytes
  uint16_t num_pages;          ///< Number of pages
  uint8_t page_write_delay_ms; ///< Maximum page write time (ms)
  bool protected;              ///< Whether write protection is enabled
} memoree_info_t;

// FUNCTIONS

/// @brief Dynamically allocates a memory object from the arguments passed
/// @warning The memory object returned by this function must be deinitialized using memoree_deinit() to free allocated resources
/// @param variant Part number of the memory IC
/// @param interface_conf memoree_i2c_conf_t or memoree_spi_conf_t object containing interface settings
/// @return memoree object on success
/// @return NULL on failure
memoree_t memoree_init(memoree_variant_t variant, void *interface_conf);

/// @brief  Frees dynamically allocated resources and optionally deinitializes the interface attached to the memory object
/// @param  deinit Whether to deinitialize the peripheral interface that the memory device is attached to
memoree_err_t memoree_deinit(memoree_t mem, bool deinit);

/// @brief Detect the presence of a functional chip connected to the initialized interface.
/// @brief For I2C chips, checks for acknowledgement. For SFDP ICs, checks for a valid SFDP table.
memoree_err_t memoree_ping(memoree_t mem, size_t timeout_ms);

/// @brief Reads one byte from the memory location specified by \a addr
memoree_err_t memoree_read_byte(memoree_t mem, uint32_t addr, uint8_t *data, size_t timeout_ms);

/// @brief Write one byte to the memory location specified by \a addr
memoree_err_t memoree_write_byte(memoree_t mem, uint32_t addr, uint8_t data, size_t timeout_ms);

/// @brief Read \a data_len bytes starting at the memory location specified by \a addr
/// @return Number of bytes read, on success
/// @return \link memoree_err_t \endlink error code, on fail
int memoree_read(memoree_t mem, uint32_t addr, uint8_t *data, uint32_t data_len, size_t timeout_ms);

/// @brief Write \a data_len bytes starting at the memory location specified by \a addr
/// @param wrap Whether to wrap to the beginning if the address reaches the end of the memory area
/// @return Number of bytes written, on success
/// @return \link memoree_err_t \endlink error code, on fail
int memoree_write(memoree_t mem, uint32_t addr, uint8_t *data, uint32_t data_len, size_t timeout_ms, bool wrap);

/// @brief Write \a erase_value to all the bytes in the specified memory \a page
memoree_err_t memoree_erase_page(memoree_t mem, uint32_t page, uint8_t erase_value);

/// @brief Write \a erase_value to all bytes in memory
memoree_err_t memoree_erase(memoree_t mem, uint8_t erase_value);

/// @brief Returns the current configuration information of the memory object
memoree_err_t memoree_get_info(memoree_t mem, memoree_info_t *mem_info);

/// @brief Enables memory protection if the \link memoree_variant_t \endlink supports it
/// @warning NOT YET IMPLEMENTED.
/// @param protect Type of memory protection to enforce
memoree_err_t memoree_protect(memoree_t mem, memoree_protection_t protect);

/// @brief  Read serial flash description parameter information and populate the properties of the corresponding memoree object
memoree_err_t memoree_get_sfdp(memoree_t mem, sfdp_param_t *param, size_t timeout_ms);

#endif