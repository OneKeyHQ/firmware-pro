#pragma once

#include <stddef.h>
#include <stdint.h>

#include <secbool.h>
#include <secure_p2p_card.h>
#include <secure_p2p_handshake.h>
#include <util_macros.h>

// ########################################################################
// defines

// Device-to-device mnemonic transfer over secure_p2p + handshake.

#define SECURE_P2P_TRANSFER_PROTOCOL_VERSION 0x04 // bump together with the handshake transcript label

#define SECURE_P2P_TRANSFER_INS_SELECT       0x01
#define SECURE_P2P_TRANSFER_INS_EXCHANGE     0x02
#define SECURE_P2P_TRANSFER_INS_AUTH_PUT     0x03
#define SECURE_P2P_TRANSFER_INS_AUTH_GET     0x04
#define SECURE_P2P_TRANSFER_INS_GET_STATUS   0x05
#define SECURE_P2P_TRANSFER_INS_EXPORT       0x06
#define SECURE_P2P_TRANSFER_INS_RELEASE      0x07
#define SECURE_P2P_TRANSFER_INS_BATCH_KEYS   0x08
#define SECURE_P2P_TRANSFER_INS_BATCH_GET    0x09

#define SECURE_P2P_TRANSFER_BATCH_COUNT      4U // maximum; a batch carries 1..4 envelopes
#define SECURE_P2P_TRANSFER_BATCH_KEYS_LEN \
  (SECURE_P2P_TRANSFER_BATCH_COUNT * SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN)

// EXPORT failure reasons reported by the card, carried as the SW
#define SECURE_P2P_TRANSFER_SW_SE_FAILED 0x6306
// a data field, chunk, point or envelope that is not as specified
#define SECURE_P2P_TRANSFER_SW_WRONG_DATA     0x6A80

#define SECURE_P2P_TRANSFER_SELECT_RESP_LEN   (2 + SECURE_P2P_HANDSHAKE_COMMIT_LEN)
#define SECURE_P2P_TRANSFER_STATUS_LEN        5
#define SECURE_P2P_TRANSFER_PUT_HEADER_LEN    1                                    // [last]
#define SECURE_P2P_TRANSFER_GET_HEADER_LEN    2                                    // total, BE16
#define SECURE_P2P_TRANSFER_OFFSET_LEN        2                                    // BE16
#define SECURE_P2P_TRANSFER_BATCH_GET_REQ_LEN (1 + SECURE_P2P_TRANSFER_OFFSET_LEN) // index || offset
#define SECURE_P2P_TRANSFER_CHUNK_LEN         80
// The Pro1 PN532 (FSC 64) hands its MCU at most four chained I-blocks;
// 125 bytes keeps every frame within three.
#define SECURE_P2P_TRANSFER_WIRE_MAX   125
#define SECURE_P2P_TRANSFER_PAD16(len) ((((len) / 16) + 1) * 16)
_Static_assert(
    5 + SECURE_P2P_TRANSFER_PAD16(SECURE_P2P_TRANSFER_PUT_HEADER_LEN + SECURE_P2P_TRANSFER_CHUNK_LEN) + 8 +
            4 <=
        SECURE_P2P_TRANSFER_WIRE_MAX,
    "an AUTH_PUT frame must fit SECURE_P2P_TRANSFER_WIRE_MAX"
);
_Static_assert(
    SECURE_P2P_TRANSFER_PAD16(SECURE_P2P_TRANSFER_GET_HEADER_LEN + SECURE_P2P_TRANSFER_CHUNK_LEN) + 2 + 8 +
            4 <=
        SECURE_P2P_TRANSFER_WIRE_MAX,
    "a chunked reply must fit SECURE_P2P_TRANSFER_WIRE_MAX"
);
_Static_assert(
    5 + SECURE_P2P_HANDSHAKE_EXCHANGE_LEN + 4 <= SECURE_P2P_TRANSFER_WIRE_MAX,
    "the plaintext EXCHANGE frame must fit SECURE_P2P_TRANSFER_WIRE_MAX"
);

// Envelope as the SE defines it: hdr (ver 0x01 || ptype 0x01) || D (64) || IV (16) || C (16..256, 16-aligned)
// || tag (16)
#define SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN 64
#define SECURE_P2P_TRANSFER_ENVELOPE_HDR_LEN    2
#define SECURE_P2P_TRANSFER_ENVELOPE_BLOCK_LEN  16
#define SECURE_P2P_TRANSFER_ENVELOPE_FIXED_LEN \
  (SECURE_P2P_TRANSFER_ENVELOPE_HDR_LEN + SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN + 16 + 16) // 98
#define SECURE_P2P_TRANSFER_ENVELOPE_MIN_LEN \
  (SECURE_P2P_TRANSFER_ENVELOPE_FIXED_LEN + SECURE_P2P_TRANSFER_ENVELOPE_BLOCK_LEN)         // 114
#define SECURE_P2P_TRANSFER_ENVELOPE_MAX_LEN (SECURE_P2P_TRANSFER_ENVELOPE_FIXED_LEN + 256) // 354

typedef enum
{
    SECURE_P2P_TRANSFER_CARD_STATE_IDLE = 0,
    SECURE_P2P_TRANSFER_CARD_STATE_SELECTED,  // EXCHANGE_B generated and committed
    SECURE_P2P_TRANSFER_CARD_STATE_EXCHANGED, // keys installed, only AUTH_PUT and RELEASE are served until
                                              // the host is verified
    SECURE_P2P_TRANSFER_CARD_STATE_AUTHED,    // host verified
    SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED,  // all negotiated envelopes delivered and wiped, only GET_STATUS
                                              // / RELEASE left
    SECURE_P2P_TRANSFER_CARD_STATE_ENDED,
} SecureP2p_TransferCardState_t;

// GET_STATUS source: ready = the user confirmed the start on the card,
// remaining_ms = time left in the card's session window.
typedef void (*SecureP2p_TransferStatus_func_t)(void* ctx_p, uint8_t* ready_p, uint32_t* remaining_ms_p);

// F5/0A on the sending SE, before AUTH_B is built.
typedef secbool_t (*SecureP2p_TransferVerifyPeer_func_t
)(void* ctx_p, const uint8_t* proof_p, uint32_t proof_len);

// EXPORT: verify the proof in the SE before each export to r_xy (already on
// the curve) and write the ENV (<= env_cap); on failure set *err_sw_p to a
// SECURE_P2P_TRANSFER_SW_* reason. Wipe per-call secrets before returning; a
// batch caller may keep its authorization until the last export of the batch,
// a failure or cancellation within the same session.
typedef secbool_t (*SecureP2p_TransferExport_func_t
)(void* ctx_p, const uint8_t* proof_p, uint32_t proof_len,
  const uint8_t r_xy[SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN], uint8_t* env_p, uint32_t* env_len_p,
  uint32_t env_cap, uint16_t* err_sw_p);

typedef struct
{
    SecureP2p_TransferStatus_func_t status;
    SecureP2p_TransferVerifyPeer_func_t verify_peer;
    SecureP2p_TransferExport_func_t export_envelope;
    void* ctx_p;
} SecureP2p_TransferCardApp_t;

typedef struct
{
    SecureP2p_TransferCardState_t state;
    const SecureP2p_HandshakeCfg_t* cfg_p;
    const SecureP2p_TransferCardApp_t* app_p;
    SecureP2p_HandshakeSession_t handshake;
    SecureP2p_CardSession_t p2p;
    uint8_t peer_auth[SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN];
    uint32_t peer_auth_len; // bytes received so far
    uint8_t own_auth[SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN];
    uint32_t own_auth_len;
    uint8_t envelope[SECURE_P2P_TRANSFER_ENVELOPE_MAX_LEN];
    uint32_t envelope_len;
    uint8_t batch_keys[SECURE_P2P_TRANSFER_BATCH_KEYS_LEN];
    uint16_t batch_keys_len;
    uint16_t batch_offset;
    uint8_t batch_count; // set by the LAST BATCH_KEYS chunk
    uint8_t batch_index;
} SecureP2p_TransferCardSession_t;

// ########################################################################
// public functions

// cfg_p and app_p must outlive the session.
secbool_t __wur secure_p2p_transfer_card_init(
    SecureP2p_TransferCardSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const SecureP2p_TransferCardApp_t* app_p
);
void secure_p2p_transfer_card_destroy(SecureP2p_TransferCardSession_t* ses_p);

// Always produces a sealed reply; on secfalse send it, then drop the link. A
// plaintext SELECT restarts any session before ENDED; ENDED needs init again.
secbool_t __wur secure_p2p_transfer_card_handle_frame(
    SecureP2p_TransferCardSession_t* ses_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p, uint32_t out_cap
);
