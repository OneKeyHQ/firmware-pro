#pragma once

#include <stdint.h>

#include "../secure_channel_port.h"

// ########################################################################
// defines

#define SECURE_CHANNEL_SCP03_MAC_LEN      8
#define SECURE_CHANNEL_SCP03_BLOCK_LEN    16
#define SECURE_CHANNEL_SCP03_MAX_KEY_LEN  32
#define SECURE_CHANNEL_SCP03_MAX_DATA_LEN 240

#define SECURE_CHANNEL_SCP03_SEC_CMAC     0x01
#define SECURE_CHANNEL_SCP03_SEC_CDEC     0x02
#define SECURE_CHANNEL_SCP03_SEC_RMAC     0x10
#define SECURE_CHANNEL_SCP03_SEC_RENC     0x20

typedef struct
{
    uint8_t s_enc[SECURE_CHANNEL_SCP03_MAX_KEY_LEN];
    uint8_t s_mac[SECURE_CHANNEL_SCP03_MAX_KEY_LEN];
    uint8_t s_rmac[SECURE_CHANNEL_SCP03_MAX_KEY_LEN];
    uint8_t mac_chaining[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    uint8_t enc_counter[SECURE_CHANNEL_SCP03_BLOCK_LEN];
    uint32_t key_len;
    uint8_t security_level;
} SecureChannel_Scp03Session_t;

// ########################################################################
// public functions

// ########## key derivation ##########

// Install pre-derived session keys and security level on the session.
secbool_t __wur secure_channel_scp03_session_init_keys(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* s_enc_p, const uint8_t* s_mac_p,
    const uint8_t* s_rmac_p, uint32_t key_len, const uint8_t mac_chaining[SECURE_CHANNEL_SCP03_BLOCK_LEN],
    uint8_t security_level
);

// ########## session lifecycle ##########

void secure_channel_scp03_destroy(SecureChannel_Scp03Session_t* session_p);

// ########## command encryption ##########

secbool_t __wur secure_channel_scp03_encrypt(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
);

secbool_t __wur secure_channel_scp03_decrypt(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
);

// ########## response encryption ##########

secbool_t __wur secure_channel_scp03_encrypt_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
);

secbool_t __wur secure_channel_scp03_decrypt_response(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* in_p, uint32_t in_len, uint8_t* out_p,
    uint32_t* out_len_p
);

// ########## C-MAC ##########

secbool_t __wur secure_channel_scp03_mac(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t mac[8]
);

secbool_t __wur secure_channel_scp03_mac_verify(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, const uint8_t mac[8]
);

// ########## R-MAC ##########

secbool_t __wur secure_channel_scp03_rmac(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, uint8_t mac[8]
);

secbool_t __wur secure_channel_scp03_rmac_verify(
    SecureChannel_Scp03Session_t* session_p, const uint8_t* data_p, uint32_t data_len, const uint8_t mac[8]
);
