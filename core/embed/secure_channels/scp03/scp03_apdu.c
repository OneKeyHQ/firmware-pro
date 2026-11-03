#include <stdint.h>
#include <string.h>

#include "../secure_channel_port.h"

#include "scp03_internal.h"
#include "scp03_apdu.h"

// ########################################################################
// defines

#define ExecuteCheck__E__sectrue__Rof__secfalse(func_call) \
  ExecuteCheck_ADV(func_call, sectrue, { return secfalse; })

#define APDU_HEADER_LEN 5

// ########################################################################
// private functions

// modify CLA for secure messaging: set bit 3, clear channel bits (0-1) and bit 4
static inline uint8_t scp03_apdu_secure_cla(uint8_t cla)
{
    return (cla & 0xE0) | 0x04;
}

// ########################################################################
// public functions

// ########## secure messaging APDU wrap/unwrap ##########

secbool_t secure_channel_scp03_apdu_wrap_command(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* apdu_p, uint32_t apdu_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    if ( apdu_len < APDU_HEADER_LEN )
    {
        return secfalse;
    }

    uint8_t header[APDU_HEADER_LEN];
    memcpy(header, apdu_p, APDU_HEADER_LEN);
    const uint8_t* data_p = (apdu_len > APDU_HEADER_LEN) ? apdu_p + APDU_HEADER_LEN : NULL;
    uint32_t data_len = (apdu_len > APDU_HEADER_LEN) ? apdu_len - APDU_HEADER_LEN : 0;

    uint8_t enc_data[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];
    uint32_t enc_len = 0;

    // GP SCP03 6.2.6: the encryption counter is incremented for every command
    // in the session, including commands with no data field; only a non-empty
    // data field is actually encrypted.
    if ( session_p->security_level & SECURE_CHANNEL_SCP03_SEC_CDEC )
    {
        sp2p_scp03_increment_counter(session_p->enc_counter);
        if ( data_len > 0 )
        {
            ExecuteCheck__E__sectrue__Rof__secfalse(
                secure_channel_scp03_encrypt(session_p, data_p, data_len, enc_data, &enc_len)
            );
            data_p = enc_data;
            data_len = enc_len;
        }
    }

    // NOTE: a short APDU carries at most 255 bytes; refuse
    // instead of truncating Lc (240 B plaintext pads to 256 + 8 MAC = 264).
    if ( data_len + SECURE_CHANNEL_SCP03_MAC_LEN > 0xFF )
    {
        crypto_utils_memzero_explicit(enc_data, sizeof(enc_data));
        return secfalse;
    }

    // modify CLA for secure messaging, update Lc to include MAC
    header[0] = scp03_apdu_secure_cla(header[0]);
    header[4] = (uint8_t)(data_len + SECURE_CHANNEL_SCP03_MAC_LEN);

    // build MAC input: header(5) + data
    uint8_t mac_buf[APDU_HEADER_LEN + SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];
    memcpy(mac_buf, header, APDU_HEADER_LEN);
    if ( data_len > 0 )
    {
        memcpy(mac_buf + APDU_HEADER_LEN, data_p, data_len);
    }

    uint8_t cmac[SECURE_CHANNEL_SCP03_MAC_LEN];
    ExecuteCheck__E__sectrue__Rof__secfalse(
        secure_channel_scp03_mac(session_p, mac_buf, APDU_HEADER_LEN + data_len, cmac)
    );

    // assemble output: header(5) + data + C-MAC(8)
    memcpy(out_p, header, APDU_HEADER_LEN);
    if ( data_len > 0 )
    {
        memcpy(out_p + APDU_HEADER_LEN, data_p, data_len);
    }
    memcpy(out_p + APDU_HEADER_LEN + data_len, cmac, SECURE_CHANNEL_SCP03_MAC_LEN);
    *out_len_p = APDU_HEADER_LEN + data_len + SECURE_CHANNEL_SCP03_MAC_LEN;

    crypto_utils_memzero_explicit(enc_data, sizeof(enc_data));
    return sectrue;
}

secbool_t secure_channel_scp03_apdu_unwrap_command(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* apdu_p, uint32_t apdu_len, uint8_t* out_p,
    uint32_t* out_len_p
)
{
    if ( apdu_len < APDU_HEADER_LEN + SECURE_CHANNEL_SCP03_MAC_LEN )
    {
        return secfalse;
    }

    uint32_t enc_data_len = apdu_len - APDU_HEADER_LEN - SECURE_CHANNEL_SCP03_MAC_LEN;

    const uint8_t* cmac_p = apdu_p + apdu_len - SECURE_CHANNEL_SCP03_MAC_LEN;

    // verify C-MAC over header(5) + encrypted_data
    ExecuteCheck__E__sectrue__Rof__secfalse(
        secure_channel_scp03_mac_verify(session_p, apdu_p, apdu_len - SECURE_CHANNEL_SCP03_MAC_LEN, cmac_p)
    );

    // decrypt if C-DECRYPTION enabled
    uint8_t header[APDU_HEADER_LEN];
    memcpy(header, apdu_p, APDU_HEADER_LEN);
    // restore original CLA
    header[0] = (header[0] & 0xE0) | (header[0] & 0x03);
    // GP SCP03 6.2.6: counter advances for every command, even empty ones.
    if ( session_p->security_level & SECURE_CHANNEL_SCP03_SEC_CDEC )
    {
        sp2p_scp03_increment_counter(session_p->enc_counter);
    }

    uint32_t plain_len = 0;
    if ( (session_p->security_level & SECURE_CHANNEL_SCP03_SEC_CDEC) && enc_data_len > 0 )
    {
        uint8_t plain[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];
        ExecuteCheck__E__sectrue__Rof__secfalse(
            secure_channel_scp03_decrypt(session_p, apdu_p + APDU_HEADER_LEN, enc_data_len, plain, &plain_len)
        );

        header[4] = (uint8_t)plain_len;
        memcpy(out_p, header, APDU_HEADER_LEN);
        memcpy(out_p + APDU_HEADER_LEN, plain, plain_len);
        crypto_utils_memzero_explicit(plain, sizeof(plain));
    }
    else
    {
        header[4] = (uint8_t)enc_data_len;
        memcpy(out_p, header, APDU_HEADER_LEN);
        if ( enc_data_len > 0 )
        {
            memcpy(out_p + APDU_HEADER_LEN, apdu_p + APDU_HEADER_LEN, enc_data_len);
        }
        plain_len = enc_data_len;
    }

    *out_len_p = APDU_HEADER_LEN + plain_len;
    return sectrue;
}

secbool_t secure_channel_scp03_apdu_wrap_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t sw1,
    uint8_t sw2, uint8_t* out_p, uint32_t* out_len_p
)
{
    const uint8_t* resp_data_p = data_p;
    uint32_t resp_data_len = data_len;
    uint8_t enc_data[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN];

    // encrypt if R-ENCRYPTION enabled
    if ( (session_p->security_level & SECURE_CHANNEL_SCP03_SEC_RENC) && data_len > 0 )
    {
        ExecuteCheck__E__sectrue__Rof__secfalse(
            secure_channel_scp03_encrypt_response(session_p, data_p, data_len, enc_data, &resp_data_len)
        );
        resp_data_p = enc_data;
    }

    uint32_t offset = 0;
    if ( resp_data_len > 0 )
    {
        memcpy(out_p, resp_data_p, resp_data_len);
        offset = resp_data_len;
    }

    // R-MAC if enabled
    if ( session_p->security_level & SECURE_CHANNEL_SCP03_SEC_RMAC )
    {
        // R-MAC input: response_data + SW1 + SW2
        uint8_t rmac_input[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN + 2];
        uint32_t rmac_input_len = 0;
        // NOTE: with R-ENC off the data is not length-checked
        // by encrypt_response, so bound it here before the memcpy.
        if ( resp_data_len > sizeof(rmac_input) - 2 )
        {
            crypto_utils_memzero_explicit(enc_data, sizeof(enc_data));
            return secfalse;
        }
        if ( resp_data_len > 0 )
        {
            memcpy(rmac_input, resp_data_p, resp_data_len);
            rmac_input_len = resp_data_len;
        }
        rmac_input[rmac_input_len++] = sw1;
        rmac_input[rmac_input_len++] = sw2;

        uint8_t rmac[SECURE_CHANNEL_SCP03_MAC_LEN];
        ExecuteCheck__E__sectrue__Rof__secfalse(
            secure_channel_scp03_rmac(session_p, rmac_input, rmac_input_len, rmac)
        );

        memcpy(out_p + offset, rmac, SECURE_CHANNEL_SCP03_MAC_LEN);
        offset += SECURE_CHANNEL_SCP03_MAC_LEN;
        crypto_utils_memzero_explicit(rmac_input, sizeof(rmac_input));
    }

    out_p[offset++] = sw1;
    out_p[offset++] = sw2;
    *out_len_p = offset;

    crypto_utils_memzero_explicit(enc_data, sizeof(enc_data));
    return sectrue;
}

secbool_t secure_channel_scp03_apdu_unwrap_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* response_p, uint32_t response_len,
    uint8_t* data_p, uint32_t* data_len_p, uint8_t* sw1_p, uint8_t* sw2_p
)
{
    if ( response_len < 2 )
    {
        return secfalse;
    }

    // SW is always last 2 bytes
    *sw1_p = response_p[response_len - 2];
    *sw2_p = response_p[response_len - 1];

    uint32_t payload_len = response_len - 2;

    // strip R-MAC if enabled
    if ( session_p->security_level & SECURE_CHANNEL_SCP03_SEC_RMAC )
    {
        if ( payload_len < SECURE_CHANNEL_SCP03_MAC_LEN )
        {
            return secfalse;
        }

        uint32_t enc_data_len = payload_len - SECURE_CHANNEL_SCP03_MAC_LEN;
        const uint8_t* rmac_p = response_p + enc_data_len;

        // verify R-MAC over response_data + SW1 + SW2
        uint8_t rmac_input[SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN + 2];
        uint32_t rmac_input_len = 0;
        // NOTE: the response comes from the peer and is not
        // yet authenticated - bound it before it touches the stack buffer.
        if ( enc_data_len > sizeof(rmac_input) - 2 )
        {
            return secfalse;
        }
        if ( enc_data_len > 0 )
        {
            memcpy(rmac_input, response_p, enc_data_len);
            rmac_input_len = enc_data_len;
        }
        rmac_input[rmac_input_len++] = *sw1_p;
        rmac_input[rmac_input_len++] = *sw2_p;

        ExecuteCheck__E__sectrue__Rof__secfalse(
            secure_channel_scp03_rmac_verify(session_p, rmac_input, rmac_input_len, rmac_p)
        );

        crypto_utils_memzero_explicit(rmac_input, sizeof(rmac_input));
        payload_len = enc_data_len;
    }

    // decrypt if R-ENCRYPTION enabled
    if ( (session_p->security_level & SECURE_CHANNEL_SCP03_SEC_RENC) && payload_len > 0 )
    {
        ExecuteCheck__E__sectrue__Rof__secfalse(
            secure_channel_scp03_decrypt_response(session_p, response_p, payload_len, data_p, data_len_p)
        );
    }
    else
    {
        // NOTE: keep the plaintext path under the same
        // ceiling the encrypted path enforces (decrypt rejects > MAX_DATA_LEN).
        if ( payload_len > SECURE_CHANNEL_SCP03_MAX_DATA_LEN + SECURE_CHANNEL_SCP03_BLOCK_LEN )
        {
            return secfalse;
        }
        if ( payload_len > 0 )
        {
            memcpy(data_p, response_p, payload_len);
        }
        *data_len_p = payload_len;
    }

    return sectrue;
}
