#pragma once

#include <stddef.h>
#include <stdint.h>

#include <secbool.h>
#include <secure_p2p_card.h>
#include <secure_p2p_identity.h>
#include <util_macros.h>

// ########################################################################
// defines

// Ephemeral P-256 key agreement for SCP03 plus SE certificate authentication.

#define SECURE_P2P_HANDSHAKE_PRIVKEY_LEN 32
#define SECURE_P2P_HANDSHAKE_PUBKEY_LEN  64
#define SECURE_P2P_HANDSHAKE_NONCE_LEN   16
#define SECURE_P2P_HANDSHAKE_HASH_LEN    32 // SHA-256 transcript / certificate hash / HKDF output
#define SECURE_P2P_HANDSHAKE_EXCHANGE_LEN \
  (SECURE_P2P_HANDSHAKE_PUBKEY_LEN + SECURE_P2P_HANDSHAKE_NONCE_LEN + SECURE_P2P_HANDSHAKE_HASH_LEN)
#define SECURE_P2P_HANDSHAKE_COMMIT_LEN   32 // SHA-256 of the card's EXCHANGE
#define SECURE_P2P_HANDSHAKE_TAG_LEN      32 // HMAC-SHA256 key-confirm tag
#define SECURE_P2P_HANDSHAKE_SIG_LEN      SECURE_P2P_IDENTITY_SIG_LEN
#define SECURE_P2P_HANDSHAKE_CERT_MAX_LEN SECURE_P2P_IDENTITY_CERT_MAX_LEN
#define SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN \
  (SECURE_P2P_HANDSHAKE_CERT_MAX_LEN + SECURE_P2P_HANDSHAKE_SIG_LEN + SECURE_P2P_HANDSHAKE_TAG_LEN) // 608
#define SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN                               \
  (1 + SECURE_P2P_HANDSHAKE_CERT_MAX_LEN + SECURE_P2P_HANDSHAKE_SIG_LEN + 1 + \
   2 * SECURE_P2P_HANDSHAKE_HASH_LEN)
#define SECURE_P2P_HANDSHAKE_SESSION_KEY_LEN 16 // AES-128 S-ENC / S-MAC / S-RMAC
#define SECURE_P2P_HANDSHAKE_SCP03_SEC_LEVEL                                                       \
  (SECURE_CHANNEL_SCP03_SEC_CMAC | SECURE_CHANNEL_SCP03_SEC_CDEC | SECURE_CHANNEL_SCP03_SEC_RMAC | \
   SECURE_CHANNEL_SCP03_SEC_RENC) // 0x33

typedef enum
{
    SECURE_P2P_HANDSHAKE_STATE_IDLE = 0,
    SECURE_P2P_HANDSHAKE_STATE_STARTED, // own EXCHANGE built, ephemeral private key held
    SECURE_P2P_HANDSHAKE_STATE_DERIVED, // session keys derived, private key wiped
    SECURE_P2P_HANDSHAKE_STATE_AUTHED,  // peer AUTH verified
} SecureP2p_HandshakeState_t;

// ECDSA-P256 with the SE certificate key (00 F5 00 04): the SE must sign
// exactly `digest`, without hashing it again. out_sig = r||s.
typedef secbool_t (*SecureP2p_HandshakeSign_func_t
)(void* ctx_p, const uint8_t digest[SECURE_P2P_HANDSHAKE_HASH_LEN],
  uint8_t out_sig[SECURE_P2P_HANDSHAKE_SIG_LEN]);

// Per-device configuration. Pointers must live as long as the session.
typedef struct
{
    const uint8_t*
        cert_p; // own device certificate (00 F5 00 02), DER, 1..SECURE_P2P_HANDSHAKE_CERT_MAX_LEN bytes
    uint32_t cert_len;
    uint8_t ca_id; // own certificate issuer, SECURE_P2P_IDENTITY_CA_ID_*
    SecureP2p_HandshakeSign_func_t sign;
    void* sign_ctx_p;
} SecureP2p_HandshakeCfg_t;

typedef struct
{
    SecureP2p_HandshakeState_t state;
    uint8_t peer_ca_id; // peer certificate issuer from SELECT
    uint8_t priv[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN];
    uint8_t own_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN];
    uint8_t peer_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN];
    uint8_t transcript[SECURE_P2P_HANDSHAKE_HASH_LEN];
    uint8_t s_enc[SECURE_P2P_HANDSHAKE_SESSION_KEY_LEN];
    uint8_t s_mac[SECURE_P2P_HANDSHAKE_SESSION_KEY_LEN];
    uint8_t s_rmac[SECURE_P2P_HANDSHAKE_SESSION_KEY_LEN];
    uint8_t k_conf[SECURE_P2P_HANDSHAKE_HASH_LEN];
} SecureP2p_HandshakeSession_t;

// ########################################################################
// public functions

void secure_p2p_handshake_init(SecureP2p_HandshakeSession_t* ses_p);
void secure_p2p_handshake_destroy(SecureP2p_HandshakeSession_t* ses_p);

// Generate the ephemeral key pair + nonce, hash the own certificate
// and build the own EXCHANGE message. IDLE -> STARTED.
secbool_t __wur secure_p2p_handshake_start(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    uint8_t out_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
);

// Deterministic core of `start` with caller-supplied private key
// (1 <= priv < n) and nonce.
secbool_t __wur secure_p2p_handshake_start_with(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const uint8_t priv[SECURE_P2P_HANDSHAKE_PRIVKEY_LEN], const uint8_t nonce[SECURE_P2P_HANDSHAKE_NONCE_LEN],
    uint8_t out_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
);

// The card commits to its EXCHANGE before it sees EXCHANGE_A. Needs STARTED.
secbool_t __wur secure_p2p_handshake_commit_build(
    const SecureP2p_HandshakeSession_t* ses_p, uint8_t out_commit[SECURE_P2P_HANDSHAKE_COMMIT_LEN]
);

// cfg_p must be the one given to `start`. STARTED -> DERIVED; wipes the
// ephemeral private key.
secbool_t __wur secure_p2p_handshake_derive(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    const uint8_t peer_exchange[SECURE_P2P_HANDSHAKE_EXCHANGE_LEN]
);

// Requires DERIVED or AUTHED; the card session moves to
// AUTHED. The channel is up before the peer is verified: the caller must gate
// everything but AUTH until auth_verify passes and the SE accepts the peer proof (84 F5 00 0A).
secbool_t __wur
secure_p2p_handshake_install_card(const SecureP2p_HandshakeSession_t* ses_p, SecureP2p_CardSession_t* card_p);

// Builds only in AUTHED, after the card verified the host.
secbool_t __wur secure_p2p_handshake_auth_build(
    const SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p,
    uint8_t out_auth[SECURE_P2P_HANDSHAKE_AUTH_MAX_LEN], uint32_t* out_len_p
);

// Verify the peer AUTH in order: SHA-256(C) equals the hash in the
// peer EXCHANGE and tag matches (constant time); the SE verifies C and sigma.
// Needs DERIVED; DERIVED -> AUTHED. An authentication failure wipes the session.
secbool_t __wur secure_p2p_handshake_auth_verify(
    SecureP2p_HandshakeSession_t* ses_p, const SecureP2p_HandshakeCfg_t* cfg_p, const uint8_t* auth_p,
    uint32_t auth_len
);

// F5/0A data from the verified peer AUTH: ca_id || C || sigma || role || T || H(C).
// Requires AUTHED and the same peer AUTH that passed auth_verify.
secbool_t __wur secure_p2p_handshake_peer_proof(
    const SecureP2p_HandshakeSession_t* ses_p, const uint8_t* peer_auth_p, uint32_t peer_auth_len,
    uint8_t out[SECURE_P2P_HANDSHAKE_PEER_PROOF_MAX_LEN], uint32_t* out_len_p
);
