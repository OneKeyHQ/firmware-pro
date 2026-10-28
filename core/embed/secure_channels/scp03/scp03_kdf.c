#include <stdint.h>
#include <string.h>

#include "../secure_channel_port.h"

#include "scp03_internal.h"

// ########################################################################
// defines

#define ExecuteCheck__E__sectrue__Rof__secfalse(func_call) \
  ExecuteCheck_ADV(func_call, sectrue, { return secfalse; })

// ########################################################################
// public functions

// NIST SP 800-108 KDF in counter mode, PRF = AES-CMAC
// input block: [00*11][DDC][00][L_hi][L_lo][counter][context]
void sp2p_scp03_pad80(const uint8_t* in_p, uint32_t in_len, uint8_t* out_p, uint32_t* out_len_p)
{
    memcpy(out_p, in_p, in_len);
    out_p[in_len] = 0x80;
    uint32_t padded = ((in_len + 16) / 16) * 16;
    memset(out_p + in_len + 1, 0x00, padded - in_len - 1);
    *out_len_p = padded;
}

secbool_t sp2p_scp03_unpad80(const uint8_t* in_p, uint32_t in_len, uint32_t* data_len_p)
{
    if ( in_len == 0 || (in_len % SECURE_CHANNEL_SCP03_BLOCK_LEN) != 0 )
    {
        return secfalse;
    }
    for ( uint32_t i = in_len; i > 0; i-- )
    {
        if ( in_p[i - 1] == 0x80 )
        {
            *data_len_p = i - 1;
            return sectrue;
        }
        if ( in_p[i - 1] != 0x00 )
        {
            return secfalse;
        }
    }
    return secfalse;
}

void sp2p_scp03_increment_counter(uint8_t counter[16])
{
    for ( int32_t i = 15; i >= 0; i-- )
    {
        if ( ++counter[i] != 0 )
        {
            break;
        }
    }
}

secbool_t secure_channel_scp03_session_init_keys(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* s_enc_p, const uint8_t* s_mac_p,
    const uint8_t* s_rmac_p, uint32_t key_len, const uint8_t mac_chaining[SECURE_CHANNEL_SCP03_BLOCK_LEN],
    uint8_t security_level
)
{
    if ( session_p == NULL || s_enc_p == NULL || s_mac_p == NULL || s_rmac_p == NULL )
    {
        return secfalse;
    }
    if ( key_len != 16 && key_len != 24 && key_len != 32 )
    {
        return secfalse;
    }

    memset(session_p, 0, sizeof(*session_p));
    session_p->key_len = key_len;
    session_p->security_level = security_level;
    memcpy(session_p->s_enc, s_enc_p, key_len);
    memcpy(session_p->s_mac, s_mac_p, key_len);
    memcpy(session_p->s_rmac, s_rmac_p, key_len);
    if ( mac_chaining != NULL )
    {
        memcpy(session_p->mac_chaining, mac_chaining, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    }

    return sectrue;
}
