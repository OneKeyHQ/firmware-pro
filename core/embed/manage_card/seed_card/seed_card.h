#ifndef _SEED_CARD_H_
#define _SEED_CARD_H_

#include <stdbool.h>
#include <stdint.h>

#include "scp11.h"

// Install the SCP11-negotiated session on the Seed Card's SCP03 back end.
// mac_chaining is the card receipt from the mutual authentication.
void seed_card_scp03_open(const scp11_session_key *session_key,
                          const uint8_t *mac_chaining);
void seed_card_scp03_close(void);

// Wrap, send and unwrap one APDU over the open SCP03 session.
bool seed_card_send_safeapdu(uint8_t *apdu, uint16_t apdu_len,
                             uint8_t *response, uint16_t *response_len,
                             uint8_t *sw1sw2);

#endif
