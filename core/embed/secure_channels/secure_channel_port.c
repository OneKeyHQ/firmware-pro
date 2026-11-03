#include "secure_channel_port.h"

#include <string.h>

#include "aes/aes.h"
#include "memzero.h"
#include "rand.h"

#include "../manage_card/cmac.h"

// ########################################################################
// hal/crypto

static bool aes_set_encrypt_key(const uint8_t* key_p, size_t key_len, aes_encrypt_ctx* ctx)
{
    switch ( key_len )
    {
    case 16:
        return aes_encrypt_key128(key_p, ctx) == EXIT_SUCCESS;
    case 24:
        return aes_encrypt_key192(key_p, ctx) == EXIT_SUCCESS;
    case 32:
        return aes_encrypt_key256(key_p, ctx) == EXIT_SUCCESS;
    default:
        return false;
    }
}

static bool aes_set_decrypt_key(const uint8_t* key_p, size_t key_len, aes_decrypt_ctx* ctx)
{
    switch ( key_len )
    {
    case 16:
        return aes_decrypt_key128(key_p, ctx) == EXIT_SUCCESS;
    case 24:
        return aes_decrypt_key192(key_p, ctx) == EXIT_SUCCESS;
    case 32:
        return aes_decrypt_key256(key_p, ctx) == EXIT_SUCCESS;
    default:
        return false;
    }
}

secbool_t crypto_cipher_aes_ecb_encrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t* in_p, size_t len, uint8_t* out_p, size_t* out_len_p
)
{
    if ( key_p == NULL || in_p == NULL || out_p == NULL || out_len_p == NULL || (len % 16) != 0 )
    {
        return secfalse;
    }
    aes_encrypt_ctx ctx;
    secbool_t ret = secfalse;
    if ( aes_set_encrypt_key(key_p, key_len, &ctx) &&
         aes_ecb_encrypt(in_p, out_p, (int)len, &ctx) == EXIT_SUCCESS )
    {
        *out_len_p = len;
        ret = sectrue;
    }
    memzero(&ctx, sizeof(ctx));
    return ret;
}

secbool_t crypto_cipher_aes_cbc_encrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t iv[16], const uint8_t* in_p, size_t len,
    uint8_t* out_p, size_t* out_len_p
)
{
    if ( key_p == NULL || iv == NULL || in_p == NULL || out_p == NULL || out_len_p == NULL ||
         (len % 16) != 0 )
    {
        return secfalse;
    }
    aes_encrypt_ctx ctx;
    uint8_t iv_work[16];
    memcpy(iv_work, iv, sizeof(iv_work)); // the AES lib advances the IV
    secbool_t ret = secfalse;
    if ( aes_set_encrypt_key(key_p, key_len, &ctx) &&
         aes_cbc_encrypt(in_p, out_p, (int)len, iv_work, &ctx) == EXIT_SUCCESS )
    {
        *out_len_p = len;
        ret = sectrue;
    }
    memzero(iv_work, sizeof(iv_work));
    memzero(&ctx, sizeof(ctx));
    return ret;
}

secbool_t crypto_cipher_aes_cbc_decrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t iv[16], const uint8_t* in_p, size_t len,
    uint8_t* out_p, size_t* out_len_p
)
{
    if ( key_p == NULL || iv == NULL || in_p == NULL || out_p == NULL || out_len_p == NULL ||
         (len % 16) != 0 )
    {
        return secfalse;
    }
    aes_decrypt_ctx ctx;
    uint8_t iv_work[16];
    memcpy(iv_work, iv, sizeof(iv_work));
    secbool_t ret = secfalse;
    if ( aes_set_decrypt_key(key_p, key_len, &ctx) &&
         aes_cbc_decrypt(in_p, out_p, (int)len, iv_work, &ctx) == EXIT_SUCCESS )
    {
        *out_len_p = len;
        ret = sectrue;
    }
    memzero(iv_work, sizeof(iv_work));
    memzero(&ctx, sizeof(ctx));
    return ret;
}

secbool_t crypto_mac_cmac_aes_compute(
    const uint8_t* key_p, size_t key_len, const uint8_t* data_p, size_t data_len, uint8_t out[16]
)
{
    if ( key_p == NULL || out == NULL || key_len != 16 || data_len > 0x7FFF ||
         (data_len > 0 && data_p == NULL) )
    {
        return secfalse;
    }
    // manage_card/cmac.c takes non-const pointers but does not write through them.
    AES128_CMAC((unsigned char*)key_p, (unsigned char*)data_p, (int)data_len, out);
    return sectrue;
}

secbool_t crypto_utils_random_generate(uint8_t* buf_p, size_t len)
{
    if ( buf_p == NULL )
    {
        return secfalse;
    }
    random_buffer(buf_p, len);
    return sectrue;
}

secbool_t crypto_utils_memzero_explicit(void* buf_p, size_t len)
{
    if ( buf_p == NULL )
    {
        return secfalse;
    }
    memzero(buf_p, len);
    return sectrue;
}
