#pragma once

// ########################################################################
// defines

// Single-frame ceiling. An L1 frame (L2 APDU bytes + 4-byte CRC32 trailer) fits inside this.
#define SECURE_P2P_FRAME_MAX 260
#define SECURE_P2P_CRC32_LEN 4

// Application APDU class byte (plaintext, inside the SCP03 wrap).
// 0xA0 chosen so the wire CLA after `wrap_command` (0xA0 | 0x04 = 0xA4)
// round-trips cleanly through `unwrap_command`'s `(cla & 0xE0) | (cla & 0x03)`
// restore. Any CLA with bit 4 set is lost on restore - do not reuse 0xB0.
#define SECURE_P2P_CLA_APP 0xA0

// ISO 7816 status words emitted by the card-side dispatcher. Applications
// may reuse these or define their own - library code only emits the ones
// below on L1/L2/state errors.
#define SECURE_P2P_SW_OK        0x9000
#define SECURE_P2P_SW_COND_NOT  0x6985 // conditions of use not satisfied (no session / wrong state)
#define SECURE_P2P_SW_AUTH_FAIL 0x6982 // security status not satisfied (peer not verified)
#define SECURE_P2P_SW_MAC_FAIL  0x6988 // secure messaging data object incorrect (C-MAC / decrypt fail)
#define SECURE_P2P_SW_INS_BAD   0x6D00 // INS not supported
#define SECURE_P2P_SW_GENERIC   0x6F00 // generic protocol error (L1 CRC32 / length)
