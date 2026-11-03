#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DEVICE_TRANSFER_IO_WAIT = 0,
    DEVICE_TRANSFER_IO_DONE,
    DEVICE_TRANSFER_IO_TIMEOUT,
    DEVICE_TRANSFER_IO_ERROR,
} device_transfer_io_result_t;

// SPI-level ACK of a command frame; unrelated to RF timing. In card emulation a
// TgGetData written right after a TgSetData can be dropped by the chip (RDY stays
// 0); the same frame written again is taken at once. The reader gives up ~4 FWT
// (~640 ms) after its block, so the retry has to come well inside that.
#define DEVICE_TRANSFER_ACK_TIMEOUT_MS 100
#define DEVICE_TRANSFER_ACK_ATTEMPTS   3

// One PN532 command stays pending across UI ticks; polling never aborts it.
bool device_transfer_io_start(uint8_t command, const uint8_t* data, uint16_t length, uint32_t timeout_ms);
device_transfer_io_result_t device_transfer_io_poll(uint8_t* data, uint16_t* length);
void device_transfer_io_close(void);
