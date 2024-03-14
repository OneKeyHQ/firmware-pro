#ifndef __NFC_H__
#define __NFC_H__

#include "pn532.h"
#include "spi.h"

#define spi_handle_nfc            spi_handles[spi_find_channel_by_device(SPI_NFC)]

#define NFC_TIMEOUT_COUNT_STEP_MS 100

typedef enum
{
    NFC_STATUS_OPERACTION_SUCCESS = 0,
    NFC_STATUS_OPERACTION_FAILED = -1,
    NFC_STATUS_OPERACTION_TIMEOUT = -2,
    NFC_STATUS_NOT_INITIALIZED = -3,
    NFC_STATUS_INVALID_USAGE = -4,
    NFC_STATUS_UNDEFINED_ERROR = -99,
} NFC_STATUS;

void nfc_init(void);
NFC_STATUS nfc_pwr_ctl(bool on_off);
NFC_STATUS nfc_wait_card(uint32_t timeout_ms);
NFC_STATUS nfc_send_recv(uint8_t* send, uint16_t sendLength, uint8_t* response, uint16_t* responseLength);
NFC_STATUS nfc_send_recv_aio(
    uint8_t* send, uint16_t sendLength, uint8_t* response, uint16_t* responseLength, uint32_t timeout_ms
);

NFC_STATUS nfc_poll_card(void);
NFC_STATUS nfc_select_aid(uint8_t* aid, uint8_t aid_len);

void nfc_test();
void nfc_test_v2();

#endif // __NFC_H__
