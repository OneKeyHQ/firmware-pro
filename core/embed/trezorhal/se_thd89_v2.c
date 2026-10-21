#include "se_thd89_v2.h"

#include <string.h>

#include "aes/aes.h"
#include "hmac.h"
#include "memzero.h"
#include "sha2.h"

static const uint8_t SESSION_ENC_LABEL[] = "THD89 SESSION ENC";
static const uint8_t SESSION_MAC_LABEL[] = "THD89 SESSION MAC";
static const uint8_t SESSION_CONFIRM_LABEL[] = "THD89 SESSION CONFIRM";
static const uint8_t SESSION_RESPONSE_LABEL[] = "THD89 SESSION RESPONSE V2";
static const uint8_t SESSION_APDU_HEADER[5] = {0x00, 0xfa, 0x01, 0x00, 0x60};
static const uint8_t RESPONSE_MAC_DOMAIN[16] = "THD89-RSP-MAC-V1";

static void derive_session_digest(const uint8_t shared_point[64],
                                  const uint8_t se_random[16],
                                  const uint8_t mcu_random[16],
                                  const uint8_t *label, size_t label_len,
                                  uint8_t digest[32]) {
  SHA256_CTX ctx = {0};

  sha256_Init(&ctx);
  sha256_Update(&ctx, shared_point, 64);
  sha256_Update(&ctx, se_random, 16);
  sha256_Update(&ctx, mcu_random, 16);
  sha256_Update(&ctx, label, label_len);
  sha256_Final(&ctx, digest);
  memzero(&ctx, sizeof(ctx));
}

void thd89_v2_derive_session_keys(const uint8_t shared_point[64],
                                  const uint8_t se_random[16],
                                  const uint8_t mcu_random[16],
                                  uint8_t enc_key[16], uint8_t mac_key[16],
                                  uint8_t confirm_key[32]) {
  uint8_t digest[32] = {0};

  derive_session_digest(shared_point, se_random, mcu_random, SESSION_ENC_LABEL,
                        sizeof(SESSION_ENC_LABEL) - 1, digest);
  memcpy(enc_key, digest, 16);

  derive_session_digest(shared_point, se_random, mcu_random, SESSION_MAC_LABEL,
                        sizeof(SESSION_MAC_LABEL) - 1, digest);
  memcpy(mac_key, digest, 16);

  derive_session_digest(shared_point, se_random, mcu_random,
                        SESSION_CONFIRM_LABEL,
                        sizeof(SESSION_CONFIRM_LABEL) - 1, confirm_key);
  memzero(digest, sizeof(digest));
}

void thd89_v2_calculate_confirmation(const uint8_t confirm_key[32],
                                     const uint8_t se_random[16],
                                     const uint8_t request_data[96],
                                     uint8_t confirmation[32]) {
  HMAC_SHA256_CTX ctx = {0};

  hmac_sha256_Init(&ctx, confirm_key, 32);
  hmac_sha256_Update(&ctx, SESSION_RESPONSE_LABEL,
                     sizeof(SESSION_RESPONSE_LABEL) - 1);
  hmac_sha256_Update(&ctx, SESSION_APDU_HEADER, sizeof(SESSION_APDU_HEADER));
  hmac_sha256_Update(&ctx, se_random, 16);
  hmac_sha256_Update(&ctx, request_data, 96);
  hmac_sha256_Final(&ctx, confirmation);
  memzero(&ctx, sizeof(ctx));
}

static void response_mac_block(const aes_encrypt_ctx *ctx, uint8_t state[16],
                               const uint8_t block[16]) {
  uint8_t input[16] = {0};
  uint8_t output[16] = {0};

  for (size_t i = 0; i < sizeof(input); i++) {
    input[i] = state[i] ^ block[i];
  }
  if (aes_ecb_encrypt(input, output, sizeof(output), ctx) == EXIT_SUCCESS) {
    memcpy(state, output, sizeof(output));
  } else {
    memzero(state, 16);
  }
  memzero(input, sizeof(input));
  memzero(output, sizeof(output));
}

void thd89_v2_calculate_response_mac(const uint8_t mac_key[16],
                                     const uint8_t header[4],
                                     const uint8_t transaction[16],
                                     const uint8_t *ciphertext,
                                     uint16_t ciphertext_len, uint16_t sw1sw2,
                                     uint8_t response_mac[4]) {
  aes_encrypt_ctx ctx = {0};
  uint8_t state[16] = {0};
  uint8_t context[16] = {0};
  uint8_t status_block[16] = {0};

  context[0] = header[0];
  context[1] = header[1];
  context[2] = header[2];
  context[3] = header[3];
  context[4] = (uint8_t)(ciphertext_len >> 8);
  context[5] = (uint8_t)ciphertext_len;
  context[6] = 0x80;

  status_block[0] = (uint8_t)(sw1sw2 >> 8);
  status_block[1] = (uint8_t)sw1sw2;
  status_block[2] = 0x80;

  if (aes_encrypt_key128(mac_key, &ctx) != EXIT_SUCCESS) {
    memzero(response_mac, 4);
    goto cleanup;
  }

  response_mac_block(&ctx, state, RESPONSE_MAC_DOMAIN);
  response_mac_block(&ctx, state, transaction);
  response_mac_block(&ctx, state, context);
  for (uint16_t offset = 0; offset < ciphertext_len; offset += 16) {
    response_mac_block(&ctx, state, ciphertext + offset);
  }
  response_mac_block(&ctx, state, status_block);
  memcpy(response_mac, state, 4);

cleanup:
  memzero(&ctx, sizeof(ctx));
  memzero(state, sizeof(state));
  memzero(context, sizeof(context));
  memzero(status_block, sizeof(status_block));
}

thd89_v2_response_shape_t thd89_v2_classify_response(uint16_t response_data_len,
                                                     uint16_t sw1sw2) {
  if ((sw1sw2 >> 8) == 0x6c) {
    return response_data_len == 0 ? THD89_V2_RESPONSE_NO_MAC_6C
                                  : THD89_V2_RESPONSE_INVALID;
  }
  if (response_data_len < 4 || ((response_data_len - 4) % 16) != 0) {
    return THD89_V2_RESPONSE_INVALID;
  }
  return THD89_V2_RESPONSE_MAC_REQUIRED;
}

bool thd89_v2_constant_time_equal(const uint8_t *left, const uint8_t *right,
                                  size_t len) {
  uint8_t diff = 0;

  if ((left == NULL || right == NULL) && len != 0) {
    return false;
  }
  for (size_t i = 0; i < len; i++) {
    diff |= left[i] ^ right[i];
  }
  return diff == 0;
}

bool thd89_v2_unpad_iso7816_4(const uint8_t *padded, uint16_t padded_len,
                              uint16_t *plaintext_len) {
  if (padded == NULL || plaintext_len == NULL || padded_len < 16 ||
      (padded_len % 16) != 0) {
    return false;
  }

  for (uint16_t offset = 0; offset < 16; offset++) {
    uint8_t value = padded[padded_len - 1 - offset];
    if (value == 0x80) {
      *plaintext_len = padded_len - 1 - offset;
      return true;
    }
    if (value != 0x00) {
      return false;
    }
  }
  return false;
}
