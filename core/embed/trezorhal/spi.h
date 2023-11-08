#ifndef __SPI_H__
#define __SPI_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include STM32_HAL_H

#define SPI_CHANNEL_TOTAL 3

typedef enum SPI_DEVICE
{
    SPI_BLUETOOTH,
    SPI_FINGERPRINT,
    SPI_NFC,
} spi_device;

typedef enum SPI_CAHNNEL
{
    SPI_2, // BLUETOOTH
    SPI_3, // FINGERPRINT
    SPI_6, // NFC
    SPI_UNKNOW = -1,
} spi_channel;

// handles
extern SPI_HandleTypeDef spi_handles[SPI_CHANNEL_TOTAL];

// init status
extern bool spi_status[SPI_CHANNEL_TOTAL];

// init function and arrays
bool SPI_2_INIT();
bool SPI_2_DEINIT();
bool SPI_3_INIT();
bool SPI_3_DEINIT();
bool SPI_6_INIT();
bool SPI_6_DEINIT();
typedef bool (*spi_init_function_t)(void);
extern spi_init_function_t spi_init_function[SPI_CHANNEL_TOTAL];
typedef bool (*spi_deinit_function_t)(void);
extern spi_deinit_function_t spi_deinit_function[SPI_CHANNEL_TOTAL];

// helper functions

bool spi_deinit_by_bus(spi_channel master);

spi_channel spi_find_channel_by_device(spi_device device);
bool is_spi_initialized_by_device(spi_device device);
bool spi_init_by_device(spi_device device);
bool spi_deinit_by_device(spi_device device); // make sure you understand what you doing!

#endif // __SPI_H__