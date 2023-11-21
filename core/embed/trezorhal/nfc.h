#ifndef __NFC_H__
#define __NFC_H__

#include "pn532.h"
#include "spi.h"

#define spi_handle_nfc spi_handles[spi_find_channel_by_device(SPI_NFC)]

typedef enum
{
    NFC_STATUS_SUCCESS = 0,
    NFC_STATUS_TIME_OUT = -1,
    NFC_STATUS_OPERACTION_FAILED = -2,
    NFC_STATUS_NOT_INITIALIZED = -3,
} NFC_STATUS;

void nfc_init(void);
NFC_STATUS nfc_send_recv_apdu(
    uint8_t* send, uint8_t sendLength, uint8_t* response, uint8_t* responseLength, uint32_t timeout_ms
);

#endif // __NFC_H__