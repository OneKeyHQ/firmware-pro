#include <string.h>

#include "display.h"
#include "pn532.h"
#include "pn532_functions.h"
#include "systick.h"
#include "nfc.h"

void nfc_init(void)
{
    pn532_init();
}

bool nfc_pwr_ctl(bool on_off)
{
    if ( !on_off )
    {
        return pn532_pwr_ctl(false) == NFC_RESULT_OK;
    }

    if ( pn532_pwr_ctl(true) != NFC_RESULT_OK )
    {
        return false;
    }
    // Callers power on and then poll, so power-on also enters reader mode.
    if ( pn532_mode_ctrl(NFC_MODE_READER, true) != NFC_RESULT_OK )
    {
        return false;
    }
    return pn532_rsession_field(true) == NFC_RESULT_OK;
}

bool nfc_poll_card(void)
{
    NfcActivationInfo_t info = {0};

    // Re-enter reader mode from whatever state we are in (drops a stale
    // TAG-mode or half-open session), then run one short activation attempt -
    // the UI drives this at ~1 Hz until a card shows up.
    if ( pn532_mode_ctrl(NFC_MODE_READER, true) != NFC_RESULT_OK )
    {
        return false;
    }
    (void)pn532_rsession_field(false);
    if ( pn532_rsession_field(true) != NFC_RESULT_OK )
    {
        return false;
    }
    if ( pn532_rsession_activate(&info, NFC_POLL_TIMEOUT_MS) != NFC_RESULT_OK )
    {
        return false;
    }

    // RF gain boost, applied after every successful activation since the register
    // resets with the field. A failed gain write is not a poll failure: the card
    // is already activated, and a later APDU surfaces any real coupling problem.
    uint8_t rfcfg = 0x39;
    (void)WriteRegister(PN532_REG_CIU_RFCfg, &rfcfg);
    dwt_delay_ms(20);
    return true;
}

bool nfc_send_recv(
    uint8_t* send, uint16_t send_len, uint8_t* response, uint16_t* response_len, uint8_t* sw1sw2
)
{
    // *response_len is IN/OUT: on entry the caller's buffer capacity, on exit
    // the response length with SW1SW2 stripped. The buffer must hold the data
    // field plus the two-byte SW; the underlying transceive rejects (does not
    // truncate) an over-capacity card response, so an oversized response from
    // an untrusted card fails cleanly instead of overflowing the buffer.
    if ( response_len == NULL || *response_len == 0 )
    {
        return false;
    }

    uint16_t recv_len = *response_len;
    if ( pn532_rsession_transceive(send, send_len, response, &recv_len, NFC_DATA_EXCHANGE_TIMEOUT_MS) !=
         NFC_RESULT_OK )
    {
        return false;
    }

    if ( recv_len < 2 )
    {
        return false;
    }

    if ( sw1sw2 != NULL )
    {
        sw1sw2[0] = response[recv_len - 2];
        sw1sw2[1] = response[recv_len - 1];
    }

    // Data field stays in place at the front of the caller's buffer; only the
    // trailing SW is dropped from the reported length.
    *response_len = recv_len - 2;
    return true;
}

bool nfc_select_aid(uint8_t* aid, uint8_t aid_len)
{
    uint8_t apdu_select[64] = {0x00, 0xA4, 0x04, 0x00, 0x00};
    uint8_t response[64] = {0};
    uint16_t response_len = 64;
    uint8_t sw1sw2[2] = {0};
    // apdu_select holds a 5-byte header + AID; aid_len comes from outside this
    // module, so bound it to the buffer before the copy.
    if ( aid == NULL || aid_len > sizeof(apdu_select) - 5 )
    {
        return false;
    }
    apdu_select[4] = aid_len;
    memcpy(apdu_select + 5, aid, aid_len);
    if ( nfc_send_recv(apdu_select, aid_len + 5, response, &response_len, sw1sw2) )
    {
        if ( sw1sw2[0] == 0x90 && sw1sw2[1] == 0x00 )
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}
