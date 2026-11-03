#include "secure_channel_port.h"

#include <string.h>

#include "aes/aes.h"
#include "memzero.h"
#include "rand.h"
#include "ecdsa.h"
#include "hmac.h"
#include "nist256p1.h"
#include "sha2.h"

#include "../extmod/modtrezorcrypto/crc.h"
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

secbool_t crypto_hash_sha256_compute(const uint8_t* data_p, size_t len, uint8_t out[32])
{
    if ( out == NULL || (len > 0 && data_p == NULL) )
    {
        return secfalse;
    }
    sha256_Raw(data_p, len, out);
    return sectrue;
}

secbool_t crypto_mac_hmac_sha256_compute(
    const uint8_t* key_p, size_t key_len, const uint8_t* data_p, size_t data_len, uint8_t out[32]
)
{
    if ( key_p == NULL || out == NULL || (data_len > 0 && data_p == NULL) || key_len > UINT32_MAX ||
         data_len > UINT32_MAX )
    {
        return secfalse;
    }
    hmac_sha256(key_p, (uint32_t)key_len, data_p, (uint32_t)data_len, out);
    return sectrue;
}

secbool_t crypto_keyexchange_ecdh_secp256r1Superfast_compute(
    const uint8_t* priv_p, size_t priv_len, const uint8_t* pub_p, size_t pub_len, uint8_t* out_p,
    size_t* out_len_p
)
{
    uint8_t pub65[65];
    uint8_t out65[65];
    secbool_t ret = secfalse;

    if ( priv_p == NULL || pub_p == NULL || out_p == NULL || out_len_p == NULL || priv_len != 32 ||
         pub_len != 64 || *out_len_p < 64 )
    {
        return secfalse;
    }
    pub65[0] = 0x04;
    memcpy(pub65 + 1, pub_p, 64);
    // ecdh_multiply -> ecdsa_read_pubkey -> ecdsa_validate_pubkey rejects an
    // off-curve point; non-zero return is failure.
    if ( ecdh_multiply(&nist256p1, priv_p, pub65, out65) == 0 )
    {
        memcpy(out_p, out65 + 1, 64);
        *out_len_p = 64;
        ret = sectrue;
    }
    memzero(pub65, sizeof(pub65));
    memzero(out65, sizeof(out65));
    return ret;
}

secbool_t crypto_sign_ecdsa_secp256r1Small_validate_pubkey(const uint8_t* pubkey_p, size_t pubkey_len)
{
    uint8_t pub65[65];
    curve_point point = {0};

    if ( pubkey_p == NULL || pubkey_len != 64 )
    {
        return secfalse;
    }
    pub65[0] = 0x04;
    memcpy(pub65 + 1, pubkey_p, 64);
    // ecdsa_read_pubkey -> ecdsa_validate_pubkey: on the curve and not the
    // point at infinity.
    return sectrue * (ecdsa_read_pubkey(&nist256p1, pub65, &point) == 1);
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

// ########################################################################
// crc

uint32_t secure_channel_port_crc32(const uint8_t* data_p, size_t len)
{
    // checksum_crc32() is the reflected-table core without the final xor;
    // init 0xFFFFFFFF + final xor gives the zlib.crc32 value.
    return checksum_crc32(data_p, (uint32_t)len, 0xFFFFFFFFu) ^ 0xFFFFFFFFu;
}
