#include <string.h>

#include "aes/aes.h"
#include "cmac.h"
#include "lite_card.h"
#include "nfc.h"
#include "scp03.h"
#include "scp11.h"

static scp11_context scp11_ctx = {0};
static scp03_context scp03_ctx = {0};

static bool lite_card_get_sd_certificate(uint8_t* cert, uint16_t* cert_len) {
  uint8_t apdu_get_sd_certificates[] = {0x80, 0xca, 0xbf, 0x21, 0x06, 0xa6,
                                        0x04, 0x83, 0x02, 0x15, 0x18};
  uint8_t sw1sw2[2] = {0};
  if (!nfc_send_recv(apdu_get_sd_certificates, sizeof(apdu_get_sd_certificates),
                     cert, cert_len, sw1sw2)) {
    return false;
  }
  if (sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00) {
    return false;
  }

  return true;
}

static bool lite_card_send_device_certificate(uint8_t* cert, uint8_t cert_len) {
  uint8_t apdu_send_device_cert[256] = {0x80, 0x2a, 0x18, 0x10, 0x00};
  uint8_t resp[2] = {0};
  uint16_t resp_len = sizeof(resp);
  uint8_t sw1sw2[2] = {0};
  apdu_send_device_cert[4] = cert_len;
  memcpy(apdu_send_device_cert + 5, cert, cert_len);
  if (!nfc_send_recv(apdu_send_device_cert, cert_len + 5, resp, &resp_len,
                     sw1sw2)) {
    return false;
  }
  if (sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00) {
    return false;
  }
  return true;
}

static bool lite_card_mutual_authentication(scp11_context* scp11_ctx) {
  uint8_t apdu_mutual_auth[256] = {0x80, 0x82, 0x18, 0x15, 0x00};
  uint8_t sw1sw2[2] = {0};
  apdu_mutual_auth[4] = 251;
  scp11_get_mutual_auth_data(apdu_mutual_auth + 5, &apdu_mutual_auth[4],
                             *scp11_ctx);
  if (!nfc_send_recv(apdu_mutual_auth, apdu_mutual_auth[4] + 5,
                     scp11_ctx->response_msg.data,
                     &scp11_ctx->response_msg.data_len, sw1sw2)) {
    return false;
  }
  if (sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00) {
    return false;
  }
  return true;
}

bool lite_card_send_safeapdu(uint8_t* apdu, uint16_t apdu_len,
                             uint8_t* response, uint16_t* response_len,
                             uint8_t* sw1sw2) {
  uint8_t mac_data[256 + 16], data_buffer[256], enc_data[256];
  uint16_t enc_data_len = 0, buffer_len = sizeof(data_buffer);
  uint8_t mac[16];
  memcpy(data_buffer, apdu, apdu_len);

  if (apdu_len == 4) {
    data_buffer[4] = 0;
    apdu_len = 5;
  }

  // encrypt data
  scp03_generate_icv(scp11_ctx.session_key.s_enc, &scp03_ctx, true);
  scp03_encrypt(scp11_ctx.session_key.s_enc, scp03_ctx.icv, data_buffer + 5,
                apdu_len - 5, enc_data, &enc_data_len);

  // gen cmac
  //  cla
  data_buffer[0] |= 4;
  // enc_data_len + mac_len
  data_buffer[4] = enc_data_len + 8;

  memcpy(mac_data, scp03_ctx.mac_chain_value,
         sizeof(scp03_ctx.mac_chain_value));
  memcpy(mac_data + 16, data_buffer, 5);
  memcpy(mac_data + 21, enc_data, enc_data_len);

  AES128_CMAC(scp11_ctx.session_key.s_mac, mac_data, enc_data_len + 5 + 16,
              mac);

  memcpy(data_buffer + 5, enc_data, enc_data_len);
  memcpy(data_buffer + enc_data_len + 5, mac, 8);

  memcpy(scp03_ctx.mac_chain_value, mac, sizeof(scp03_ctx.mac_chain_value));

  if (!nfc_send_recv(data_buffer, enc_data_len + 5 + 8, data_buffer,
                     &buffer_len, sw1sw2)) {
    return false;
  }

  if (buffer_len == 0) {
    if (sw1sw2[0] == 0x90 && sw1sw2[1] == 0x00) {
      return false;
    }
    if (sw1sw2[0] == 0x63) {
      return false;
    }
    return true;
  }

  // enc data len + mac len
  if (buffer_len < AES_BLOCK_SIZE + SCP03_MAC_SIZE) {
    return false;
  }

  if ((buffer_len - SCP03_MAC_SIZE) % AES_BLOCK_SIZE != 0) {
    return false;
  }

  // verify mac
  memcpy(mac_data, scp03_ctx.mac_chain_value,
         sizeof(scp03_ctx.mac_chain_value));
  memcpy(mac_data + 16, data_buffer, buffer_len - 8);
  memcpy(mac_data + 16 + buffer_len - 8, sw1sw2, 2);
  AES128_CMAC(scp11_ctx.session_key.s_rmac, mac_data, buffer_len + 8 + 2, mac);

  if (memcmp(mac, data_buffer + buffer_len - 8, 8) != 0) {
    return false;
  }

  buffer_len -= 8;

  // decrypt data
  scp03_generate_icv(scp11_ctx.session_key.s_enc, &scp03_ctx, false);
  if (!scp03_decrypt(scp11_ctx.session_key.s_enc, scp03_ctx.icv, data_buffer,
                     buffer_len, enc_data, &buffer_len)) {
    return false;
  }
  memcpy(response, enc_data, buffer_len);
  *response_len = buffer_len;
  return true;
}

bool lite_card_open_secure_channel(void) {
  if (scp11_get_secure_channel_status(&scp11_ctx)) {
    return true;
  }
  scp11_init(&scp11_ctx);

  scp11_ctx.sd_cert.raw_len = sizeof(scp11_ctx.sd_cert.raw);
  if (!lite_card_get_sd_certificate(scp11_ctx.sd_cert.raw,
                                    &scp11_ctx.sd_cert.raw_len)) {
    return false;
  }
  if (!scp11_certificate_parse_and_verify(scp11_ctx.sd_cert.raw,
                                          scp11_ctx.sd_cert.raw_len,
                                          &scp11_ctx.sd_cert)) {
    return false;
  }
  if (!lite_card_send_device_certificate(scp11_ctx.oce_cert.raw,
                                         scp11_ctx.oce_cert.raw_len)) {
    return false;
  }
  if (!lite_card_mutual_authentication(&scp11_ctx)) {
    return false;
  }

  if (!scp11_open_secure_channel(&scp11_ctx)) {
    return false;
  }

  scp03_init(&scp03_ctx, scp11_ctx.response_msg.lv_GPC_TLV_MA_RECEIPT.value);

  return true;
}

bool lite_card_apdu(uint8_t* apdu, uint16_t apdu_len, uint8_t* response,
                    uint16_t* response_len, uint8_t* sw1sw2, bool safe) {
  if (memcmp(apdu, "\x00\xa4\x04\x00", 4) == 0) {
    scp11_close_secure_channel(&scp11_ctx);
  }

  if (safe) {
    if (!lite_card_open_secure_channel()) {
      return false;
    }
    return lite_card_send_safeapdu(apdu, apdu_len, response, response_len,
                                   sw1sw2);
  }
  return nfc_send_recv(apdu, apdu_len, response, response_len, sw1sw2);
}

bool lite_card_safe_apdu_test(void) {
  uint8_t select_mf[] = {0x00, 0xa4, 0x04, 0x00};
  uint8_t select_old_aid[] = {0x00, 0xa4, 0x04, 0x00, 0x08, 0xD1, 0x56,
                              0x00, 0x01, 0x32, 0x83, 0x40, 0x01};
  uint8_t reset_card[] = {0x80, 0xcb, 0x80, 0x00, 0x05,
                          0xdf, 0xfe, 0x02, 0x82, 0x05};
  uint8_t check_pin[] = {0x80, 0xcb, 0x80, 0x00, 0x05,
                         0xdf, 0xff, 0x02, 0x81, 0x05};
  uint8_t set_pin[] = {0x80, 0xcb, 0x80, 0x00, 0x0e, 0xdf, 0xfe,
                       0x0b, 0x82, 0x04, 0x08, 0x00, 0x06, 0x31,
                       0x32, 0x33, 0x34, 0x35, 0x36};
  uint8_t verify_pin[] = {0x80, 0x20, 0x00, 0x00, 0x07, 0x06,
                          0x31, 0x32, 0x33, 0x34, 0x35, 0x36};
  uint8_t resp[256];
  uint16_t resp_len = sizeof(resp);
  uint8_t sw1sw2[2] = {0};

  lite_card_apdu(select_mf, 4, resp, &resp_len, sw1sw2, false);

  resp_len = sizeof(resp);
  if (!lite_card_apdu(reset_card, sizeof(reset_card), resp, &resp_len, sw1sw2,
                      true)) {
    return false;
  }
  resp_len = sizeof(resp);
  if (!lite_card_apdu(check_pin, sizeof(check_pin), resp, &resp_len, sw1sw2,
                      true)) {
    return false;
  }

  resp_len = sizeof(resp);
  if (!lite_card_apdu(set_pin, sizeof(set_pin), resp, &resp_len, sw1sw2,
                      true)) {
    return false;
  }

  resp_len = sizeof(resp);
  if (!lite_card_apdu(check_pin, sizeof(check_pin), resp, &resp_len, sw1sw2,
                      true)) {
    return false;
  }
  if (resp[0] != 0) {
    return false;
  }

  lite_card_apdu(select_old_aid, sizeof(select_old_aid), resp, &resp_len,
                 sw1sw2, false);

  resp_len = sizeof(resp);
  if (!lite_card_apdu(verify_pin, sizeof(verify_pin), resp, &resp_len, sw1sw2,
                      true)) {
    return false;
  }
  return true;
}
