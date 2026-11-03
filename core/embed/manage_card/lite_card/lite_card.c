#include <string.h>

#include "aes/aes.h"
#include "cmac.h"
#include "lite_card.h"
#include "memzero.h"
#include "nfc.h"
#include "scp03.h"

static scp03_context scp03_ctx = {0};
static scp11_session_key session_key = {0};

// ########################################################################
// secure messaging

void lite_card_scp03_open(const scp11_session_key *keys,
                          const uint8_t *mac_chaining) {
  memcpy(&session_key, keys, sizeof(session_key));
  scp03_init(&scp03_ctx, (uint8_t *)mac_chaining);
}

void lite_card_scp03_close(void) {
  memzero(&scp03_ctx, sizeof(scp03_ctx));
  memzero(&session_key, sizeof(session_key));
}

bool lite_card_send_safeapdu(uint8_t *apdu, uint16_t apdu_len,
                             uint8_t *response, uint16_t *response_len,
                             uint8_t *sw1sw2) {
  uint8_t mac_data[256 + 16], data_buffer[256], enc_data[256];
  uint16_t enc_data_len = 0, buffer_len = sizeof(data_buffer);
  uint8_t mac[16];
  bool ret = false;

  // Largest APDU whose padded data, header and MAC still fit data_buffer.
  if (apdu_len > 244) {
    return false;
  }
  memcpy(data_buffer, apdu, apdu_len);

  if (apdu_len == 4) {
    data_buffer[4] = 0;
    apdu_len = 5;
  }

  scp03_generate_icv(session_key.s_enc, &scp03_ctx, true);
  scp03_encrypt(session_key.s_enc, scp03_ctx.icv, data_buffer + 5, apdu_len - 5,
                enc_data, &enc_data_len);

  data_buffer[0] |= 4;
  data_buffer[4] = enc_data_len + 8;

  memcpy(mac_data, scp03_ctx.mac_chain_value,
         sizeof(scp03_ctx.mac_chain_value));
  memcpy(mac_data + 16, data_buffer, 5);
  memcpy(mac_data + 21, enc_data, enc_data_len);

  AES128_CMAC(session_key.s_mac, mac_data, enc_data_len + 5 + 16, mac);

  memcpy(data_buffer + 5, enc_data, enc_data_len);
  memcpy(data_buffer + enc_data_len + 5, mac, 8);

  memcpy(scp03_ctx.mac_chain_value, mac, sizeof(scp03_ctx.mac_chain_value));

  if (!nfc_send_recv(data_buffer, enc_data_len + 5 + 8, data_buffer,
                     &buffer_len, sw1sw2)) {
    goto cleanup;
  }

  if (buffer_len == 0) {
    ret = !(sw1sw2[0] == 0x90 && sw1sw2[1] == 0x00) && sw1sw2[0] != 0x63;
    if (ret) {
      *response_len = 0;
    }
    goto cleanup;
  }

  if (buffer_len < AES_BLOCK_SIZE + SCP03_MAC_SIZE) {
    goto cleanup;
  }

  if ((buffer_len - SCP03_MAC_SIZE) % AES_BLOCK_SIZE != 0) {
    goto cleanup;
  }

  memcpy(mac_data, scp03_ctx.mac_chain_value,
         sizeof(scp03_ctx.mac_chain_value));
  memcpy(mac_data + 16, data_buffer, buffer_len - 8);
  memcpy(mac_data + 16 + buffer_len - 8, sw1sw2, 2);
  AES128_CMAC(session_key.s_rmac, mac_data, buffer_len + 8 + 2, mac);

  if (memcmp(mac, data_buffer + buffer_len - 8, 8) != 0) {
    goto cleanup;
  }

  buffer_len -= 8;

  scp03_generate_icv(session_key.s_enc, &scp03_ctx, false);
  if (!scp03_decrypt(session_key.s_enc, scp03_ctx.icv, data_buffer, buffer_len,
                     enc_data, &buffer_len)) {
    goto cleanup;
  }
  memcpy(response, enc_data, buffer_len);
  *response_len = buffer_len;
  ret = true;

cleanup:
  memzero(mac_data, sizeof(mac_data));
  memzero(data_buffer, sizeof(data_buffer));
  memzero(enc_data, sizeof(enc_data));
  memzero(mac, sizeof(mac));
  return ret;
}
