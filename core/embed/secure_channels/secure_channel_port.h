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
secbool_t crypto_utils_memzero_explicit(void* buf_p, size_t len);
