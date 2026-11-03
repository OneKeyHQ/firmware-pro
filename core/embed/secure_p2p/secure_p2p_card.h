#pragma once

#include <stddef.h>
#include <stdint.h>

#include "embed/secure_channels/scp03/scp03.h"
#include "embed/secure_channels/secure_channel_port.h"

// ########################################################################
// defines

typedef enum
{
    SECURE_P2P_CARD_STATE_IDLE = 0, // no session (before key install, or after failure)
    SECURE_P2P_CARD_STATE_AUTHED,   // session keys installed, wrapped commands accepted
} SecureP2p_CardState_t;

typedef struct
{
    SecureChannel_Scp03Session_t scp03;
    SecureP2p_CardState_t state;
} SecureP2p_CardSession_t;

// Application dispatch callback. Called once per wrapped frame, after L1
// CRC32 verify and SCP03 unwrap_command have produced the plaintext
// inner APDU. The library already checked CLA == SECURE_P2P_CLA_APP and
// parsed Lc; the callback receives INS + data view + writable response
// buffer.
//
// Inputs:
//   ctx_p         - opaque pointer, passed through from handle_frame
//   ins           - inner APDU INS byte (application's choice)
//   data_p        - inner APDU data field (Lc bytes); NULL if Lc == 0
//   data_len      - single-byte Lc (0..255)
//
// Outputs:
//   out_data_p    - response body (no SW). Buffer is SECURE_P2P_FRAME_MAX
//                   bytes; keep body small enough that body + SW + C-MAC
//                   + R-MAC + CRC32 all fit.
//   out_len_p     - response body length written
//   out_sw_p      - 16-bit status word (e.g. SECURE_P2P_SW_OK)
//
// Return: sectrue to stay in the listen loop, secfalse to end the session
// cleanly after sending this response. secbool_t (not plain bool) is used
// so a single bit flip on the wire/in memory cannot silently flip "keep
// going" into "end session" or vice versa.
typedef secbool_t (*SecureP2p_CardDispatch_func_t
)(void* ctx_p,                              //
  uint8_t ins,                              //
  const uint8_t* data_p, uint32_t data_len, //
  uint8_t* out_data_p, uint32_t* out_len_p, //
  uint16_t* out_sw_p                        //
);

// ########################################################################
// public functions

// Initialise session state to IDLE with a clean SCP03 context.
void secure_p2p_card_init(SecureP2p_CardSession_t* ses_p);

// Wipe session state. Calls secure_channel_scp03_destroy.
void secure_p2p_card_destroy(SecureP2p_CardSession_t* ses_p);

// Handle one L1 frame received over the transport. Produces a sealed L1
// response ready to send back. The input buffer is not modified.
//
// Returns:
//   sectrue  - keep listening: transport should send out_buf_p, then
//              receive the next L1 frame and call again.
//   secfalse - session ending (clean END or fatal): transport should send
//              out_buf_p once more, then drop the link.
secbool_t __wur secure_p2p_card_handle_frame(
    SecureP2p_CardSession_t* ses_p,                      //
    SecureP2p_CardDispatch_func_t dispatch, void* ctx_p, //
    const uint8_t* in_buf_p, uint32_t in_len,            //
    uint8_t* out_buf_p, uint32_t* out_len_p, uint32_t out_cap
);
