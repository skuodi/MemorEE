#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "memoree.h"
#include "memoree_platform.h"

#define VARIANT_ISSTUB(v) (v == MEMOREE_VARIANT_STUB_I2C || v == MEMOREE_VARIANT_STUB_SPI)
#define VARIANT_ISVALID(v) (v < MEMOREE_VARIANT_MAX && v != MEMOREE_VARIANT_I2C_MAX && v != MEMOREE_VARIANT_93CXX_MAX && v != MEMOREE_VARIANT_STUB_SPI && v != MEMOREE_VARIANT_STUB_I2C)
#define MEMOREE_ISVALID(m) (m && m->interface && VARIANT_ISVALID(m->info.variant))
#define ADDRESS_ISVALID(m, a) (m && (a < m->info.size))
#define PAGE_ISVALID(m, p) (m && (p < m->info.num_pages))

#define MEMOREE_DEFAULT_TIMEOUT(m, s) (s / (m->info.speed / 8000))

/// @brief Generic configuration parameter used to extract common peripheral settings
typedef struct
{
  int port;
  uint32_t speed;
} memoree_periph_conf_t;

/// @brief Table of memory variant properties used by memoree_init() to initialize a device
/// @note For SFDP memories, the size, address length and number of pages are populated from SFDP table using memoree_get_sfdp()
/// @note When adding new entries, ensure the index of entries here matches the values defined in the definitions in \link memoree_variant_t \endlink
/// @note because the \link memoree_variant_t \endlink value is used by memoree_init() to index directly into this array.
memoree_info_t mem_props[] = {
    {
        .variant = MEMOREE_VARIANT_STUB_I2C,
        .type = MEMOREE_TYPE_I2C,
    },
    {
        .variant = MEMOREE_VARIANT_24XX02,
        .type = MEMOREE_TYPE_I2C,
        .size = 256,
        .addr_len = 8,
        .page_size = 8,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX04,
        .type = MEMOREE_TYPE_I2C,
        .size = 512,
        .addr_len = 8,
        .page_size = 16,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX08,
        .type = MEMOREE_TYPE_I2C,
        .size = 1024,
        .addr_len = 8,
        .page_size = 16,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX16,
        .type = MEMOREE_TYPE_I2C,
        .size = 2048,
        .addr_len = 8,
        .page_size = 16,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX32,
        .type = MEMOREE_TYPE_I2C,
        .size = 4096,
        .addr_len = 16,
        .page_size = 32,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX64,
        .type = MEMOREE_TYPE_I2C,
        .size = 8192,
        .addr_len = 16,
        .page_size = 32,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX128,
        .type = MEMOREE_TYPE_I2C,
        .size = 16384,
        .addr_len = 16,
        .page_size = 64,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX256,
        .type = MEMOREE_TYPE_I2C,
        .size = 32768,
        .addr_len = 16,
        .page_size = 64,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX512,
        .type = MEMOREE_TYPE_I2C,
        .size = 65536,
        .addr_len = 16,
        .page_size = 128,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_24XX1024,
        .type = MEMOREE_TYPE_I2C,
        .size = 131072,
        .addr_len = 16,
        .page_size = 128,
        .page_write_delay_ms = 5,
    },
    {}, ///< MEMOREE_VARIANT_I2C_MAX filler structure for alignment
    {
        .variant = MEMOREE_VARIANT_STUB_SPI,
        .type = MEMOREE_TYPE_SPI,
    },
    {
        .variant = MEMOREE_VARIANT_93C46,
        .type = MEMOREE_TYPE_SPI,
        .size = 128,
        .addr_len = 7,
        .page_size = 1,
        .page_write_delay_ms = 10,
    },
    {
        .variant = MEMOREE_VARIANT_93C56,
        .type = MEMOREE_TYPE_SPI,
        .size = 256,
        .addr_len = 9,
        .page_size = 1,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_93C66,
        .type = MEMOREE_TYPE_SPI,
        .size = 512,
        .addr_len = 9,
        .page_size = 1,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_93C76,
        .type = MEMOREE_TYPE_SPI,
        .size = 1024,
        .addr_len = 11,
        .page_size = 1,
        .page_write_delay_ms = 5,
    },
    {
        .variant = MEMOREE_VARIANT_93C86,
        .type = MEMOREE_TYPE_SPI,
        .size = 2048,
        .addr_len = 11,
        .page_size = 1,
        .page_write_delay_ms = 5,
    },
    {}, ///< MEMOREE_VARIANT_93CXX_MAX filler structure for alignment
    {
        .variant = MEMOREE_VARIANT_25XX_SFDP,
        .type = MEMOREE_TYPE_SPI,
        .page_write_delay_ms = 5,
    },
};

/// @brief Holds the properties of the memory chip such as size and address length, as well as a handle to the peripheral interface it is connected to
struct memoree
{
  memoree_interface_t interface;
  memoree_info_t info;
};

//////////////////////UTILITY FUNCTIONS

/// @brief Returns the number of bits required to address the entire memory array
/// @return Number of bits, on success
/// @return \link memoree_err_t \endlink error code, on fail
static int _get_address_space(memoree_t mem)
{
  if (!mem || !(mem->info.size) || mem->info.size % 2)
    return MEMOREE_ERR_INVALID_ARG;

  uint32_t size = mem->info.size - 1;
  int bit_width = 0;
  while ((size >> bit_width++) & 1);

  return bit_width;
}

/// @brief Sends \a addr in the address phase of communicaton followed by a stream of \a data_len bytes
/// @note This fuction does not perform any address translation of pages nor does it split data into chunks before sending.
/// @return Number of bytes written, on success
/// @return \link memoree_err_t \endlink error code, on failure
static int _memoree_write_bytes(memoree_t mem, uint32_t addr, uint8_t *data, uint32_t data_len, size_t timeout_ms)
{
  if (!ADDRESS_ISVALID(mem, addr) || !data)
    return MEMOREE_ERR_INVALID_ARG;

  if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX)
  {
    uint8_t write_buffer[data_len + (mem->info.addr_len / 8)];
    uint8_t *write_p = write_buffer;
    int8_t addr_space = _get_address_space(mem);
    if (addr_space <= 0)
      return MEMOREE_ERR_INVALID_ARG;

    uint8_t i2c_address;

    // Pack the MSB of the address into byte 3:1 of the I2C target address
    if (addr_space <= mem->info.addr_len)
      i2c_address = mem->info.addr;
    else
      i2c_address = mem->info.addr | (((addr >> mem->info.addr_len) << 1) & 0x0F);

    int i = mem->info.addr_len;

    // Prepend the address bytes to the data buffer
    do
    {
      i -= 8;
      *write_p++ = addr >> i;
    } while (i);
    memcpy(write_p, data, data_len);

    int ret = platform_i2c_write(mem->interface, i2c_address, write_buffer, sizeof(write_buffer), timeout_ms);
    return (ret > 0) ? ret - (mem->info.addr_len / 8) : ret;
  }
  else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    memoree_spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.cmd_len = 8;
    t.cmd = MEMOREE_CMD_25XX_PP;
    t.addr_len = mem->info.addr_len;
    t.addr = (addr & (mem->info.size - 1));
    t.write_len = data_len;
    t.write_buff = data;
    t.timeout_ms = timeout_ms ? timeout_ms : 0;

    int ret = platform_spi_write_read(mem->interface, &t);
    return (ret == 0) ? data_len : MEMOREE_ERR_FAIL;
  }
  else
    return MEMOREE_ERR_INVALID_ARG;

  return MEMOREE_ERR_FAIL;
}

/// @brief Send a write enable command to an SPI memory device
static memoree_err_t _memoree_spi_write_enable(memoree_t mem)
{
  if (!MEMOREE_ISVALID(mem))
    return MEMOREE_ERR_INVALID_ARG;

  memoree_spi_transaction_t t;
  memset(&t, 0, sizeof(t));

  if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX)
    return MEMOREE_ERR_INVALID_ARG;
  else if (mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
  {
    t.cmd_len = 5;
    t.cmd = MEMOREE_CMD_93CXX_WEN;
    t.addr_len = 5;

    int8_t ret = platform_spi_write_read(mem->interface, &t);
    if (ret == 0)
      return MEMOREE_ERR_OK;
  }
  else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    t.cmd = MEMOREE_CMD_25XX_WREN;
    t.cmd_len = 8;
    int ret = platform_spi_write_read(mem->interface, &t);
    if (ret == 0)
      return MEMOREE_ERR_OK;
  }

  return MEMOREE_ERR_FAIL;
}

//////////////////////PUBLIC FUNCTIONS

memoree_t memoree_init(memoree_variant_t variant, void *interface_conf)
{
  if (!VARIANT_ISVALID(variant) || !interface_conf)
    return NULL;

  memoree_interface_t interface = NULL;
  uint32_t speed = ((memoree_periph_conf_t *)interface_conf)->speed;

  if (variant < MEMOREE_VARIANT_I2C_MAX)
  {
    speed = (speed > MEMOREE_I2C_MAX_SPEED) ? MEMOREE_I2C_MAX_SPEED : speed;
    ((memoree_i2c_conf_t *)interface_conf)->speed = speed;

    interface = platform_i2c_init((memoree_i2c_conf_t *)interface_conf);
    if (!interface)
      return NULL;
  }
  else
  {
    if (variant == MEMOREE_VARIANT_25XX_SFDP || variant == MEMOREE_VARIANT_STUB_SPI)
      speed = (speed > MEMOREE_SPI_MAX_SPEED) ? MEMOREE_SPI_MAX_SPEED : speed;
    else if (variant < MEMOREE_VARIANT_93CXX_MAX)
      speed = (speed > MEMOREE_SPI_93X_MAX_SPEED) ? MEMOREE_SPI_93X_MAX_SPEED : speed;

    ((memoree_spi_conf_t *)interface_conf)->speed = speed;

    interface = platform_spi_init((memoree_spi_conf_t *)interface_conf);
    if (!interface)
      return NULL;
  }

  memoree_t mem = malloc(sizeof(struct memoree));
  if (!mem)
    return NULL;

  memcpy(&mem->info, &mem_props[variant], sizeof(memoree_info_t));
  mem->interface = interface;

  if (mem->info.type == MEMOREE_TYPE_I2C)
    mem->info.addr = ((memoree_i2c_conf_t *)interface_conf)->addr;

  if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    sfdp_param_t param;

    if (memoree_get_sfdp(mem, &param, 100) != MEMOREE_ERR_OK)
    {
      memoree_deinit(mem, false);
      return NULL;
    }
  }

  mem->info.speed = speed;
  mem->info.num_pages = mem->info.size / mem->info.page_size;
  mem->info.protected = false;
  return mem;
}

memoree_err_t memoree_deinit(memoree_t mem, bool if_deinit)
{
  if (!mem || !mem->interface)
    return MEMOREE_ERR_INVALID_ARG;

  if (mem->info.variant != MEMOREE_VARIANT_STUB_I2C && mem->info.variant != MEMOREE_VARIANT_STUB_SPI)
    if (!MEMOREE_ISVALID(mem))
      return MEMOREE_ERR_INVALID_ARG;

  if (if_deinit)
  {
    switch (mem->info.type)
    {
    case MEMOREE_TYPE_I2C:
      if (platform_i2c_deinit(mem->interface) != 0)
        return MEMOREE_ERR_FAIL;
      break;
    case MEMOREE_TYPE_SPI:
      if (platform_spi_deinit(mem->interface) != 0)
        return MEMOREE_ERR_FAIL;
      break;
    default:
      return MEMOREE_ERR_INVALID_ARG;
      break;
    }
  }
  free(mem);
  mem = NULL;
  return MEMOREE_ERR_OK;
}

memoree_err_t memoree_ping(memoree_t mem, size_t timeout_ms)
{
  if (!MEMOREE_ISVALID(mem))
    return MEMOREE_ERR_INVALID_ARG;

  if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX)
    return (platform_i2c_ping(mem->interface, mem->info.addr, timeout_ms)) ? MEMOREE_ERR_OK : MEMOREE_ERR_FAIL;
  else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    sfdp_param_t param;
    return memoree_get_sfdp(mem, &param, timeout_ms);
  }

  return MEMOREE_ERR_INVALID_ARG;
}

memoree_err_t memoree_read_byte(memoree_t mem, uint32_t addr, uint8_t *data, size_t timeout_ms)
{
  memoree_err_t ret = memoree_read(mem, addr, data, 1, timeout_ms);
  return (ret == 1) ? MEMOREE_ERR_OK : ret;
}

memoree_err_t memoree_write_byte(memoree_t mem, uint32_t addr, uint8_t data, size_t timeout_ms)
{

  if (!MEMOREE_ISVALID(mem) || !ADDRESS_ISVALID(mem, addr))
    return MEMOREE_ERR_INVALID_ARG;

  int ret;
  if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX)
    ret = _memoree_write_bytes(mem, addr, &data, 1, timeout_ms);
  else
  {
    if (_memoree_spi_write_enable(mem) != MEMOREE_ERR_OK)
      return MEMOREE_ERR_FAIL;

    memoree_spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.addr_len = mem->info.addr_len;
    t.addr = (addr & (mem->info.size - 1));
    t.write_len = 1;
    t.write_buff = &data;
    t.timeout_ms = timeout_ms ? timeout_ms : 0;

    if (mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
    {
      t.cmd_len = 3;
      t.cmd = MEMOREE_CMD_93CXX_WRITE;
    }
    else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
    {
      t.cmd_len = 8;
      t.cmd = MEMOREE_CMD_25XX_PP;
    }

    ret = platform_spi_write_read(mem->interface, &t);
  }

  return ret;
}

int memoree_read(memoree_t mem, uint32_t addr, uint8_t *data, uint32_t data_len, size_t timeout_ms)
{
  if (!MEMOREE_ISVALID(mem) && !ADDRESS_ISVALID(mem, addr) || !data)
    return MEMOREE_ERR_INVALID_ARG;

  int ret = MEMOREE_ERR_FAIL;

  if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX)
  {
    uint8_t write_buffer[mem->info.addr_len / 8];
    uint8_t *write_p = write_buffer;
    int16_t addr_space = _get_address_space(mem);
    if (addr_space <= 0)
      return MEMOREE_ERR_INVALID_ARG;

    uint8_t i2c_address;

    if (addr_space == mem->info.addr_len)
      i2c_address = mem->info.addr;
    else
      i2c_address = mem->info.addr | (((addr >> mem->info.addr_len) << 1) & 0x0F);

    int i = mem->info.addr_len;

    do
    {
      i -= 8;
      *write_p++ = addr >> i;
    } while (i > 0);

    ret = platform_i2c_write_read(mem->interface, i2c_address, write_buffer, sizeof(write_buffer), data, data_len, timeout_ms);
  }
  else if (mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
  {
    memoree_spi_transaction_t t =
        {
            .cmd_len = 3,
            .cmd = MEMOREE_CMD_93CXX_READ,
            .addr_len = mem->info.addr_len,
            .addr = (addr & (mem->info.size - 1)),
            .read_len = data_len,
            .read_buff = data,
            .timeout_ms = timeout_ms,
        };

    ret = platform_spi_write_read(mem->interface, &t);
  }
  else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    memoree_spi_transaction_t t =
        {
            .cmd_len = 8,
            .cmd = MEMOREE_CMD_25XX_READ,
            .addr_len = mem->info.addr_len,
            .addr = (addr & (mem->info.size - 1)),
            .read_len = data_len,
            .read_buff = data,
            .timeout_ms = timeout_ms,
        };

    ret = platform_spi_write_read(mem->interface, &t);
  }

  return (ret < 0) ? ret : data_len;
}

int memoree_write(memoree_t mem, uint32_t addr, uint8_t *data, uint32_t data_len, size_t timeout_ms, bool wrap)
{
  if (!MEMOREE_ISVALID(mem) && !ADDRESS_ISVALID(mem, addr) || !data)
    return MEMOREE_ERR_INVALID_ARG;

  int32_t bytes_written = 0;
  int64_t overflow = (int64_t)(addr + data_len) - (int64_t)(mem->info.size);

  if (mem->info.variant > MEMOREE_VARIANT_I2C_MAX && mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
  {
    if (_memoree_spi_write_enable(mem) != MEMOREE_ERR_OK)
      return MEMOREE_ERR_FAIL;

    memoree_spi_transaction_t t = {
        .cmd_len = 3,
        .cmd = MEMOREE_CMD_93CXX_WRITE,
        .addr_len = mem->info.addr_len,
        .addr = (addr & (mem->info.size - 1)),
        .write_len = 1,
        .write_buff = data,
        .timeout_ms = timeout_ms ? timeout_ms / data_len : 0,
    };

    // Write one byte at a time
    uint32_t bytes_to_write = data_len;
    int8_t ret = 0;
    do
    {
      ret = platform_spi_write_read(mem->interface, &t);
      t.addr++;
      t.write_buff++;
      platform_ms_delay(mem->info.page_write_delay_ms);
    } while (ret == 0 && --bytes_to_write);

    return (ret == 0) ? data_len : MEMOREE_ERR_FAIL;
  }
  else if (mem->info.variant < MEMOREE_VARIANT_I2C_MAX || mem->info.variant == MEMOREE_VARIANT_25XX_SFDP)
  {
    // Perform page address translation and overflow handling
    if (overflow > 0)
    {
      if (wrap)
      {
        bytes_written = memoree_write(mem, addr, data, data_len - overflow, timeout_ms, false);
        if (bytes_written != data_len - overflow)
          return bytes_written;
        bytes_written = memoree_write(mem, 0, data + bytes_written,
                                      overflow, timeout_ms, false);
        if (bytes_written != overflow)
          return bytes_written;

        return data_len;
      }
      else
        data_len -= (addr + data_len) % (mem->info.size);
    }

    uint16_t alignment_bytes = mem->info.page_size - (addr % mem->info.page_size);
    uint16_t interations = (data_len - alignment_bytes) / mem->info.page_size;

    // Write the first few bytes so that the next address is at the start of a page
    bytes_written = _memoree_write_bytes(mem, addr, data, alignment_bytes, MEMOREE_DEFAULT_TIMEOUT(mem, alignment_bytes));
    platform_ms_delay(mem->info.page_write_delay_ms);
    if (bytes_written < 0)
      return MEMOREE_ERR_FAIL;

    addr += alignment_bytes;
    data += alignment_bytes;

    for (uint16_t i = 0; i < interations; i++)
    {
      bytes_written = _memoree_write_bytes(mem, addr + (mem->info.page_size * i), data + (mem->info.page_size * i), mem->info.page_size, MEMOREE_DEFAULT_TIMEOUT(mem, mem->info.page_size));
      platform_ms_delay(mem->info.page_write_delay_ms * 2);
      if (bytes_written < 0)
        return MEMOREE_ERR_FAIL;
    }
  }

  return data_len;
}

memoree_err_t memoree_erase_page(memoree_t mem, uint32_t page, uint8_t erase_value)
{
  if (!PAGE_ISVALID(mem, page))
    return MEMOREE_ERR_INVALID_ARG;

  int ret;

  if (mem->info.variant > MEMOREE_VARIANT_I2C_MAX && mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
  {
    memoree_spi_transaction_t t = {
        .cmd_len = 5,
        .addr_len = 5,
        .dummy_len = 0,
        .read_len = 0,
        .read_buff = NULL,
        .write_len = 0,
        .write_buff = NULL,
        .timeout_ms = 0,
    };

    if (erase_value == 0xFF)
      t.cmd = MEMOREE_CMD_93CXX_ERASE;
    else
    {
      t.cmd = MEMOREE_CMD_93CXX_WRITE;
      t.write_len = 1;
      t.write_buff = &erase_value;
    }

    ret = platform_spi_write_read(mem->interface, &t);
    platform_ms_delay(mem->info.page_write_delay_ms);

    return ret;
  }

  uint32_t erase_buff_size = mem->info.page_size;
  uint8_t erase_buff[erase_buff_size];
  memset(erase_buff, erase_value, sizeof(erase_buff));
  ret = _memoree_write_bytes(mem, page * erase_buff_size, erase_buff, erase_buff_size, MEMOREE_DEFAULT_TIMEOUT(mem, erase_buff_size));

  return (ret == erase_buff_size) ? MEMOREE_ERR_OK : ret;
}

memoree_err_t memoree_erase(memoree_t mem, uint8_t erase_value)
{
  if (!MEMOREE_ISVALID(mem))
    return MEMOREE_ERR_INVALID_ARG;
  int ret;
  if (mem->info.variant > MEMOREE_VARIANT_I2C_MAX && mem->info.variant < MEMOREE_VARIANT_93CXX_MAX)
  {
    if (_memoree_spi_write_enable(mem) != MEMOREE_ERR_OK)
      return MEMOREE_ERR_FAIL;

    memoree_spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    if (erase_value == 0xFF)
    {
      t.cmd_len = 5;
      t.cmd = MEMOREE_CMD_93CXX_ERAL;
      t.addr_len = 5;

      ret = platform_spi_write_read(mem->interface, &t);
      platform_ms_delay(mem->info.page_write_delay_ms);

      return (ret == 0) ? MEMOREE_ERR_OK : ret;
    }

    t.cmd_len = 3;
    t.cmd = MEMOREE_CMD_93CXX_WRITE;
    t.addr_len = mem->info.addr_len;
    t.write_len = 1;
    t.write_buff = &erase_value;

    // Write one byte at a time
    uint32_t bytes_to_write = mem->info.size;
    ret = 0;
    do
    {
      ret = platform_spi_write_read(mem->interface, &t);
      t.addr++;
      platform_ms_delay(mem->info.page_write_delay_ms);
    } while (ret == 0 && --bytes_to_write);

    return ret;
  }
  else if (mem->info.variant == MEMOREE_VARIANT_25XX_SFDP && erase_value == 0xFF)
  {
    sfdp_param_t param;
    if ((ret = memoree_get_sfdp(mem, &param, 100)) != MEMOREE_ERR_OK)
      return ret;

    // If 4k sector erase is supported
    if (param.erase4k_opcode)
    {
      memoree_spi_transaction_t t = {
          .cmd_len = 8,
          .cmd = param.erase4k_opcode,
          .addr_len = mem->info.addr_len,
      };

      for (t.addr = 0; t.addr < mem->info.size; t.addr += 4096)
      {
        ret = platform_spi_write_read(mem->interface, &t);
        platform_ms_delay(mem->info.page_write_delay_ms);

        return (ret == 0) ? MEMOREE_ERR_OK : ret;
      }
    }
  }

  uint32_t erase_buff_size = mem->info.page_size;
  uint8_t erase_buff[erase_buff_size];
  memset(erase_buff, erase_value, erase_buff_size);
  uint16_t page = 0;
  for (; page < mem->info.num_pages; page++)
  {
    int ret = _memoree_write_bytes(mem, page * erase_buff_size, erase_buff, erase_buff_size, MEMOREE_DEFAULT_TIMEOUT(mem, erase_buff_size));
    platform_ms_delay(mem->info.page_write_delay_ms);
    if (ret != erase_buff_size)
      return ret;
  }

  return (page == mem->info.num_pages) ? MEMOREE_ERR_OK : MEMOREE_ERR_FAIL;
}

memoree_err_t memoree_get_info(memoree_t mem, memoree_info_t *mem_info)
{
  if (!MEMOREE_ISVALID(mem) || !mem_info)
    return MEMOREE_ERR_INVALID_ARG;

  mem_info->type = mem->info.type;
  mem_info->variant = mem->info.variant;
  mem_info->size = mem->info.size;
  mem_info->speed = mem->info.speed;
  mem_info->addr_len = mem->info.addr_len;
  mem_info->addr = mem->info.addr;
  mem_info->page_size = mem->info.page_size;
  mem_info->num_pages = mem->info.num_pages;
  mem_info->page_write_delay_ms = mem->info.page_write_delay_ms;
  mem_info->protected = mem->info.protected;

  return MEMOREE_ERR_OK;
}

memoree_err_t memoree_protect(memoree_t mem, memoree_protection_t protection)
{
  if (!MEMOREE_ISVALID(mem) || mem->info.variant != MEMOREE_VARIANT_25XX_SFDP)
    return MEMOREE_ERR_INVALID_ARG;

  return MEMOREE_ERR_FAIL;
}

memoree_err_t memoree_get_sfdp(memoree_t mem, sfdp_param_t *param, size_t timeout_ms)
{
  if (!MEMOREE_ISVALID(mem) || !param || mem->info.variant != MEMOREE_VARIANT_25XX_SFDP)
    return false;

  int ret = 0;
  uint8_t read_buff[15];
  memset(read_buff, 0xFF, sizeof(read_buff));
  memoree_spi_transaction_t t = {
      .cmd_len = 8,
      .cmd = MEMOREE_CMD_25XX_SFDP,
      .addr_len = 24,
      .addr = 0,
      .write_len = sizeof(read_buff),
      .write_buff = read_buff,
      .read_len = sizeof(read_buff),
      .read_buff = read_buff,
      .dummy_len = 8,
      .timeout_ms = timeout_ms ? timeout_ms : 0,
  };

  ret = platform_spi_write_read(mem->interface, &t);
  if (ret != 0)
    return MEMOREE_ERR_FAIL;

  if (read_buff[0] != 'S' || read_buff[1] != 'F' || read_buff[2] != 'D' || read_buff[3] != 'P')
    return MEMOREE_ERR_SFDP_NOT_SUPPORTED;

  // Byte 7 is unused and must be 0xFF && Flash parameter table must at contain the flash memory size (the second double word,bytes 5-8)
  if (read_buff[7] != 0xFF || read_buff[11] < 2)
    return MEMOREE_ERR_SFDP_INVALID_HEADER;

  param->header_ver = (read_buff[5] << 8) | read_buff[4];
  param->header_cnt = read_buff[6] + 1; // SFDP header count is zero indexed
  param->fparam_ver = (read_buff[10] << 8) | read_buff[9];
  param->fparam_size = read_buff[11] * 4;
  param->fparam_ptr = (read_buff[14] << 16) | (read_buff[13] << 8) | read_buff[12];

  uint8_t sfdp_table[param->fparam_size];
  memset(sfdp_table, 0xFF, param->fparam_size);
  t.addr = param->fparam_ptr;
  t.write_len = t.read_len = sizeof(sfdp_table);
  t.write_buff = t.read_buff = sfdp_table;

  ret = platform_spi_write_read(mem->interface, &t);
  if (ret != 0)
    return MEMOREE_ERR_FAIL;

  if (((sfdp_table[0] >> 5) & 0b111) != 0b111 || ((sfdp_table[2] >> 7) & 0b1) != 0b1 || sfdp_table[3] != 0xFF)
    return MEMOREE_ERR_SFDP_INVALID_HEADER;

  param->write_size = (sfdp_table[0] >> 2 & 0b1) ? 64 : 1;
  param->erase4k_opcode = (sfdp_table[0] & 0b11) == 0b11 ? 0 : sfdp_table[1];
  param->addr_bytes = ((sfdp_table[2] >> 1 & 0b11) == 0b00) ? 3 : ((sfdp_table[2] >> 1 & 0b11) == 0b10) ? 4
                                                                                                        : 0;
  param->dtr_support = (sfdp_table[2] >> 3 & 0b1);
  param->min_sector = ((sfdp_table[0] & 0b11) == 01) ? 4096 : 0;

  uint64_t flash_size = (sfdp_table[7] & 0x7F) << 24 |
                        sfdp_table[6] << 16 |
                        sfdp_table[5] << 8 |
                        sfdp_table[4] << 0;

  param->size = ((sfdp_table[7] >> 7) & 0b1) ? 2 << (flash_size - 1) : flash_size;
  param->size >>= 3;

  mem->info.addr_len = param->addr_bytes * 8;
  mem->info.page_size = param->write_size;
  mem->info.size = param->size;

  return MEMOREE_ERR_OK;
}

int memoree_stub_read(memoree_t mem, memoree_stub_transaction_t *t)
{
  if (!mem || !mem->interface || !VARIANT_ISSTUB(mem->info.variant) || !t)
    return MEMOREE_ERR_INVALID_ARG;

  int ret = 0;

  if (mem->info.variant == MEMOREE_VARIANT_STUB_I2C)
    ret = platform_i2c_write_read(mem->interface, (uint8_t)t->addr, t->write_buff,
                                  t->write_len, t->read_buff, t->read_len, t->timeout_ms);
  else
    ret = platform_spi_write_read(mem->interface, t);

  return ret;
}