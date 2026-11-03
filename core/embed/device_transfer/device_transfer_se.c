#include "device_transfer_se.h"

#include <string.h>
#include "memzero.h"
#include "se_thd89.h"

secbool device_transfer_seal_mnemonic(
    const uint8_t* mnemonic, uint16_t mnemonic_len, const uint8_t r_xy[64], uint8_t* envelope,
    uint16_t* envelope_len
)
{
    if ( envelope == NULL || envelope_len == NULL )
        return secfalse;
    const uint16_t capacity = *envelope_len;
    *envelope_len = 0;
    if ( mnemonic == NULL || r_xy == NULL || mnemonic_len == 0 ||
         mnemonic_len > DEVICE_TRANSFER_MNEMONIC_MAX_LEN || memchr(mnemonic, '\0', mnemonic_len) != NULL )
    {
        memzero(envelope, capacity);
        return secfalse;
    }

    uint8_t data[1 + DEVICE_TRANSFER_MNEMONIC_MAX_LEN + 64] = {0};
    data[0] = (uint8_t)mnemonic_len;
    memcpy(data + 1, mnemonic, mnemonic_len);
    memcpy(data + 1 + mnemonic_len, r_xy, 64);
    *envelope_len = capacity;
    secbool result =
        se_transmit_mac(0xE2, 0x00, 0x09, data, (uint16_t)(65 + mnemonic_len), envelope, envelope_len);
    memzero(data, sizeof(data));
    // ENV = 01 01 || D64 || IV16 || ciphertext (16..256 bytes) || Tag16.
    if ( result != sectrue || *envelope_len > capacity || *envelope_len < 114 || *envelope_len > 354 ||
         (*envelope_len - 98) % 16 != 0 || envelope[0] != 0x01 || envelope[1] != 0x01 )
    {
        memzero(envelope, capacity);
        *envelope_len = 0;
        return secfalse;
    }
    return sectrue;
}
