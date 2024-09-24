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

bool nfc_read_card_certificate(void)
{
    uint8_t cert[256] = {0};
    uint16_t cert_len = sizeof(cert);
    uint8_t apdu_get_sd_certificates[] = {0x80, 0xca, 0xbf, 0x21, 0x06, 0xa6, 0x04, 0x83, 0x02, 0x15, 0x18};
    uint8_t sw1sw2[2] = {0};
    if ( !nfc_send_recv(apdu_get_sd_certificates, sizeof(apdu_get_sd_certificates), cert, &cert_len, sw1sw2) )
    {
        return false;
    }
    if ( sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00 )
    {
        return false;
    }

    return true;
}

bool nfc_send_device_certificate(void)
{
    const static uint8_t device_certificate[] = {
        "\x7F\x21\x81\xDB\x93\x10\x43\x45\x52\x54\x5F\x4F\x43\x45\x5F\x45\x43\x4B"
        "\x41\x30\x30\x31\x42\x0D\x6A\x75\x62\x69\x74\x65\x72\x77\x61\x6C\x6C\x65"
        "\x74\x5F\x20\x0D\x6A\x75\x62\x69\x74\x65\x72\x77\x61\x6C\x6C\x65\x74\x95"
        "\x02\x00\x80\x5F\x25\x04\x20\x20\x05\x25\x5F\x24\x04\x20\x25\x05\x24\x53"
        "\x00\xBF\x20\x00\x7F\x49\x46\xB0\x41\x04\x08\xCC\xB4\x9E\xB9\x10\x57\x28"
        "\x75\x72\xE6\x87\x06\xF3\xCB\x4C\x27\xCE\x19\xAD\x94\xC4\x0B\x2A\x37\xC5"
        "\x94\xE5\x1B\xC0\x9E\xAD\x96\x34\x94\x66\x30\x6C\x58\x63\xF6\xE8\xBE\xB3"
        "\xF0\xEA\x99\x71\x18\x48\x16\x32\x01\xBF\xE8\xC7\x88\x43\x3D\x45\x81\x64"
        "\x69\xE5\xF0\x01\x00\x5F\x37\x47\x30\x45\x02\x21\x00\x87\x9E\xEB\x7E\xE0"
        "\x96\x2B\x44\xBD\x3D\x87\x01\x16\x1A\x26\x34\x77\xCC\x2F\x08\xD7\x68\x1A"
        "\xF8\x54\x6F\xBC\x17\xEB\x3E\x99\x65\x02\x20\x16\x00\xFA\x7A\x74\x1B\x0E"
        "\xFE\x7C\x14\x3D\x73\x71\x3E\x80\x31\xAF\xBB\x3F\x1C\x0B\x6D\x69\x04\x80"
        "\x20\xD2\x73\xE4\x8A\xAF\x5E"};

    uint8_t apdu_send_device_cert[256] = {0x80, 0x2a, 0x18, 0x10, 0x00};
    uint8_t resp[2] = {0};
    uint16_t resp_len = sizeof(resp);
    uint8_t sw1sw2[2] = {0};
    apdu_send_device_cert[4] = sizeof(device_certificate) - 1;
    memcpy(apdu_send_device_cert + 5, device_certificate, apdu_send_device_cert[4]);
    if ( !nfc_send_recv(apdu_send_device_cert, apdu_send_device_cert[4] + 5, resp, &resp_len, sw1sw2) )
    {
        return false;
    }
    if ( sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00 )
    {
        return false;
    }
    return true;
}