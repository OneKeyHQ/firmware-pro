// Internal helpers carry an sp2p_ prefix to avoid clashing with manage_card/scp03*.
#pragma once

#include "scp03.h"

// ########################################################################
// defines

// max buffer for CMAC input: 16 (chaining) + MAX_DATA_LEN + 16 (padding)
#define SCP03_MAC_BUF_LEN \
  (SECURE_CHANNEL_SCP03_BLOCK_LEN + SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN)

// ########################################################################
// private functions

static inline secbool_t scp03_secure_compare(const uint8_t* a_p, const uint8_t* b_p, uint32_t len)
{
    volatile uint8_t diff = 0;
    for ( uint32_t i = 0; i < len; i++ )
    {
        diff |= a_p[i] ^ b_p[i];
    }
    return diff == 0 ? sectrue : secfalse;
}

void sp2p_scp03_pad80(const uint8_t* in_p, uint32_t in_len, uint8_t* out_p, uint32_t* out_len_p);
secbool_t __wur sp2p_scp03_unpad80(const uint8_t* in_p, uint32_t in_len, uint32_t* data_len_p);

void sp2p_scp03_increment_counter(uint8_t counter[16]);
