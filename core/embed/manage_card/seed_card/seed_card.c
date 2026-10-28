#include <stdio.h>
#include <string.h>

#include "embed/secure_channels/scp03/scp03.h"
#include "embed/secure_channels/scp03/scp03_apdu.h"
#include "manage_card.h"
#include "memzero.h"
#include "nfc.h"
#include "seed_card.h"

// GP SCP03 session, shared implementation with the device-transfer channel
// (secure_channels/scp03). Security level is what the SCP11 handshake
// negotiates: C-MAC + C-DEC + R-MAC + R-ENC.
#define SEED_CARD_SCP03_SEC_LEVEL 0x33
#define SEED_CARD_SCP03_MAC_SIZE SECURE_CHANNEL_SCP03_MAC_LEN
static SecureChannel_Scp03Session_t scp03_ses = {0};

// ########################################################################
// secure messaging

void seed_card_scp03_open(const scp11_session_key *session_key,
                          const uint8_t *mac_chaining) {
  // mac_chaining is the card receipt. init_keys only fails on a NULL key
  // pointer; leave the session destroyed in that case so every wrap fails
  // instead of running on a half-initialised context.
  if (secure_channel_scp03_session_init_keys(
          &scp03_ses, session_key->s_enc, session_key->s_mac,
          session_key->s_rmac, SPC11_SESSION_KEY_SIZE, mac_chaining,
          SEED_CARD_SCP03_SEC_LEVEL) != sectrue) {
    printf("seed: SCP03 session init failed\n");
    secure_channel_scp03_destroy(&scp03_ses);
  }
}

void seed_card_scp03_close(void) { secure_channel_scp03_destroy(&scp03_ses); }

bool seed_card_send_safeapdu(uint8_t *apdu, uint16_t apdu_len,
                             uint8_t *response, uint16_t *response_len,
                             uint8_t *sw1sw2) {
  uint8_t plain_apdu[MANAGE_CARD_APDU_BUF_LEN] = {0};
  uint8_t wrapped[MANAGE_CARD_APDU_BUF_LEN] = {0};
  uint8_t card_response[MANAGE_CARD_APDU_BUF_LEN] = {0};
  uint32_t wrapped_len = 0, unwrapped_len = 0;
  uint16_t card_response_len = sizeof(card_response) - 2;  // room for SW
  bool ret = false;

  if (apdu == NULL || response == NULL || response_len == NULL ||
      sw1sw2 == NULL || apdu_len < 4 || apdu_len > sizeof(plain_apdu)) {
    return false;
  }

  memcpy(plain_apdu, apdu, apdu_len);
  if (apdu_len == 4) {
    // case 1 command: give it the empty data field the wrapper expects
    plain_apdu[4] = 0;
    apdu_len = 5;
  }

  if (secure_channel_scp03_apdu_wrap_command(
          &scp03_ses, plain_apdu, apdu_len, wrapped, &wrapped_len) != sectrue ||
      wrapped_len > UINT16_MAX) {
    printf("seed: SCP03 wrap failed\n");
    goto cleanup;
  }

  if (!nfc_send_recv(wrapped, (uint16_t)wrapped_len, card_response,
                     &card_response_len, sw1sw2)) {
    printf("seed: no answer to wrapped apdu\n");
    goto cleanup;
  }

  if (card_response_len < SEED_CARD_SCP03_MAC_SIZE) {
    // Error status words come back bare, with no R-MAC to unwrap - 69 85
    // (locked), 69 93 (void), 6A 88 (no such data) and friends. Pass the status
    // word up untouched so the caller can act on it. A bare 90 00 is different:
    // nothing authenticated it, so it must not read as success.
    printf("seed: bare answer, sw=%X %X len=%u\n", sw1sw2[0], sw1sw2[1],
           (unsigned)card_response_len);
    if (sw1sw2[0] == 0x90 && sw1sw2[1] == 0x00) {
      goto cleanup;
    }
    *response_len = 0;
    ret = true;
    goto cleanup;
  }

  // The shared unwrap takes the response as the card sent it, SW included;
  // nfc_send_recv split the SW off, so put it back.
  card_response[card_response_len++] = sw1sw2[0];
  card_response[card_response_len++] = sw1sw2[1];
  ret = secure_channel_scp03_apdu_unwrap_response(
            &scp03_ses, card_response, card_response_len, response,
            &unwrapped_len, &sw1sw2[0], &sw1sw2[1]) == sectrue &&
        unwrapped_len <= UINT16_MAX;
  if (ret) {
    *response_len = (uint16_t)unwrapped_len;
  } else {
    printf("seed: SCP03 unwrap failed (R-MAC or padding), sw=%X %X\n",
           sw1sw2[0], sw1sw2[1]);
  }

cleanup:
  memzero(plain_apdu, sizeof(plain_apdu));
  memzero(wrapped, sizeof(wrapped));
  memzero(card_response, sizeof(card_response));
  return ret;
}
