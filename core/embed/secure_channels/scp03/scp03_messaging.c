#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "../secure_channel_port.h"

#include "scp03_internal.h"

// ########################################################################
// defines

#define ExecuteCheck__E__sectrue__Rof__secfalse(func_call) \
  ExecuteCheck_ADV(func_call, sectrue, { return secfalse; })

// ########################################################################
// private functions

static secbool_t scp03_compute_mac_internal(
    const uint8_t* key_p, uint32_t key_len, const uint8_t chaining[16], const uint8_t* data_p,
    uint32_t data_len, uint8_t full_mac[16]
)
{
    uint8_t buf[SCP03_MAC_BUF_LEN];
    uint32_t buf_len = SECURE_CHANNEL_SCP03_BLOCK_LEN + data_len;
    if ( buf_len > sizeof(buf) )
    {
        return secfalse;
    }

    memcpy(buf, chaining, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    if ( data_len > 0 )
    {
        memcpy(buf + SECURE_CHANNEL_SCP03_BLOCK_LEN, data_p, data_len);
    }

    secbool_t ret = crypto_mac_cmac_aes_compute(key_p, key_len, buf, buf_len, full_mac);
    crypto_utils_memzero_explicit(buf, sizeof(buf));
    return ret;
}

static secbool_t scp03_encrypt_internal(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p, bool is_response
)
{
    if ( in_len == 0 )
    {
        *out_len_p = 0;
        return sectrue;
    }
    if ( in_len > SECURE_CHANNEL_SCP03_MAX_DATA_LEN )
    {
        return secfalse;
    }

    // ICV = AES_ECB(S-ENC, counter_block)
    uint8_t counter_block[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    memcpy(counter_block, session_p->enc_counter, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    if ( is_response )
    {
        counter_block[0] |= 0x80;
    }

    uint8_t icv[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    size_t icv_len;
    ExecuteCheck__E__sectrue__Rof__secfalse(crypto_cipher_aes_ecb_encrypt(
        session_p->s_enc, session_p->key_len, counter_block, SECURE_CHANNEL_SCP03_BLOCK_LEN, icv, &icv_len
    ));

    // pad80
    uint8_t padded[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];
    uint32_t padded_len;
    sp2p_scp03_pad80(in_p, in_len, padded, &padded_len);

    // AES-CBC encrypt
    size_t enc_out_len;
    secbool_t ret = crypto_cipher_aes_cbc_encrypt(
        session_p->s_enc, session_p->key_len, icv, padded, padded_len, out_p, &enc_out_len
    );
    *out_len_p = (uint32_t)enc_out_len;

    crypto_utils_memzero_explicit(padded, sizeof(padded));
    crypto_utils_memzero_explicit(icv, sizeof(icv));
    return ret;
}

static secbool_t scp03_decrypt_internal(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p, bool is_response
)
{
    if ( in_len == 0 )
    {
        *out_len_p = 0;
        return sectrue;
    }
    if ( in_len % SECURE_CHANNEL_SCP03_BLOCK_LEN != 0 )
    {
        return secfalse;
    }
    if ( in_len > SECURE_CHANNEL_SCP03_MAX_DATA_LEN )
    {
        return secfalse;
    }

    // ICV = AES_ECB(S-ENC, counter_block)
    uint8_t counter_block[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    memcpy(counter_block, session_p->enc_counter, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    if ( is_response )
    {
        counter_block[0] |= 0x80;
    }

    uint8_t icv[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    size_t icv_len;
    ExecuteCheck__E__sectrue__Rof__secfalse(crypto_cipher_aes_ecb_encrypt(
        session_p->s_enc, session_p->key_len, counter_block, SECURE_CHANNEL_SCP03_BLOCK_LEN, icv, &icv_len
    ));

    // AES-CBC decrypt
    uint8_t padded[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];
    size_t padded_len_sz;
    ExecuteCheck__E__sectrue__Rof__secfalse(crypto_cipher_aes_cbc_decrypt(
        session_p->s_enc, session_p->key_len, icv, in_p, in_len, padded, &padded_len_sz
    ));
    uint32_t padded_len = (uint32_t)padded_len_sz;

    // unpad80
    uint32_t data_len;
    secbool_t ret = sp2p_scp03_unpad80(padded, padded_len, &data_len);
    if ( ret == sectrue )
    {
        memcpy(out_p, padded, data_len);
        *out_len_p = data_len;
    }

    crypto_utils_memzero_explicit(padded, sizeof(padded));
    crypto_utils_memzero_explicit(icv, sizeof(icv));
    return ret;
}

// ########################################################################
// public functions

void secure_channel_scp03_destroy(SecureChannel_Scp03Session_t* session_p)
{
    crypto_utils_memzero_explicit(session_p, sizeof(*session_p));
}

// ########## command encryption ##########

secbool_t secure_channel_scp03_encrypt(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    return scp03_encrypt_internal(session_p, in_p, in_len, out_p, out_len_p, false);
}

secbool_t secure_channel_scp03_decrypt(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    return scp03_decrypt_internal(session_p, in_p, in_len, out_p, out_len_p, false);
}

// ########## response encryption ##########

secbool_t secure_channel_scp03_encrypt_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    return scp03_encrypt_internal(session_p, in_p, in_len, out_p, out_len_p, true);
}

secbool_t secure_channel_scp03_decrypt_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    return scp03_decrypt_internal(session_p, in_p, in_len, out_p, out_len_p, true);
}

// ########## C-MAC ##########

secbool_t secure_channel_scp03_mac(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t mac[8]
)
{
    uint8_t full_mac[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    ExecuteCheck__E__sectrue__Rof__secfalse(scp03_compute_mac_internal(
        session_p->s_mac, session_p->key_len, session_p->mac_chaining, data_p, data_len, full_mac
    ));

    memcpy(mac, full_mac, SECURE_CHANNEL_SCP03_MAC_LEN);
    memcpy(session_p->mac_chaining, full_mac, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    crypto_utils_memzero_explicit(full_mac, sizeof(full_mac));
    return sectrue;
}

secbool_t secure_channel_scp03_mac_verify(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, const uint8_t mac[8]
)
{
    uint8_t full_mac[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    ExecuteCheck__E__sectrue__Rof__secfalse(scp03_compute_mac_internal(
        session_p->s_mac, session_p->key_len, session_p->mac_chaining, data_p, data_len, full_mac
    ));

    secbool_t ret = scp03_secure_compare(full_mac, mac, SECURE_CHANNEL_SCP03_MAC_LEN);
    if ( ret == sectrue )
    {
        memcpy(session_p->mac_chaining, full_mac, SECURE_CHANNEL_SCP03_BLOCK_LEN);
    }
    crypto_utils_memzero_explicit(full_mac, sizeof(full_mac));
    return ret;
}

// ########## R-MAC ##########

secbool_t secure_channel_scp03_rmac(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t mac[8]
)
{
    uint8_t full_mac[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    ExecuteCheck__E__sectrue__Rof__secfalse(scp03_compute_mac_internal(
        session_p->s_rmac, session_p->key_len, session_p->mac_chaining, data_p, data_len, full_mac
    ));

    memcpy(mac, full_mac, SECURE_CHANNEL_SCP03_MAC_LEN);
    crypto_utils_memzero_explicit(full_mac, sizeof(full_mac));
    return sectrue;
}

secbool_t secure_channel_scp03_rmac_verify(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, const uint8_t mac[8]
)
{
    uint8_t full_mac[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    ExecuteCheck__E__sectrue__Rof__secfalse(scp03_compute_mac_internal(
        session_p->s_rmac, session_p->key_len, session_p->mac_chaining, data_p, data_len, full_mac
    ));

    secbool_t ret = scp03_secure_compare(full_mac, mac, SECURE_CHANNEL_SCP03_MAC_LEN);
    crypto_utils_memzero_explicit(full_mac, sizeof(full_mac));
    return ret;
}
