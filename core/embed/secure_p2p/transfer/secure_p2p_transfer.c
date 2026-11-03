#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <crypto.h>
#include <secure_p2p_defines.h>
#include <secure_p2p_frame.h>
#include <secure_p2p_identity.h>

#include "secure_p2p_transfer.h"

// ########################################################################
// defines

#define TRANSFER_PLAIN_HEADER_LEN 5
#define TRANSFER_SW_LEN           2
#define TRANSFER_CLA_WRAPPED      (SECURE_P2P_CLA_APP | 0x04) // wire CLA after SCP03 wrap_command
#define TRANSFER_PUT_LAST         0x01

// ########################################################################
// private functions

// ########## plaintext APDU framing ##########

static void transfer_plain_reply(
    uint16_t sw, const uint8_t* data_p, uint32_t data_len, uint8_t* out_p, uint32_t* out_len_p,
    uint32_t out_cap
)
{
    uint32_t len = data_len + TRANSFER_SW_LEN;
    if ( len + SECURE_P2P_CRC32_LEN > out_cap )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        return;
    }
    if ( data_len > 0 )
    {
        memcpy(out_p, data_p, data_len);
    }
    out_p[data_len] = (uint8_t)(sw >> 8);
    out_p[data_len + 1] = (uint8_t)(sw & 0xFF);
    *out_len_p = (secure_p2p_frame_seal(out_p, &len, out_cap) == sectrue) ? len : 0;
}

// ########## chunked messages ##########

static uint32_t transfer_be16(const uint8_t* data_p)
{
    return ((uint32_t)data_p[0] << 8) | data_p[1];
}

static void transfer_put_be16(uint8_t* out_p, uint32_t value)
{
    out_p[0] = (uint8_t)(value >> 8);
    out_p[1] = (uint8_t)value;
}

// Build one GET-style reply from msg_p at offset (offset < msg_len); returns the chunk length.
static uint32_t transfer_chunk_reply(
    const uint8_t* msg_p, uint32_t msg_len, uint32_t offset, uint8_t* out_data_p, uint32_t* out_len_p
)
{
    uint32_t chunk = msg_len - offset;
    if ( chunk > SECURE_P2P_TRANSFER_CHUNK_LEN )
    {
        chunk = SECURE_P2P_TRANSFER_CHUNK_LEN;
    }
    transfer_put_be16(out_data_p, msg_len);
    memcpy(out_data_p + SECURE_P2P_TRANSFER_GET_HEADER_LEN, msg_p + offset, chunk);
    *out_len_p = SECURE_P2P_TRANSFER_GET_HEADER_LEN + chunk;
    return chunk;
}

static secbool_t transfer_envelope_len_valid(uint32_t len)
{
    if ( len < SECURE_P2P_TRANSFER_ENVELOPE_MIN_LEN || len > SECURE_P2P_TRANSFER_ENVELOPE_MAX_LEN )
    {
        return secfalse;
    }
    return ((len - SECURE_P2P_TRANSFER_ENVELOPE_FIXED_LEN) % SECURE_P2P_TRANSFER_ENVELOPE_BLOCK_LEN == 0)
             ? sectrue
             : secfalse;
}

// ########## card ##########

static void transfer_card_clear(SecureP2p_TransferCardSession_t* ses_p)
{
    secure_p2p_handshake_destroy(&ses_p->handshake);
    secure_p2p_card_destroy(&ses_p->p2p);
    crypto_utils_memzero_explicit(ses_p->peer_auth, sizeof(ses_p->peer_auth));
    crypto_utils_memzero_explicit(ses_p->own_auth, sizeof(ses_p->own_auth));
    crypto_utils_memzero_explicit(ses_p->envelope, sizeof(ses_p->envelope));
    ses_p->peer_auth_len = 0;
    ses_p->own_auth_len = 0;
    ses_p->envelope_len = 0;
    crypto_utils_memzero_explicit(ses_p->batch_keys, sizeof(ses_p->batch_keys));
    ses_p->batch_keys_len = 0;
    ses_p->batch_offset = 0;
    ses_p->batch_count = 0;
    ses_p->batch_index = 0;
}

static void transfer_card_wipe(SecureP2p_TransferCardSession_t* ses_p)
{
    transfer_card_clear(ses_p);
    ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_ENDED;
}

static secbool_t transfer_card_in_wrapped_phase(const SecureP2p_TransferCardSession_t* ses_p)
{
    return (ses_p->state == SECURE_P2P_TRANSFER_CARD_STATE_EXCHANGED ||
            ses_p->state == SECURE_P2P_TRANSFER_CARD_STATE_AUTHED ||
            ses_p->state == SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED)
             ? sectrue
             : secfalse;
}

static secbool_t transfer_card_plain(
    SecureP2p_TransferCardSession_t* ses_p, uint8_t ins, const uint8_t* data_p, uint32_t data_len,
    uint8_t* out_p, uint32_t* out_len_p, uint32_t out_cap
)
{
    uint8_t reply[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN];
    uint32_t reply_len = 0;
    uint16_t sw = SECURE_P2P_SW_COND_NOT;

    switch ( ins )
    {
    case SECURE_P2P_TRANSFER_INS_SELECT:
        if ( data_len != 1 || data_p[0] > SECURE_P2P_IDENTITY_CA_ID_MAX )
        {
            break;
        }
        // a reader that missed the reply resends SELECT: restart cleanly, like INIT UPDATE in
        // secure_p2p_card. The card commits to a fresh EXCHANGE_B here, before it can see EXCHANGE_A.
        transfer_card_clear(ses_p);
        if ( secure_p2p_handshake_start(&ses_p->handshake, ses_p->cfg_p, reply) != sectrue ||
             secure_p2p_handshake_commit_build(&ses_p->handshake, reply + 2) != sectrue )
        {
            sw = SECURE_P2P_SW_GENERIC;
            break;
        }
        ses_p->handshake.peer_ca_id = data_p[0];
        reply[0] = SECURE_P2P_TRANSFER_PROTOCOL_VERSION;
        reply[1] = ses_p->cfg_p->ca_id;
        reply_len = SECURE_P2P_TRANSFER_SELECT_RESP_LEN;
        ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_SELECTED;
        sw = SECURE_P2P_SW_OK;
        break;

    case SECURE_P2P_TRANSFER_INS_EXCHANGE:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_SELECTED ||
             data_len != SECURE_P2P_HANDSHAKE_EXCHANGE_LEN )
        {
            break;
        }
        if ( secure_p2p_handshake_derive(&ses_p->handshake, ses_p->cfg_p, data_p) != sectrue ||
             secure_p2p_handshake_install_card(&ses_p->handshake, &ses_p->p2p) != sectrue )
        {
            sw = SECURE_P2P_SW_GENERIC;
            break;
        }
        memcpy(reply, ses_p->handshake.own_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN);
        reply_len = SECURE_P2P_HANDSHAKE_EXCHANGE_LEN;
        ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_EXCHANGED;
        sw = SECURE_P2P_SW_OK;
        break;

    default:
        sw = SECURE_P2P_SW_INS_BAD;
        break;
    }

    transfer_plain_reply(sw, reply, reply_len, out_p, out_len_p, out_cap);
    crypto_utils_memzero_explicit(reply, sizeof(reply));
    if ( sw != SECURE_P2P_SW_OK )
    {
        transfer_card_wipe(ses_p);
        return secfalse;
    }
    return sectrue;
}

// R_0 .. R_(count-1): valid P-256 points, pairwise distinct
static secbool_t
transfer_batch_keys_valid(const uint8_t keys[SECURE_P2P_TRANSFER_BATCH_KEYS_LEN], uint32_t count)
{
    for ( uint32_t i = 0; i < count; i++ )
    {
        const uint8_t* key_p = keys + i * SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN;
        if ( secure_p2p_identity_point_validate(key_p) != sectrue )
        {
            return secfalse;
        }
        for ( uint32_t j = 0; j < i; j++ )
        {
            if ( memcmp(
                     key_p, keys + j * SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN,
                     SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN
                 ) == 0 )
            {
                return secfalse;
            }
        }
    }
    return sectrue;
}

static secbool_t transfer_card_verify_peer(SecureP2p_TransferCardSession_t* ses_p)
{
    uint8_t proof[SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN];
    uint32_t proof_len = 0;
    secbool_t ret = secure_p2p_handshake_peer_proof(
        &ses_p->handshake, ses_p->peer_auth, ses_p->peer_auth_len, proof, &proof_len
    );
    if ( ret == sectrue )
    {
        ret = ses_p->app_p->verify_peer(ses_p->app_p->ctx_p, proof, proof_len);
    }
    crypto_utils_memzero_explicit(proof, sizeof(proof));
    return ret;
}

static secbool_t
transfer_card_export(SecureP2p_TransferCardSession_t* ses_p, const uint8_t* r_xy_p, uint16_t* err_sw_p)
{
    uint8_t proof[SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN];
    uint32_t proof_len = 0;
    secbool_t ret = secure_p2p_handshake_peer_proof(
        &ses_p->handshake, ses_p->peer_auth, ses_p->peer_auth_len, proof, &proof_len
    );
    if ( ret == sectrue )
    {
        ret = ses_p->app_p->export_envelope(
            ses_p->app_p->ctx_p, proof, proof_len, r_xy_p, ses_p->envelope, &ses_p->envelope_len,
            sizeof(ses_p->envelope), err_sw_p
        );
    }
    crypto_utils_memzero_explicit(proof, sizeof(proof));
    return ret;
}

// wrapped-phase dispatcher, called by secure_p2p_card after unwrap
static secbool_t transfer_card_wrapped(
    void* ctx_p, uint8_t ins, const uint8_t* data_p, uint32_t data_len, uint8_t* out_data_p,
    uint32_t* out_len_p, uint16_t* out_sw_p
)
{
    SecureP2p_TransferCardSession_t* ses_p = (SecureP2p_TransferCardSession_t*)ctx_p;
    uint32_t chunk = 0;
    uint32_t offset = 0;
    uint8_t ready = 0;
    uint32_t remaining_ms = 0;
    uint16_t err_sw = SECURE_P2P_TRANSFER_SW_SE_FAILED;
    secbool_t keep = secfalse;

    *out_len_p = 0;
    *out_sw_p = SECURE_P2P_SW_COND_NOT;

    switch ( ins )
    {
    case SECURE_P2P_TRANSFER_INS_AUTH_PUT:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_EXCHANGED )
        {
            break;
        }
        if ( data_len <= SECURE_P2P_TRANSFER_PUT_HEADER_LEN ||
             (data_p[0] != 0x00 && data_p[0] != TRANSFER_PUT_LAST) )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        chunk = data_len - SECURE_P2P_TRANSFER_PUT_HEADER_LEN;
        if ( chunk > SECURE_P2P_TRANSFER_CHUNK_LEN ||
             chunk > SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN - ses_p->peer_auth_len )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        memcpy(ses_p->peer_auth + ses_p->peer_auth_len, data_p + SECURE_P2P_TRANSFER_PUT_HEADER_LEN, chunk);
        ses_p->peer_auth_len += chunk;
        if ( data_p[0] != TRANSFER_PUT_LAST )
        {
            *out_sw_p = SECURE_P2P_SW_OK;
            keep = sectrue;
            break;
        }
        if ( secure_p2p_handshake_auth_verify(
                 &ses_p->handshake, ses_p->cfg_p, ses_p->peer_auth, ses_p->peer_auth_len
             ) != sectrue ||
             transfer_card_verify_peer(ses_p) != sectrue )
        {
            *out_sw_p = SECURE_P2P_SW_AUTH_FAIL;
            break;
        }
        ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_AUTHED;
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_AUTH_GET:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_AUTHED )
        {
            break;
        }
        if ( data_len != SECURE_P2P_TRANSFER_OFFSET_LEN )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        // the card signs once, when the verified host first asks for its identity
        if ( ses_p->own_auth_len == 0 &&
             secure_p2p_handshake_auth_build(
                 &ses_p->handshake, ses_p->cfg_p, ses_p->own_auth, &ses_p->own_auth_len
             ) != sectrue )
        {
            *out_sw_p = SECURE_P2P_SW_GENERIC;
            break;
        }
        offset = transfer_be16(data_p);
        if ( offset >= ses_p->own_auth_len )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        transfer_chunk_reply(ses_p->own_auth, ses_p->own_auth_len, offset, out_data_p, out_len_p);
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_GET_STATUS:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_AUTHED &&
             ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED )
        {
            break;
        }
        if ( data_len != 0 )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        if ( ses_p->app_p->status != NULL )
        {
            ses_p->app_p->status(ses_p->app_p->ctx_p, &ready, &remaining_ms);
        }
        out_data_p[0] = ready;
        out_data_p[1] = (uint8_t)(remaining_ms >> 24);
        out_data_p[2] = (uint8_t)(remaining_ms >> 16);
        out_data_p[3] = (uint8_t)(remaining_ms >> 8);
        out_data_p[4] = (uint8_t)remaining_ms;
        *out_len_p = SECURE_P2P_TRANSFER_STATUS_LEN;
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_BATCH_KEYS:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_AUTHED || ses_p->envelope_len != 0 ||
             ses_p->batch_count != 0 || ses_p->batch_index != 0 )
        {
            break;
        }
        if ( data_len <= SECURE_P2P_TRANSFER_PUT_HEADER_LEN ||
             (data_p[0] != 0x00 && data_p[0] != TRANSFER_PUT_LAST) )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        chunk = data_len - SECURE_P2P_TRANSFER_PUT_HEADER_LEN;
        if ( chunk > SECURE_P2P_TRANSFER_CHUNK_LEN ||
             chunk > sizeof(ses_p->batch_keys) - ses_p->batch_keys_len )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        memcpy(ses_p->batch_keys + ses_p->batch_keys_len, data_p + SECURE_P2P_TRANSFER_PUT_HEADER_LEN, chunk);
        ses_p->batch_keys_len += chunk;
        // LAST fixes n = total / 64; a rejected LAST ends the session
        if ( data_p[0] == TRANSFER_PUT_LAST )
        {
            ses_p->batch_count = (uint8_t)(ses_p->batch_keys_len / SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN);
        }
        if ( (data_p[0] == TRANSFER_PUT_LAST &&
              (ses_p->batch_keys_len % SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN != 0 ||
               transfer_batch_keys_valid(ses_p->batch_keys, ses_p->batch_count) != sectrue)) ||
             (data_p[0] != TRANSFER_PUT_LAST && ses_p->batch_keys_len == sizeof(ses_p->batch_keys)) )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_BATCH_GET:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_AUTHED ||
             ses_p->batch_index >= ses_p->batch_count )
        {
            break;
        }
        if ( data_len != SECURE_P2P_TRANSFER_BATCH_GET_REQ_LEN || data_p[0] != ses_p->batch_index ||
             transfer_be16(data_p + 1) != ses_p->batch_offset )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        if ( ses_p->batch_offset == 0 )
        {
            if ( transfer_card_export(
                     ses_p, ses_p->batch_keys + ses_p->batch_index * SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN,
                     &err_sw
                 ) != sectrue )
            {
                *out_sw_p = err_sw;
                break;
            }
            if ( transfer_envelope_len_valid(ses_p->envelope_len) != sectrue )
            {
                *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
                break;
            }
        }
        chunk = transfer_chunk_reply(
            ses_p->envelope, ses_p->envelope_len, ses_p->batch_offset, out_data_p, out_len_p
        );
        ses_p->batch_offset += chunk;
        if ( ses_p->batch_offset == ses_p->envelope_len )
        {
            crypto_utils_memzero_explicit(ses_p->envelope, sizeof(ses_p->envelope));
            ses_p->envelope_len = 0;
            ses_p->batch_offset = 0;
            if ( ++ses_p->batch_index == ses_p->batch_count )
            {
                ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED;
            }
        }
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_EXPORT:
        if ( ses_p->state != SECURE_P2P_TRANSFER_CARD_STATE_AUTHED || ses_p->batch_keys_len != 0 )
        {
            break;
        }
        if ( data_len == SECURE_P2P_TRANSFER_ENVELOPE_PUBKEY_LEN )
        {
            if ( ses_p->envelope_len != 0 )
            {
                break;
            }
            if ( secure_p2p_identity_point_validate(data_p) != sectrue )
            {
                *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
                break;
            }
            if ( transfer_card_export(ses_p, data_p, &err_sw) != sectrue )
            {
                *out_sw_p = err_sw;
                break;
            }
            if ( transfer_envelope_len_valid(ses_p->envelope_len) != sectrue )
            {
                *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
                break;
            }
            offset = 0;
        }
        else if ( data_len == SECURE_P2P_TRANSFER_OFFSET_LEN )
        {
            offset = transfer_be16(data_p);
            if ( ses_p->envelope_len == 0 || offset >= ses_p->envelope_len )
            {
                *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
                break;
            }
        }
        else
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        chunk = transfer_chunk_reply(ses_p->envelope, ses_p->envelope_len, offset, out_data_p, out_len_p);
        if ( offset + chunk == ses_p->envelope_len )
        {
            crypto_utils_memzero_explicit(ses_p->envelope, sizeof(ses_p->envelope));
            ses_p->envelope_len = 0;
            ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_EXPORTED;
        }
        *out_sw_p = SECURE_P2P_SW_OK;
        keep = sectrue;
        break;

    case SECURE_P2P_TRANSFER_INS_RELEASE:
        if ( data_len != 0 || (ses_p->batch_keys_len != 0 &&
                               (ses_p->batch_count == 0 || ses_p->batch_index != ses_p->batch_count)) )
        {
            *out_sw_p = SECURE_P2P_TRANSFER_SW_WRONG_DATA;
            break;
        }
        *out_sw_p = SECURE_P2P_SW_OK;
        break;

    default:
        *out_sw_p = SECURE_P2P_SW_INS_BAD;
        break;
    }

    if ( keep != sectrue )
    {
        crypto_utils_memzero_explicit(ses_p->envelope, sizeof(ses_p->envelope));
        ses_p->envelope_len = 0;
    }
    return keep;
}

// ########################################################################
// public functions

secbool_t secure_p2p_transfer_card_init(
    SecureP2p_TransferCardSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const SecureP2p_TransferCardApp_t* app_p
)
{
    if ( ses_p == NULL || cfg_p == NULL || app_p == NULL || cfg_p->ca_id > SECURE_P2P_IDENTITY_CA_ID_MAX ||
         app_p->verify_peer == NULL || app_p->export_envelope == NULL )
    {
        return secfalse;
    }
    crypto_utils_memzero_explicit(ses_p, sizeof(*ses_p));
    secure_p2p_handshake_init(&ses_p->handshake);
    secure_p2p_card_init(&ses_p->p2p);
    ses_p->cfg_p = cfg_p;
    ses_p->app_p = app_p;
    ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_IDLE;
    return sectrue;
}

void secure_p2p_transfer_card_destroy(SecureP2p_TransferCardSession_t* ses_p)
{
    if ( ses_p == NULL )
    {
        return;
    }
    transfer_card_wipe(ses_p);
    ses_p->state = SECURE_P2P_TRANSFER_CARD_STATE_IDLE;
}

secbool_t secure_p2p_transfer_card_handle_frame(
    SecureP2p_TransferCardSession_t* ses_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p, uint32_t out_cap
)
{
    uint32_t body_len = in_len;

    if ( out_len_p != NULL )
    {
        *out_len_p = 0;
    }
    if ( ses_p == NULL || in_p == NULL || out_p == NULL || out_len_p == NULL )
    {
        return secfalse;
    }
    if ( ses_p->state == SECURE_P2P_TRANSFER_CARD_STATE_ENDED )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_COND_NOT, out_p, out_len_p, out_cap);
        return secfalse;
    }
    if ( in_len > SECURE_P2P_FRAME_MAX )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        transfer_card_wipe(ses_p);
        return secfalse;
    }
    if ( secure_p2p_frame_verify(in_p, &body_len) != sectrue || body_len < TRANSFER_PLAIN_HEADER_LEN )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        return sectrue;
    }

    uint8_t cla = in_p[0];
    if ( transfer_card_in_wrapped_phase(ses_p) == sectrue )
    {
        // once the keys are installed only wrapped frames and a plaintext SELECT (restart) are legal
        if ( cla != TRANSFER_CLA_WRAPPED &&
             (cla != SECURE_P2P_CLA_APP || in_p[1] != SECURE_P2P_TRANSFER_INS_SELECT) )
        {
            secure_p2p_frame_build_sw(SECURE_P2P_SW_INS_BAD, out_p, out_len_p, out_cap);
            transfer_card_wipe(ses_p);
            return secfalse;
        }
        if ( cla == TRANSFER_CLA_WRAPPED )
        {
            secbool_t keep = secure_p2p_card_handle_frame(
                &ses_p->p2p, transfer_card_wrapped, ses_p, in_p, in_len, out_p, out_len_p, out_cap
            );
            // the SW trails every reply in the clear; anything but 9000 (card layer refusal or dispatcher
            // error) ends a transfer
            if ( *out_len_p < TRANSFER_SW_LEN + SECURE_P2P_CRC32_LEN ||
                 out_p[*out_len_p - SECURE_P2P_CRC32_LEN - TRANSFER_SW_LEN] !=
                     (uint8_t)(SECURE_P2P_SW_OK >> 8) ||
                 out_p[*out_len_p - SECURE_P2P_CRC32_LEN - 1] != (uint8_t)(SECURE_P2P_SW_OK & 0xFF) )
            {
                keep = secfalse;
            }
            if ( keep != sectrue )
            {
                transfer_card_wipe(ses_p);
            }
            return keep;
        }
    }

    if ( cla != SECURE_P2P_CLA_APP || (uint32_t)TRANSFER_PLAIN_HEADER_LEN + in_p[4] != body_len )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_INS_BAD, out_p, out_len_p, out_cap);
        transfer_card_wipe(ses_p);
        return secfalse;
    }
    return transfer_card_plain(
        ses_p, in_p[1], in_p + TRANSFER_PLAIN_HEADER_LEN, in_p[4], out_p, out_len_p, out_cap
    );
}
