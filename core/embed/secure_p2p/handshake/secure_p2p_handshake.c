#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <crypto.h>
#include <secure_p2p_identity.h>

#include "secure_p2p_handshake.h"

// ########################################################################
// defines

// ExecuteCheck_ADV declares its own `ret` and would shadow the local one
#define HANDSHAKE_EXEC_TRUE_GOTO(expr) \
  do                                   \
  {                                    \
    if ( (expr) != sectrue )           \
    {                                  \
      goto cleanup;                    \
    }                                  \
  }                                    \
  while ( 0 )

#define HANDSHAKE_LABEL_TRANSCRIPT "onekey-p2p-v4" // moves together with SECURE_P2P_TRANSFER_PROTOCOL_VERSION
#define HANDSHAKE_LABEL_AUTH       "onekey-p2p-auth-v1"
#define HANDSHAKE_LABEL_HOST       "A"
#define HANDSHAKE_LABEL_CARD       "B"
#define HANDSHAKE_INFO_ENC         "enc"
#define HANDSHAKE_INFO_MAC         "mac"
#define HANDSHAKE_INFO_RMAC        "rmac"
#define HANDSHAKE_INFO_CONFIRM     "confirm"
#define HANDSHAKE_STR_LEN(str)     (sizeof(str) - 1)

#define HANDSHAKE_COORD_LEN        32
#define HANDSHAKE_START_ATTEMPTS   4
#define HANDSHAKE_INFO_MAX_LEN     15
#define HANDSHAKE_ROLE_LABEL_LEN   1
#define HANDSHAKE_AUTH_TRAILER_LEN \
  (SECURE_P2P_HANDSHAKE_SIG_LEN + SECURE_P2P_HANDSHAKE_TAG_LEN) // sigma || tag after the certificate

#define HANDSHAKE_EXCHANGE_PUB_OFFSET       0
#define HANDSHAKE_EXCHANGE_NONCE_OFFSET     (HANDSHAKE_EXCHANGE_PUB_OFFSET + SECURE_P2P_HANDSHAKE_PUBKEY_LEN)
#define HANDSHAKE_EXCHANGE_CERT_HASH_OFFSET (HANDSHAKE_EXCHANGE_NONCE_OFFSET + SECURE_P2P_HANDSHAKE_NONCE_LEN)

// ########################################################################
// variables

// P-256 generator point, X||Y. Scalar multiplication by G through the ECDH
// primitive is how an ephemeral public key is derived from its private key.
static const uint8_t handshake_p256_generator_xy[SECURE_P2P_HANDSHAKE_PUBKEY_LEN] = {
    0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2, //
    0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96, //
    0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16, //
    0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5, //
};

// P-256 group order n, big-endian
static const uint8_t handshake_p256_order[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //
    0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51, //
};

// ########################################################################
// private functions

static secbool_t handshake_ct_equal(const uint8_t* a_p, const uint8_t* b_p, size_t len)
{
    uint8_t diff = 0;
    for ( size_t i = 0; i < len; i++ )
    {
        diff |= (uint8_t)(a_p[i] ^ b_p[i]);
    }
    return (diff == 0) ? sectrue : secfalse;
}

// constant-time 1 <= scalar < n
static secbool_t handshake_scalar_valid(const uint8_t scalar[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN])
{
    uint32_t below = 0;
    uint32_t equal = 1;
    uint32_t nonzero = 0;
    for ( size_t i = 0; i < SECURE_P2P_HANDSHAKE_PRIVKEY_LEN; i++ )
    {
        uint32_t a = scalar[i];
        uint32_t b = handshake_p256_order[i];
        below |= equal & ((a - b) >> 31);
        equal &= ((a ^ b) - 1U) >> 31;
        nonzero |= a;
    }
    return (below == 1 && nonzero != 0) ? sectrue : secfalse;
}

static secbool_t handshake_cfg_valid(const SecureP2p_HandshakeCfg_t* cfg_p)
{
    if ( cfg_p == NULL || cfg_p->cert_p == NULL || cfg_p->ca_id > SECURE_P2P_IDENTITY_CA_ID_MAX ||
         cfg_p->sign == NULL )
    {
        return secfalse;
    }
    if ( cfg_p->cert_len == 0 || cfg_p->cert_len > SECURE_P2P_HANDSHAKE_CERT_MAX_LEN )
    {
        return secfalse;
    }
    return sectrue;
}

static void handshake_wipe(SecureP2p_HandshakeSession_t* ses_p)
{
    crypto_utils_memzero_explicit(ses_p, sizeof(*ses_p));
    ses_p->state = SECURE_P2P_HANDSHAKE_STATE_IDLE;
}

// out_xy = X||Y
static secbool_t handshake_point_mult(
    const uint8_t scalar[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN],
    const uint8_t point_xy[SECURE_P2P_HANDSHAKE_PUBKEY_LEN], uint8_t out_xy[SECURE_P2P_HANDSHAKE_PUBKEY_LEN]
)
{
    size_t out_len = SECURE_P2P_HANDSHAKE_PUBKEY_LEN;
    if ( crypto_keyexchange_ecdh_secp256r1Superfast_compute(
             scalar, SECURE_P2P_HANDSHAKE_PRIVKEY_LEN, point_xy, SECURE_P2P_HANDSHAKE_PUBKEY_LEN, out_xy,
             &out_len
         ) != sectrue )
    {
        return secfalse;
    }
    return (out_len == SECURE_P2P_HANDSHAKE_PUBKEY_LEN) ? sectrue : secfalse;
}

// RFC 5869 HKDF-SHA256 restricted to one expand block (out_len <= 32).
static secbool_t handshake_hkdf(
    const uint8_t* salt_p, size_t salt_len, const uint8_t* ikm_p, size_t ikm_len, const char* info_p,
    size_t info_len, uint8_t* out_p, size_t out_len
)
{
    uint8_t prk[SECURE_P2P_HANDSHAKE_HASH_LEN];
    uint8_t block[SECURE_P2P_HANDSHAKE_HASH_LEN];
    uint8_t msg[HANDSHAKE_INFO_MAX_LEN + 1];
    secbool_t ret = secfalse;

    if ( out_len > SECURE_P2P_HANDSHAKE_HASH_LEN || info_len > HANDSHAKE_INFO_MAX_LEN )
    {
        return secfalse;
    }

    HANDSHAKE_EXEC_TRUE_GOTO(crypto_mac_hmac_sha256_compute(salt_p, salt_len, ikm_p, ikm_len, prk));
    memcpy(msg, info_p, info_len);
    msg[info_len] = 0x01;
    HANDSHAKE_EXEC_TRUE_GOTO(crypto_mac_hmac_sha256_compute(prk, sizeof(prk), msg, info_len + 1, block));
    memcpy(out_p, block, out_len);
    ret = sectrue;

cleanup:
    crypto_utils_memzero_explicit(prk, sizeof(prk));
    crypto_utils_memzero_explicit(block, sizeof(block));
    return ret;
}

// m = SHA-256("onekey-p2p-auth-v1" || "B" || T), the digest the certificate key signs
static secbool_t handshake_auth_digest(
    const SecureP2p_HandshakeSession_t* ses_p, uint8_t digest[SECURE_P2P_HANDSHAKE_HASH_LEN]
)
{
    uint8_t
        msg[HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_AUTH) + HANDSHAKE_ROLE_LABEL_LEN +
            SECURE_P2P_HANDSHAKE_HASH_LEN];
    size_t offset = 0;

    memcpy(msg + offset, HANDSHAKE_LABEL_AUTH, HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_AUTH));
    offset += HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_AUTH);
    memcpy(msg + offset, HANDSHAKE_LABEL_CARD, HANDSHAKE_ROLE_LABEL_LEN);
    offset += HANDSHAKE_ROLE_LABEL_LEN;
    memcpy(msg + offset, ses_p->transcript, sizeof(ses_p->transcript));
    offset += sizeof(ses_p->transcript);
    return crypto_hash_sha256_compute(msg, offset, digest);
}

// tag = HMAC-SHA256(K-CONF, role_label || T)
static secbool_t handshake_conf_tag(
    const SecureP2p_HandshakeSession_t* ses_p, const char* role_label,
    uint8_t tag[SECURE_P2P_HANDSHAKE_TAG_LEN]
)
{
    uint8_t msg[HANDSHAKE_ROLE_LABEL_LEN + SECURE_P2P_HANDSHAKE_HASH_LEN];

    memcpy(msg, role_label, HANDSHAKE_ROLE_LABEL_LEN);
    memcpy(msg + HANDSHAKE_ROLE_LABEL_LEN, ses_p->transcript, sizeof(ses_p->transcript));
    return crypto_mac_hmac_sha256_compute(ses_p->k_conf, sizeof(ses_p->k_conf), msg, sizeof(msg), tag);
}

// ########################################################################
// public functions

void secure_p2p_handshake_init(SecureP2p_HandshakeSession_t* ses_p)
{
    if ( ses_p == NULL )
    {
        return;
    }
    handshake_wipe(ses_p);
}

void secure_p2p_handshake_destroy(SecureP2p_HandshakeSession_t* ses_p)
{
    if ( ses_p == NULL )
    {
        return;
    }
    handshake_wipe(ses_p);
}

secbool_t secure_p2p_handshake_start(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    uint8_t out_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
)
{
    uint8_t priv[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN];
    uint8_t nonce[SECURE_P2P_HANDSHAKE_NONCE_LEN];
    secbool_t scalar_ok = secfalse;
    secbool_t ret = secfalse;

    if ( ses_p == NULL )
    {
        return secfalse;
    }

    // 32 random bytes fall outside [1, n-1] with probability ~2^-32; draw again
    for ( uint32_t attempt = 0; attempt < HANDSHAKE_START_ATTEMPTS && scalar_ok != sectrue; attempt++ )
    {
        HANDSHAKE_EXEC_TRUE_GOTO(crypto_utils_random_generate(priv, sizeof(priv)));
        scalar_ok = handshake_scalar_valid(priv);
    }
    HANDSHAKE_EXEC_TRUE_GOTO(scalar_ok);
    HANDSHAKE_EXEC_TRUE_GOTO(crypto_utils_random_generate(nonce, sizeof(nonce)));
    ret = secure_p2p_handshake_start_with(ses_p, cfg_p, priv, nonce, out_exchange);

cleanup:
    crypto_utils_memzero_explicit(priv, sizeof(priv));
    crypto_utils_memzero_explicit(nonce, sizeof(nonce));
    return ret;
}

secbool_t secure_p2p_handshake_start_with(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const uint8_t priv[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN], const uint8_t nonce[SECURE_P2P_HANDSHAKE_NONCE_LEN],
    uint8_t out_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
)
{
    if ( ses_p == NULL || priv == NULL || nonce == NULL || out_exchange == NULL ||
         handshake_cfg_valid(cfg_p) != sectrue )
    {
        return secfalse;
    }
    if ( ses_p->state != SECURE_P2P_HANDSHAKE_STATE_IDLE || handshake_scalar_valid(priv) != sectrue )
    {
        return secfalse;
    }

    memcpy(ses_p->priv, priv, SECURE_P2P_HANDSHAKE_PRIVKEY_LEN);
    if ( handshake_point_mult(
             ses_p->priv, handshake_p256_generator_xy, ses_p->own_exchange + HANDSHAKE_EXCHANGE_PUB_OFFSET
         ) != sectrue ||
         crypto_hash_sha256_compute(
             cfg_p->cert_p, cfg_p->cert_len, ses_p->own_exchange + HANDSHAKE_EXCHANGE_CERT_HASH_OFFSET
         ) != sectrue )
    {
        handshake_wipe(ses_p);
        return secfalse;
    }
    memcpy(ses_p->own_exchange + HANDSHAKE_EXCHANGE_NONCE_OFFSET, nonce, SECURE_P2P_HANDSHAKE_NONCE_LEN);

    memcpy(out_exchange, ses_p->own_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN);
    ses_p->state = SECURE_P2P_HANDSHAKE_STATE_STARTED;
    return sectrue;
}

secbool_t secure_p2p_handshake_derive(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const uint8_t peer_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
)
{
    uint8_t s[SECURE_P2P_HANDSHAKE_PUBKEY_LEN]; // e * E_peer
    uint8_t t_in[HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_TRANSCRIPT) + 2 * SECURE_P2P_HANDSHAKE_EXCHANGE_LEN];
    size_t offset = 0;
    secbool_t ret = secfalse;

    if ( ses_p == NULL || cfg_p == NULL || peer_exchange == NULL )
    {
        return secfalse;
    }
    if ( ses_p->state != SECURE_P2P_HANDSHAKE_STATE_STARTED )
    {
        return secfalse;
    }

    memcpy(ses_p->peer_exchange, peer_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN);

    // a peer echoing our own EXCHANGE back is not a peer
    if ( handshake_ct_equal(ses_p->peer_exchange, ses_p->own_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN) ==
         sectrue )
    {
        goto cleanup;
    }

    HANDSHAKE_EXEC_TRUE_GOTO(
        secure_p2p_identity_point_validate(ses_p->peer_exchange + HANDSHAKE_EXCHANGE_PUB_OFFSET)
    );
    HANDSHAKE_EXEC_TRUE_GOTO(
        handshake_point_mult(ses_p->priv, ses_p->peer_exchange + HANDSHAKE_EXCHANGE_PUB_OFFSET, s)
    );

    // T = H(label || EXCHANGE_A || EXCHANGE_B)
    memcpy(t_in + offset, HANDSHAKE_LABEL_TRANSCRIPT, HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_TRANSCRIPT));
    offset += HANDSHAKE_STR_LEN(HANDSHAKE_LABEL_TRANSCRIPT);
    memcpy(t_in + offset, ses_p->peer_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN);
    offset += SECURE_P2P_HANDSHAKE_EXCHANGE_LEN;
    memcpy(t_in + offset, ses_p->own_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN);
    offset += SECURE_P2P_HANDSHAKE_EXCHANGE_LEN;
    HANDSHAKE_EXEC_TRUE_GOTO(crypto_hash_sha256_compute(t_in, offset, ses_p->transcript));

    // IKM = S.x, salt = T
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_hkdf(
        ses_p->transcript, sizeof(ses_p->transcript), s, HANDSHAKE_COORD_LEN, HANDSHAKE_INFO_ENC,
        HANDSHAKE_STR_LEN(HANDSHAKE_INFO_ENC), ses_p->s_enc, sizeof(ses_p->s_enc)
    ));
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_hkdf(
        ses_p->transcript, sizeof(ses_p->transcript), s, HANDSHAKE_COORD_LEN, HANDSHAKE_INFO_MAC,
        HANDSHAKE_STR_LEN(HANDSHAKE_INFO_MAC), ses_p->s_mac, sizeof(ses_p->s_mac)
    ));
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_hkdf(
        ses_p->transcript, sizeof(ses_p->transcript), s, HANDSHAKE_COORD_LEN, HANDSHAKE_INFO_RMAC,
        HANDSHAKE_STR_LEN(HANDSHAKE_INFO_RMAC), ses_p->s_rmac, sizeof(ses_p->s_rmac)
    ));
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_hkdf(
        ses_p->transcript, sizeof(ses_p->transcript), s, HANDSHAKE_COORD_LEN, HANDSHAKE_INFO_CONFIRM,
        HANDSHAKE_STR_LEN(HANDSHAKE_INFO_CONFIRM), ses_p->k_conf, sizeof(ses_p->k_conf)
    ));

    ses_p->state = SECURE_P2P_HANDSHAKE_STATE_DERIVED;
    ret = sectrue;

cleanup:
    crypto_utils_memzero_explicit(ses_p->priv, sizeof(ses_p->priv));
    crypto_utils_memzero_explicit(s, sizeof(s));
    if ( ret != sectrue )
    {
        handshake_wipe(ses_p);
    }
    return ret;
}

secbool_t secure_p2p_handshake_commit_build(
    const SecureP2p_HandshakeSession_t* ses_p, uint8_t out_commit[SECURE_P2P_HANDSHAKE_COMMIT_LEN]
)
{
    if ( ses_p == NULL || out_commit == NULL || ses_p->state != SECURE_P2P_HANDSHAKE_STATE_STARTED )
    {
        return secfalse;
    }
    return crypto_hash_sha256_compute(ses_p->own_exchange, SECURE_P2P_HANDSHAKE_EXCHANGE_LEN, out_commit);
}

secbool_t
secure_p2p_handshake_install_card(const SecureP2p_HandshakeSession_t* ses_p, SecureP2p_CardSession_t* card_p)
{
    if ( card_p == NULL )
    {
        return secfalse;
    }
    if ( ses_p == NULL || (ses_p->state != SECURE_P2P_HANDSHAKE_STATE_DERIVED &&
                           ses_p->state != SECURE_P2P_HANDSHAKE_STATE_AUTHED) )
    {
        return secfalse;
    }
    if ( secure_channel_scp03_session_init_keys(
             &card_p->scp03, ses_p->s_enc, ses_p->s_mac, ses_p->s_rmac, SECURE_P2P_HANDSHAKE_SESSION_KEY_LEN,
             NULL, SECURE_P2P_HANDSHAKE_SCP03_SEC_LEVEL
         ) != sectrue )
    {
        return secfalse;
    }
    card_p->state = SECURE_P2P_CARD_STATE_AUTHED;
    return sectrue;
}

secbool_t secure_p2p_handshake_auth_build(
    const SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    uint8_t out_auth[SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN], uint32_t* out_len_p
)
{
    uint8_t digest[SECURE_P2P_HANDSHAKE_HASH_LEN];
    uint8_t* sig_p = NULL;
    secbool_t ret = secfalse;

    if ( ses_p == NULL || out_auth == NULL || out_len_p == NULL || handshake_cfg_valid(cfg_p) != sectrue )
    {
        return secfalse;
    }
    // the card identifies itself only after it verified the host
    if ( ses_p->state != SECURE_P2P_HANDSHAKE_STATE_AUTHED )
    {
        return secfalse;
    }

    memcpy(out_auth, cfg_p->cert_p, cfg_p->cert_len);
    sig_p = out_auth + cfg_p->cert_len;
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_auth_digest(ses_p, digest));
    HANDSHAKE_EXEC_TRUE_GOTO(cfg_p->sign(cfg_p->sign_ctx_p, digest, sig_p));
    HANDSHAKE_EXEC_TRUE_GOTO(
        handshake_conf_tag(ses_p, HANDSHAKE_LABEL_CARD, sig_p + SECURE_P2P_HANDSHAKE_SIG_LEN)
    );
    *out_len_p = cfg_p->cert_len + HANDSHAKE_AUTH_TRAILER_LEN;
    ret = sectrue;

cleanup:
    crypto_utils_memzero_explicit(digest, sizeof(digest));
    if ( ret != sectrue )
    {
        crypto_utils_memzero_explicit(out_auth, SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN);
        *out_len_p = 0;
    }
    return ret;
}

secbool_t secure_p2p_handshake_auth_verify(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p, const uint8_t* auth_p,
    uint32_t auth_len
)
{
    uint8_t cert_hash[SECURE_P2P_HANDSHAKE_HASH_LEN];
    uint8_t expected[SECURE_P2P_HANDSHAKE_TAG_LEN];
    uint32_t cert_len = 0;
    const uint8_t* sig_p = NULL;
    secbool_t ret = secfalse;

    if ( ses_p == NULL || auth_p == NULL || handshake_cfg_valid(cfg_p) != sectrue )
    {
        return secfalse;
    }
    if ( ses_p->state != SECURE_P2P_HANDSHAKE_STATE_DERIVED )
    {
        return secfalse;
    }

    HANDSHAKE_EXEC_TRUE_GOTO(
        (auth_len > HANDSHAKE_AUTH_TRAILER_LEN && auth_len <= SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN) ? sectrue
                                                                                                 : secfalse
    );
    cert_len = auth_len - HANDSHAKE_AUTH_TRAILER_LEN;
    sig_p = auth_p + cert_len;

    // 1. the certificate is the one hashed into the peer EXCHANGE
    HANDSHAKE_EXEC_TRUE_GOTO(crypto_hash_sha256_compute(auth_p, cert_len, cert_hash));
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_ct_equal(
        cert_hash, ses_p->peer_exchange + HANDSHAKE_EXCHANGE_CERT_HASH_OFFSET, sizeof(cert_hash)
    ));
    // 2. the peer holds the same session keys
    HANDSHAKE_EXEC_TRUE_GOTO(handshake_conf_tag(ses_p, HANDSHAKE_LABEL_HOST, expected));
    HANDSHAKE_EXEC_TRUE_GOTO(
        handshake_ct_equal(expected, sig_p + SECURE_P2P_HANDSHAKE_SIG_LEN, SECURE_P2P_HANDSHAKE_TAG_LEN)
    );

    ses_p->state = SECURE_P2P_HANDSHAKE_STATE_AUTHED;
    ret = sectrue;

cleanup:
    crypto_utils_memzero_explicit(expected, sizeof(expected));
    if ( ret != sectrue )
    {
        handshake_wipe(ses_p);
    }
    return ret;
}

secbool_t secure_p2p_handshake_peer_proof(
    const SecureP2p_HandshakeSession_t* ses_p, const uint8_t* peer_auth_p, uint32_t peer_auth_len,
    uint8_t out[SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN], uint32_t* out_len_p
)
{
    uint8_t cert_hash[SECURE_P2P_HANDSHAKE_HASH_LEN];

    if ( out_len_p != NULL )
    {
        *out_len_p = 0;
    }
    if ( ses_p == NULL || peer_auth_p == NULL || out == NULL || out_len_p == NULL ||
         ses_p->state != SECURE_P2P_HANDSHAKE_STATE_AUTHED ||
         ses_p->peer_ca_id > SECURE_P2P_IDENTITY_CA_ID_MAX || peer_auth_len <= HANDSHAKE_AUTH_TRAILER_LEN ||
         peer_auth_len > SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN )
    {
        return secfalse;
    }
    // only the certificate hashed into the peer EXCHANGE may reach the SE
    if ( crypto_hash_sha256_compute(peer_auth_p, peer_auth_len - HANDSHAKE_AUTH_TRAILER_LEN, cert_hash) !=
             sectrue ||
         handshake_ct_equal(
             cert_hash, ses_p->peer_exchange + HANDSHAKE_EXCHANGE_CERT_HASH_OFFSET, sizeof(cert_hash)
         ) != sectrue )
    {
        crypto_utils_memzero_explicit(out, SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN);
        return secfalse;
    }
    uint32_t offset = 1 + peer_auth_len - SECURE_P2P_HANDSHAKE_TAG_LEN;
    out[0] = ses_p->peer_ca_id;
    memcpy(out + 1, peer_auth_p, offset - 1);
    out[offset++] = (uint8_t)HANDSHAKE_LABEL_HOST[0];
    memcpy(out + offset, ses_p->transcript, SECURE_P2P_HANDSHAKE_HASH_LEN);
    offset += SECURE_P2P_HANDSHAKE_HASH_LEN;
    memcpy(
        out + offset, ses_p->peer_exchange + HANDSHAKE_EXCHANGE_CERT_HASH_OFFSET,
        SECURE_P2P_HANDSHAKE_HASH_LEN
    );
    *out_len_p = offset + SECURE_P2P_HANDSHAKE_HASH_LEN;
    return sectrue;
}
