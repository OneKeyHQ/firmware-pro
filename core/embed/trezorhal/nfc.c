#include <string.h>

#include "display.h"
#include "pn532.h"
#include "nfc.h"

static bool nfc_powered = false;

void nfc_init(void)
{
    pn532_init();
}

bool nfc_pwr_ctl(bool on_off)
{
    if ( on_off == nfc_powered )
    {
        return true;
    }
    pn532_power_ctl(on_off);
    if ( on_off )
    {
        return pn532_SAMConfiguration();
    }
    return true;
}

bool nfc_send_recv(
    uint8_t* send, uint16_t send_len, uint8_t* response, uint16_t* response_len, uint8_t* sw1sw2
)
{
    if ( !pn532_inDataExchange(send, send_len, response, response_len) )
    {
        return false;
    }

    if ( sw1sw2 != NULL )
    {
        sw1sw2[0] = response[*response_len - 2];
        sw1sw2[1] = response[*response_len - 1];
    }

    *response_len -= 2;
    return true;
}

bool nfc_poll_card(void)
{
    return pn532_inListPassiveTarget();
}

bool nfc_select_aid(uint8_t* aid, uint8_t aid_len)
{
    uint8_t apdu_select[64] = {0x00, 0xA4, 0x04, 0x00, 0x00};
    uint8_t response[64] = {0};
    uint16_t response_len = 64;
    uint8_t sw1sw2[2] = {0};
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

NFC_STATUS nfc_get_status(uint8_t* status)
{
    return pn532_tgGetStatus(status) ? NFC_STATUS_OPERACTION_SUCCESS : NFC_STATUS_OPERACTION_FAILED;
}
