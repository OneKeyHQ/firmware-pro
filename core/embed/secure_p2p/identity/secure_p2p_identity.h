#pragma once

#include <stdint.h>

#include <secbool.h>
#include <util_macros.h>

// ########################################################################
// defines

#define SECURE_P2P_IDENTITY_CERT_MAX_LEN 512
#define SECURE_P2P_IDENTITY_PUBKEY_LEN   64 // P-256 X||Y
#define SECURE_P2P_IDENTITY_SIG_LEN      64 // ECDSA r||s

// certificate issuer selector carried in SELECT and F5/0A
#define SECURE_P2P_IDENTITY_CA_ID_PRO  0
#define SECURE_P2P_IDENTITY_CA_ID_PRO2 1
#define SECURE_P2P_IDENTITY_CA_ID_MAX  SECURE_P2P_IDENTITY_CA_ID_PRO2

// ########################################################################
// public functions

// sectrue for a point on P-256 that is not the point at infinity.
secbool_t __wur secure_p2p_identity_point_validate(const uint8_t point_xy[SECURE_P2P_IDENTITY_PUBKEY_LEN]);
