#ifndef _TREZORHAL_SE_THD89_V2_H_
#define _TREZORHAL_SE_THD89_V2_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  THD89_V2_RESPONSE_MAC_REQUIRED = 0,
  THD89_V2_RESPONSE_NO_MAC_6C,
  THD89_V2_RESPONSE_INVALID,
} thd89_v2_response_shape_t;

void thd89_v2_derive_session_keys(const uint8_t shared_point[64],
                                  const uint8_t se_random[16],
                                  const uint8_t mcu_random[16],
                                  uint8_t enc_key[16], uint8_t mac_key[16],
                                  uint8_t confirm_key[32]);

void thd89_v2_calculate_confirmation(const uint8_t confirm_key[32],
                                     const uint8_t se_random[16],
                                     const uint8_t request_data[96],
                                     uint8_t confirmation[32]);

void thd89_v2_calculate_response_mac(const uint8_t mac_key[16],
                                     const uint8_t header[4],
                                     const uint8_t transaction[16],
                                     const uint8_t *ciphertext,
                                     uint16_t ciphertext_len, uint16_t sw1sw2,
                                     uint8_t response_mac[4]);

thd89_v2_response_shape_t thd89_v2_classify_response(uint16_t response_data_len,
                                                     uint16_t sw1sw2);

bool thd89_v2_constant_time_equal(const uint8_t *left, const uint8_t *right,
                                  size_t len);

bool thd89_v2_unpad_iso7816_4(const uint8_t *padded, uint16_t padded_len,
                              uint16_t *plaintext_len);

#endif
