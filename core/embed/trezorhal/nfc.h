#ifndef __NFC_H__
#define __NFC_H__

#include "pn532.h"
#include "spi.h"

// Wall-clock bounds for the reader API below.
#define NFC_POLL_TIMEOUT_MS          300
#define NFC_DATA_EXCHANGE_TIMEOUT_MS 2000

// ########################################################################
// Reader API for the card flows (manage_card, trezorio.nfc, factory tests).

void nfc_init(void);
bool nfc_pwr_ctl(bool on_off);
// nfc_send_recv: *response_len is IN/OUT - on entry it MUST be the capacity
// of `response` (the buffer must hold the data field plus SW1SW2); on exit it
// is the response length with SW1SW2 stripped. An over-capacity card response
// is rejected (returns false), never truncated. SW1SW2 is returned separately.
bool nfc_send_recv(
    uint8_t* send, uint16_t send_len, uint8_t* response, uint16_t* response_len, uint8_t* sw1sw2
);
bool nfc_poll_card(void);
bool nfc_select_aid(uint8_t* aid, uint8_t aid_len);

// ########################################################################
// Mode control. See pn532.h for the state machine.

#define nfc_mode_ctrl pn532_mode_ctrl

#endif // __NFC_H__
