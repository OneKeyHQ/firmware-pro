#pragma once

#include <stdint.h>
#include "secbool.h"

#define DEVICE_TRANSFER_MNEMONIC_MAX_LEN 240

// 84 E2 00 09: seal user-entered BIP39 text for the receiver's public key.
// The unlocked SE checks that it matches its wallet. No export switch or PIN
// is sent, and this command does not replace the mnemonic stored in the SE.
secbool device_transfer_seal_mnemonic(
    const uint8_t* mnemonic, uint16_t mnemonic_len, const uint8_t r_xy[64], uint8_t* envelope,
    uint16_t* envelope_len
);
