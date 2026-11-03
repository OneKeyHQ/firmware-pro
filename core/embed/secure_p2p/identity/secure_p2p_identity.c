#include <stddef.h>

#include <crypto.h>

#include "secure_p2p_identity.h"

// ########################################################################
// public functions

secbool_t secure_p2p_identity_point_validate(const uint8_t point_xy[SECURE_P2P_IDENTITY_PUBKEY_LEN])
{
    if ( point_xy == NULL )
    {
        return secfalse;
    }
    return crypto_sign_ecdsa_secp256r1Small_validate_pubkey(point_xy, SECURE_P2P_IDENTITY_PUBKEY_LEN);
}
