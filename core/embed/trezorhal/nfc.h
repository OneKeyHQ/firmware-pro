#ifndef __NFC_H__
#define __NFC_H__

#include "pn532.h"
#include "spi.h"

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
bool nfc_pwr_ctl(bool on_off);
bool nfc_send_recv(
    uint8_t* send, uint16_t send_len, uint8_t* response, uint16_t* response_len, uint8_t* sw1sw2
);
bool nfc_poll_card(void);
bool nfc_select_aid(uint8_t* aid, uint8_t aid_len);
#endif // __NFC_H__
