#include <stdio.h>
#include <string.h>

#include "manage_card.h"
#include "memzero.h"
#include "nfc.h"
#include "scp11.h"

#include "lite_card/lite_card.h"
#include "seed_card/seed_card.h"

// GET DATA / serial number. Watching the answer to this one is how the card
// generation is detected: nothing else on the wire identifies the card.
static const uint8_t apdu_get_serial[] = {0x80, 0xCB, 0x80, 0x00, 0x05,
                                          0xDF, 0xFF, 0x02, 0x81, 0x01};
static const uint8_t apdu_select_prefix[] = {0x00, 0xA4, 0x04, 0x00};

static scp11_context scp11_ctx = {0};
static uint8_t card_version = MANAGE_CARD_VERSION_UNKNOWN;

// ########################################################################
// private functions

static bool is_seed_card(void) {
  return card_version == MANAGE_CARD_VERSION_SEED;
}

static bool manage_card_get_sd_certificate(uint8_t *cert, uint16_t *cert_len) {
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

static bool manage_card_send_device_certificate(uint8_t *cert,
                                                uint8_t cert_len) {
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

static bool manage_card_mutual_authentication(scp11_context *ctx) {
  uint8_t apdu_mutual_auth[256] = {0x80, 0x82, 0x18, 0x15, 0x00};
  uint8_t sw1sw2[2] = {0};
  apdu_mutual_auth[4] = 251;
  if (!scp11_get_mutual_auth_data(apdu_mutual_auth + 5, &apdu_mutual_auth[4],
                                  ctx)) {
    return false;
  }
  if (!nfc_send_recv(apdu_mutual_auth, apdu_mutual_auth[4] + 5,
                     ctx->response_msg.data, &ctx->response_msg.data_len,
                     sw1sw2)) {
    return false;
  }
  if (sw1sw2[0] != 0x90 || sw1sw2[1] != 0x00) {
    return false;
  }
  return true;
}

// SCP11 handshake, identical for both card generations except for the
// credentials scp11_init() picks and the key derivation inside
// scp11_open_secure_channel(). Stops before the SCP03 session is installed so
// the factory test can reuse it.
static bool manage_card_run_scp11_handshake(void) {
  if (!scp11_init(&scp11_ctx, card_version)) {
    printf("scp11_init failed (OCE cert did not parse/verify)\n");
    return false;
  }

  scp11_ctx.sd_cert.raw_len = sizeof(scp11_ctx.sd_cert.raw);
  if (!manage_card_get_sd_certificate(scp11_ctx.sd_cert.raw,
                                      &scp11_ctx.sd_cert.raw_len)) {
    printf("GET SD CERT failed\n");
    return false;
  }
  if (!scp11_certificate_parse_and_verify(
          scp11_ctx.sd_cert.raw, scp11_ctx.sd_cert.raw_len,
          scp11_root_pubkey(card_version), &scp11_ctx.sd_cert)) {
    printf("SD cert verify failed (wrong root key for this card?)\n");
    return false;
  }
  if (!manage_card_send_device_certificate(scp11_ctx.oce_cert.raw,
                                           scp11_ctx.oce_cert.raw_len)) {
    printf("PUT OCE CERT failed (card rejected our certificate)\n");
    return false;
  }
  if (!manage_card_mutual_authentication(&scp11_ctx)) {
    printf("MUTUAL AUTH apdu failed\n");
    return false;
  }
  return true;
}

// ########################################################################
// public functions

void manage_card_set_version(uint8_t version) {
  if (version == card_version) {
    return;
  }
  // The generations do not share session state, so anything still open belongs
  // to the previous card.
  manage_card_close_secure_channel();
  card_version = version;
}

bool manage_card_version_from_serial(const uint8_t *serial, uint16_t serial_len,
                                     uint8_t *version) {
  if (serial == NULL || version == NULL || serial_len < 6) {
    return false;
  }

  if (memcmp(serial, "OKLC", 4) == 0) {
    *version = MANAGE_CARD_VERSION_SEED;
    return true;
  }
  if (serial[4] == 'T' && serial[5] == '2') {
    *version = MANAGE_CARD_VERSION_V1;
    return true;
  }
  if (serial[4] == 'B' && serial[5] == 'A') {
    *version = MANAGE_CARD_VERSION_V2;
    return true;
  }
  return false;
}

void manage_card_close_secure_channel(void) {
  lite_card_scp03_close();
  seed_card_scp03_close();
  scp11_close_secure_channel(&scp11_ctx);
}

void manage_card_reset(void) {
  manage_card_close_secure_channel();
  card_version = MANAGE_CARD_VERSION_UNKNOWN;
}

bool manage_card_open_secure_channel(void) {
  if (scp11_get_secure_channel_status(&scp11_ctx)) {
    return true;
  }

  if (!manage_card_run_scp11_handshake()) {
    goto fail;
  }
  if (!scp11_open_secure_channel(&scp11_ctx, card_version)) {
    goto fail;
  }

  // The card receipt seeds the SCP03 MAC chaining value.
  if (is_seed_card()) {
    seed_card_scp03_open(&scp11_ctx.session_key,
                         scp11_ctx.response_msg.lv_GPC_TLV_MA_RECEIPT.value);
  } else {
    lite_card_scp03_open(&scp11_ctx.session_key,
                         scp11_ctx.response_msg.lv_GPC_TLV_MA_RECEIPT.value);
  }
  // The back end holds its own copy from here on.
  memzero(&scp11_ctx.session_key, sizeof(scp11_ctx.session_key));
  return true;

fail:
  printf("secure channel FAILED, version=%u\n", (unsigned)card_version);
  manage_card_close_secure_channel();
  return false;
}

bool manage_card_data_exchange_test(void) {
  manage_card_close_secure_channel();
  bool ok = manage_card_run_scp11_handshake();
  manage_card_close_secure_channel();
  return ok;
}

bool manage_card_apdu(uint8_t *apdu, uint16_t apdu_len, uint8_t *response,
                      uint16_t *response_len, uint8_t *sw1sw2, bool safe) {
  if (apdu == NULL || response == NULL || response_len == NULL ||
      sw1sw2 == NULL || apdu_len < 4) {
    return false;
  }

  // Selecting another applet tears down whatever channel was open.
  if (memcmp(apdu, apdu_select_prefix, sizeof(apdu_select_prefix)) == 0) {
    manage_card_close_secure_channel();
  }

  if (safe) {
    if (!manage_card_open_secure_channel()) {
      return false;
    }
    bool ok = is_seed_card() ? seed_card_send_safeapdu(apdu, apdu_len, response,
                                                       response_len, sw1sw2)
                             : lite_card_send_safeapdu(apdu, apdu_len, response,
                                                       response_len, sw1sw2);
    if (!ok) {
      printf("safe apdu FAILED, sw=%X %X\n", sw1sw2[0], sw1sw2[1]);
      return false;
    }
    return true;
  }

  if (!nfc_send_recv(apdu, apdu_len, response, response_len, sw1sw2)) {
    printf("plain apdu FAILED (no answer from card)\n");
    return false;
  }

  // Auto-detect the card generation from the serial number. Every card session
  // reads the serial before it does anything else, so this lands before the
  // first secure-channel command.
  if (apdu_len == sizeof(apdu_get_serial) &&
      memcmp(apdu, apdu_get_serial, sizeof(apdu_get_serial)) == 0 &&
      sw1sw2[0] == 0x90 && sw1sw2[1] == 0x00) {
    uint8_t detected = MANAGE_CARD_VERSION_UNKNOWN;
    if (manage_card_version_from_serial(response, *response_len, &detected)) {
      manage_card_set_version(detected);
    }
  }
  return true;
}
