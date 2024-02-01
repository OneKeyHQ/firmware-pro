#ifndef TREZORHAL_QSPI_FLASH_
#define TREZORHAL_QSPI_FLASH_

#include STM32_HAL_H

#include "qspi_flash_defines.h"

#define QSPI_FLASH_BASE_ADDRESS 0x90000000

extern QSPIFlash_t* qspi_flash;
extern QSPIFlahs_Info_t* qspi_flash_info;

// functions
bool qspi_flash_init(void);
bool qspi_flash_deinit(void);
bool qspi_flash_probe(void);

bool qspi_flash_set_memory_map(bool mapped);
bool qspi_flash_erase_chip();
bool qspi_flash_erase(uint32_t address, uint32_t len);
bool qspi_flash_read_buffer(uint8_t* data, uint32_t address, uint32_t len);
bool qspi_flash_write_buffer(uint8_t* data, uint32_t address, uint32_t len);

bool qspi_flash_test();

#endif