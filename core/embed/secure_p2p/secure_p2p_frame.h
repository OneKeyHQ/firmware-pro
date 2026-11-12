#pragma once

#include <stddef.h>
#include <stdint.h>

#include "embed/secure_channels/secure_channel_port.h"

// ########################################################################
// public functions

// Append CRC32 (big-endian) to buf_p[0..*len_p) in place.
// On success *len_p grows by 4. Returns secfalse if capacity is short.
// Lengths use uint32_t for arithmetic headroom; an L1 frame is capped at
// SECURE_P2P_FRAME_MAX (260) bytes, not >64KB.
secbool_t __wur secure_p2p_frame_seal(uint8_t* buf_p, uint32_t* len_p, uint32_t capacity);

// Verify + strip the trailing CRC32 of an L1 frame. Returns sectrue iff
// the trailing CRC matches the body. On success *len_p shrinks by 4. On
// failure the buffer is left as-is.
secbool_t __wur secure_p2p_frame_verify(const uint8_t* buf_p, uint32_t* len_p);

// Build an SW-only response frame: [SW1][SW2][CRC32]. Requires capacity
// >= 2 + SECURE_P2P_CRC32_LEN.
void secure_p2p_frame_build_sw(uint16_t sw, uint8_t* out_p, uint32_t* out_len_p, uint32_t capacity);
