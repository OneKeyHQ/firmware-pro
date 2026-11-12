#pragma once

// secbool_t, __wur, ExecuteCheck_ADV and the AES / CMAC / random primitives
// the secure_channels sources use, on top of trezor-crypto.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "secbool.h"

// ########################################################################
// secbool_t / util_macros

typedef secbool secbool_t;

#ifndef __wur
  #define __wur __attribute__((warn_unused_result))
#endif

// NOTE: must stay token- and whitespace-identical to trezorhal/util_macros.h,
// which redefines it unguarded when included after this header
// clang-format off
#ifndef ExecuteCheck_ADV
#define ExecuteCheck_ADV(expr, expected_result, on_false) \
  {                                                       \
    typeof(expected_result) ret = (expr);                 \
    if (ret != (expected_result)) {                       \
      on_false                                            \
    }                                                     \
  }
#endif
// clang-format on

// ########################################################################
// crypto primitives

secbool_t __wur crypto_cipher_aes_ecb_encrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t* in_p, size_t len, uint8_t* out_p, size_t* out_len_p
);
secbool_t __wur crypto_cipher_aes_cbc_encrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t iv[16], const uint8_t* in_p, size_t len,
    uint8_t* out_p, size_t* out_len_p
);
secbool_t __wur crypto_cipher_aes_cbc_decrypt(
    const uint8_t* key_p, size_t key_len, const uint8_t iv[16], const uint8_t* in_p, size_t len,
    uint8_t* out_p, size_t* out_len_p
);
// AES-CMAC one-shot. This firmware only carries an AES-128 CMAC, so
// key_len != 16 returns secfalse (the P2P channel uses 16-byte keys).
secbool_t __wur crypto_mac_cmac_aes_compute(
    const uint8_t* key_p, size_t key_len, const uint8_t* data_p, size_t data_len, uint8_t out[16]
);
secbool_t __wur crypto_utils_random_generate(uint8_t* buf_p, size_t len);
// SHA-256 / HMAC-SHA256 one-shot (secure_p2p/handshake).
secbool_t __wur crypto_hash_sha256_compute(const uint8_t* data_p, size_t len, uint8_t out[32]);
secbool_t __wur crypto_mac_hmac_sha256_compute(
    const uint8_t* key_p, size_t key_len, const uint8_t* data_p, size_t data_len, uint8_t out[32]
);
// P-256 scalar multiplication (secure_p2p/handshake): priv 32 bytes; pub and
// out are raw X||Y (64 bytes, no 0x04 prefix); *out_len_p is the capacity in
// and 64 out. Off-curve pub returns secfalse (trezor-crypto validates the
// point); the base point G is accepted as pub so callers can derive a public
// key from a private one. Scalar range (1 <= e < n) is the caller's job.
secbool_t __wur crypto_keyexchange_ecdh_secp256r1Superfast_compute(
    const uint8_t* priv_p, size_t priv_len, const uint8_t* pub_p, size_t pub_len, uint8_t* out_p,
    size_t* out_len_p
);
// sectrue only for a point that lies on P-256 and is not the point at
// infinity (secure_p2p/identity; used on the peer's ephemeral key E and the
// envelope public key R).
secbool_t __wur crypto_sign_ecdsa_secp256r1Small_validate_pubkey(const uint8_t* pubkey_p, size_t pubkey_len);
secbool_t crypto_utils_memzero_explicit(void* buf_p, size_t len);

// ########################################################################
// crc

// CRC-32/ISO-HDLC: poly 0x04C11DB7, init 0xFFFFFFFF, reflected in/out,
// final xor 0xFFFFFFFF - byte-identical to Python zlib.crc32.
uint32_t secure_channel_port_crc32(const uint8_t* data_p, size_t len);
