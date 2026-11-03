#include <string.h>

#include "embed/secure_channels/scp03/scp03_apdu.h"

#include "secure_p2p_defines.h"
#include "secure_p2p_frame.h"
#include "secure_p2p_card.h"

// ########################################################################
// private functions

static void secure_p2p_card_reset(SecureP2p_CardSession_t* session_p)
{
    secure_channel_scp03_destroy(&session_p->scp03);
    memset(&session_p->scp03, 0, sizeof(session_p->scp03));
    session_p->state = SECURE_P2P_CARD_STATE_IDLE;
}

// Parse plaintext inner APDU header (CLA/INS/Lc), validate CLA, hand off
// to the application callback for INS-specific logic.
static secbool_t dispatch_inner(
    SecureP2p_CardDispatch_func_t dispatch, void* ctx_p, //
    const uint8_t* inner_p, uint32_t inner_len,          //
    uint8_t* out_data_p, uint32_t* out_data_len_p, uint16_t* out_sw_p
)
{
    *out_data_len_p = 0;
    *out_sw_p = SECURE_P2P_SW_INS_BAD;

    if ( inner_len < 5 )
    {
        *out_sw_p = SECURE_P2P_SW_GENERIC;
        return sectrue;
    }

    uint8_t cla = inner_p[0];
    uint8_t ins = inner_p[1];
    uint8_t lc = inner_p[4];

    if ( inner_len < 5U + (uint32_t)lc )
    {
        *out_sw_p = SECURE_P2P_SW_GENERIC;
        return sectrue;
    }

    const uint8_t* data_p = (lc > 0) ? inner_p + 5 : NULL;

    if ( cla != SECURE_P2P_CLA_APP )
    {
        *out_sw_p = SECURE_P2P_SW_INS_BAD;
        return sectrue;
    }

    return dispatch(ctx_p, ins, data_p, lc, out_data_p, out_data_len_p, out_sw_p);
}

// Handle a wrapped P2P command (CLA high nibble 0xA). Unwrap, dispatch,
// wrap response, seal with CRC32.
static secbool_t handle_wrapped(
    SecureP2p_CardSession_t* session_p,                  //
    SecureP2p_CardDispatch_func_t dispatch, void* ctx_p, //
    const uint8_t* cmd_p, uint32_t cmd_len,              //
    uint8_t* out_p, uint32_t* out_len_p, uint32_t out_cap
)
{
    if ( session_p->state != SECURE_P2P_CARD_STATE_AUTHED )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_COND_NOT, out_p, out_len_p, out_cap);
        return sectrue;
    }

    uint8_t inner[SECURE_P2P_FRAME_MAX];
    uint32_t inner_len = 0;
    if ( secure_channel_scp03_apdu_unwrap_command(&session_p->scp03, cmd_p, cmd_len, inner, &inner_len) !=
         sectrue )
    {
        secure_p2p_card_reset(session_p);
        secure_p2p_frame_build_sw(SECURE_P2P_SW_MAC_FAIL, out_p, out_len_p, out_cap);
        return sectrue;
    }

    uint8_t out_data[SECURE_P2P_FRAME_MAX];
    uint32_t out_data_len = 0;
    uint16_t out_sw = SECURE_P2P_SW_GENERIC;
    secbool_t keep_going =
        dispatch_inner(dispatch, ctx_p, inner, inner_len, out_data, &out_data_len, &out_sw);

    uint8_t wrapped[SECURE_P2P_FRAME_MAX];
    uint32_t wrapped_len = 0;
    if ( secure_channel_scp03_apdu_wrap_response(
             &session_p->scp03,                                //
             out_data, out_data_len,                           //
             (uint8_t)(out_sw >> 8), (uint8_t)(out_sw & 0xFF), //
             wrapped, &wrapped_len
         ) != sectrue )
    {
        secure_p2p_card_reset(session_p);
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        return sectrue;
    }

    if ( wrapped_len + SECURE_P2P_CRC32_LEN > out_cap )
    {
        secure_p2p_card_reset(session_p);
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        return sectrue;
    }

    memcpy(out_p, wrapped, wrapped_len);
    uint32_t out_len = wrapped_len;
    if ( secure_p2p_frame_seal(out_p, &out_len, out_cap) != sectrue )
    {
        secure_p2p_card_reset(session_p);
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_p, out_len_p, out_cap);
        return sectrue;
    }
    *out_len_p = out_len;
    return keep_going;
}

// ########################################################################
// public functions

void secure_p2p_card_init(SecureP2p_CardSession_t* session_p)
{
    memset(session_p, 0, sizeof(*session_p));
    session_p->state = SECURE_P2P_CARD_STATE_IDLE;
}

void secure_p2p_card_destroy(SecureP2p_CardSession_t* session_p)
{
    secure_p2p_card_reset(session_p);
}

secbool_t secure_p2p_card_handle_frame(
    SecureP2p_CardSession_t* session_p,                  //
    SecureP2p_CardDispatch_func_t dispatch, void* ctx_p, //
    const uint8_t* in_buf_p, uint32_t in_len,            //
    uint8_t* out_buf_p, uint32_t* out_len_p, uint32_t out_cap
)
{
    if ( secure_p2p_frame_verify(in_buf_p, &in_len) != sectrue )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_buf_p, out_len_p, out_cap);
        return sectrue;
    }

    if ( in_len < 4 )
    {
        secure_p2p_frame_build_sw(SECURE_P2P_SW_GENERIC, out_buf_p, out_len_p, out_cap);
        return sectrue;
    }

    uint8_t cla = in_buf_p[0];

    // unwrap_command verifies C-MAC, so a plaintext 0xA0 fails there.
    if ( (cla & 0xF0) == (SECURE_P2P_CLA_APP & 0xF0) )
    {
        return handle_wrapped(session_p, dispatch, ctx_p, in_buf_p, in_len, out_buf_p, out_len_p, out_cap);
    }

    secure_p2p_frame_build_sw(SECURE_P2P_SW_INS_BAD, out_buf_p, out_len_p, out_cap);
    return sectrue;
}
