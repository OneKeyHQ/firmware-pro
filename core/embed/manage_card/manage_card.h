#ifndef _MANAGE_CARD_H_
#define _MANAGE_CARD_H_

#include <stdbool.h>
#include <stdint.h>

// Card generations that speak the OneKey NFC backup applet. V1/V2 are the Lite
// Card (serial "xxxxT2..." / "xxxxBA..."), SEED is the Seed Card (serial
// "OKLC..."). They share the SCP11 handshake but differ in the OCE certificate,
// the session-key KDF and the SCP03 secure messaging dialect, so every entry
// point below is dispatched on the version.
#define MANAGE_CARD_VERSION_UNKNOWN 0
#define MANAGE_CARD_VERSION_V1 1
#define MANAGE_CARD_VERSION_V2 2
#define MANAGE_CARD_VERSION_SEED 3

#define CARD_AID_MAX_LEN 32

// Largest APDU / response this layer moves in one exchange, header and SCP03
// padding included.
#define MANAGE_CARD_APDU_BUF_LEN 0x120

// Select the card generation. UNKNOWN is handled as a Lite Card.
void manage_card_set_version(uint8_t version);

// Map a card serial to a version.
bool manage_card_version_from_serial(const uint8_t* serial, uint16_t serial_len,
                                     uint8_t* version);

// SCP11 handshake plus the SCP03 session for the selected card generation.
bool manage_card_open_secure_channel(void);
void manage_card_close_secure_channel(void);

// Drop the channel and forget the card generation. For when the field goes
// down: the next card on the reader is not necessarily the same one.
void manage_card_reset(void);

// Send one APDU. safe = wrap it in the secure channel, opening the channel
// first when it is not up yet. Plain sends are also watched for the GET SERIAL
// response, which is what auto-detects the card generation.
bool manage_card_apdu(uint8_t* apdu, uint16_t apdu_len, uint8_t* response,
                      uint16_t* response_len, uint8_t* sw1sw2, bool safe);

// Factory test: run the SCP11 handshake against whatever card is on the reader
// and report whether it completed. Leaves no SCP03 session behind.
bool manage_card_data_exchange_test(void);

#endif
