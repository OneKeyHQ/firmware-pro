#ifndef _SE_THD89_H_
#define _SE_THD89_H_

#include "bip32.h"
#include "secbool.h"
#include "thd89.h"

#define SESSION_KEYLEN (16)

#define PUBLIC_REGION_SIZE (0x800)   // 2KB
#define PRIVATE_REGION_SIZE (0x800)  // 2KB

#define MAX_AUTHORIZATION_LEN 128

#define PIN_MAX_LENGTH (50)
#define PASSPHRASE_MAX_LENGTH (50)

#define SESSION_TYPE_NORMAL 0
#define SESSION_TYPE_PASSPHRASE 1

typedef enum {
  PIN_SUCCESS,
  USER_PIN_ENTERED,
  USER_PIN_FAILED,
  PASSPHRASE_PIN_ENTERED,
  PASSPHRASE_PIN_NO_MATCHED,
  USER_PIN_NOT_ENTERED,
  WIPE_CODE_ENTERED,
  PIN_SAME_AS_USER_PIN,
  PIN_SAME_AS_WIPE_CODE,
  PIN_PASSPHRASE_MAX_ITEMS_REACHED,
  PIN_PASSPHRASE_SAVE_FAILED,
  PIN_PASSPHRASE_READ_FAILED,
  PIN_FAILED
} pin_result_t;

typedef enum {
  PIN_TYPE_USER,
  PIN_TYPE_USER_CHECK,
  PIN_TYPE_USER_AND_PASSPHRASE_PIN,
  PIN_TYPE_PASSPHRASE_PIN,
  PIN_TYPE_PASSPHRASE_PIN_CHECK,
  PIN_TYPE_MAX
} pin_type_t;

#define FIDO2_RESIDENT_CREDENTIALS_SIZE (512)
#define FIDO2_RESIDENT_CREDENTIALS_COUNT (60)
#define FIDO2_RESIDENT_CREDENTIALS_FLAGS "\x66\x69\x64\x6F"  // "fido"
#define FIDO2_RESIDENT_CREDENTIALS_HEADER_LEN (6)
typedef struct {
  uint8_t credential_id_flag[4];
  uint16_t credential_length;
  uint8_t rp_id_hash[32];
  uint8_t credential_id[474];
} __attribute__((packed)) CTAP_credential_id_storage;
_Static_assert(sizeof(CTAP_credential_id_storage) ==
                   FIDO2_RESIDENT_CREDENTIALS_SIZE,
               "CTAP_credential_id_storage size must be flash page size");

typedef secbool (*UI_WAIT_CALLBACK)(uint32_t wait, uint32_t progress,
                                    const char *message);
void se_set_ui_callback(UI_WAIT_CALLBACK callback);

secbool se_transmit_mac(uint8_t ins, uint8_t p1, uint8_t p2, uint8_t *data,
                        uint16_t data_len, uint8_t *recv, uint16_t *recv_len);

secbool se_get_rand(uint8_t *rand, uint16_t rand_len);
secbool se_reset_se(void);
secbool se_random_encrypted(uint8_t *rand, uint16_t len);
secbool se_random_encrypted_ex(uint8_t addr, uint8_t *session_key,
                               uint8_t *rand, uint16_t len);
secbool se_sync_session_key(void);
secbool se_device_init(uint8_t mode, const char *passphrase);
secbool se_ecdsa_get_pubkey(uint32_t *address, uint8_t count, uint8_t *pubkey);

secbool se_reset_storage(void);
secbool se_set_sn(const char *serial, uint8_t len);
secbool se_get_sn(char **serial);
int se_get_version(uint8_t addr, char *ver, uint16_t in_len);
int se_get_build_id(uint8_t addr, char *build_id, uint16_t in_len);
int se_get_hash(uint8_t addr, uint8_t *hash, uint16_t in_len);
int se_get_boot_version(uint8_t addr, char *ver, uint16_t in_len);
int se_get_boot_build_id(uint8_t addr, char *build_id, uint16_t in_len);
int se_get_boot_hash(uint8_t addr, uint8_t *hash, uint16_t in_len);
char *se01_get_version(void);
char *se01_get_build_id(void);
uint8_t *se01_get_hash(void);
char *se01_get_boot_version(void);
char *se01_get_boot_build_id(void);
uint8_t *se01_get_boot_hash(void);
char *se02_get_version(void);
char *se02_get_build_id(void);
uint8_t *se02_get_hash(void);
char *se02_get_boot_version(void);
char *se02_get_boot_build_id(void);
uint8_t *se02_get_boot_hash(void);
char *se03_get_version(void);
char *se03_get_build_id(void);
uint8_t *se03_get_hash(void);
char *se03_get_boot_version(void);
char *se03_get_boot_build_id(void);
uint8_t *se03_get_boot_hash(void);
char *se04_get_version(void);
char *se04_get_build_id(void);
uint8_t *se04_get_hash(void);
char *se04_get_boot_version(void);
char *se04_get_boot_build_id(void);
uint8_t *se04_get_boot_hash(void);
secbool se_isInitialized(void);
secbool se_hasPin(void);
secbool se_setPin(const char *pin);
secbool se_verifyPin(const char *pin, pin_type_t pin_type);
secbool se_changePin(const char *oldpin, const char *newpin);
uint32_t se_pinFailedCounter(void);
secbool se_getRetryTimes(uint8_t *ptimes);
pin_result_t se_get_pin_result_type(void);
secbool se_set_pin_passphrase(const char *pin, const char *passphrase_pin,
                              const char *passphrase, bool *override);
secbool se_delete_pin_passphrase(const char *passphrase_pin, bool *current);
pin_result_t se_get_pin_passphrase_ret(void);
secbool se_get_pin_passphrase_space(uint8_t *space);
secbool se_check_passphrase_btc_test_address(const char *address);
secbool se_clearSecsta(void);
secbool se_getSecsta(void);
secbool se_set_u2f_counter(uint32_t u2fcounter);
secbool se_get_u2f_counter(uint32_t *u2fcounter);
secbool se_set_mnemonic(const char *mnemonic, uint16_t len);
secbool se_import_slip39(const uint8_t *master_secret, uint8_t len,
                         uint8_t backup_type, uint16_t identifier,
                         uint8_t iteration_exponent);
secbool se_sessionStart(uint8_t *session_id_bytes);
secbool se_sessionOpen(uint8_t *session_id_bytes);

secbool se_get_session_seed_state(uint8_t *state);
secbool se_session_is_open(void);

secbool se_sessionClose(void);
secbool se_sessionClear(void);
secbool se_session_get_type(uint8_t *type);

secbool se_set_public_region(uint16_t offset, const void *val_dest,
                             uint16_t len);
secbool se_get_public_region(uint16_t offset, void *val_dest, uint16_t len);
secbool se_set_private_region(uint16_t offset, const void *val_dest,
                              uint16_t len);
secbool se_get_private_region(uint16_t offset, void *val_dest, uint16_t len);

secbool se_get_pubkey(uint8_t *pubkey);
secbool se_get_ecdh_pubkey(uint8_t addr, uint8_t *key);
secbool se_lock_ecdh_pubkey(uint8_t addr);
secbool se_write_certificate(const uint8_t *cert, uint16_t cert_len);
secbool se_read_certificate(uint8_t *cert, uint16_t *cert_len);
secbool se_has_cerrificate(void);
secbool se_sign_message(uint8_t *msg, uint32_t msg_len, uint8_t *signature);
secbool se_sign_message_with_write_key(uint8_t *msg, uint32_t msg_len,
                                       uint8_t *signature);
secbool se_set_private_key_extern(uint8_t key[32]);
secbool se_set_session_key_ex(uint8_t addr, const uint8_t *session_key);
secbool se_set_session_key(const uint8_t *session_key);

secbool se_containsMnemonic(const char *mnemonic);
secbool se_exportMnemonic(char *mnemonic, uint16_t dest_size);
secbool se_set_needs_backup(bool needs_backup);
secbool se_get_needs_backup(bool *needs_backup);
secbool se_hasWipeCode(void);
secbool se_changeWipeCode(const char *pin, const char *wipe_code);

uint8_t *se_session_startSession(const uint8_t *received_session_id);
secbool se_gen_session_seed(const char *passphrase, bool cardano);
secbool se_derive_keys(HDNode *out, const char *curve,
                       const uint32_t *address_n, size_t address_n_count,
                       uint32_t *fingerprint);
secbool se_derive_xmr_key(const char *curve, const uint32_t *address_n,
                          size_t address_n_count, uint8_t *pubkey,
                          uint8_t *prikey_hash);
secbool se_derive_xmr_private_key(const uint8_t *pubkey, const uint32_t index,
                                  uint8_t *prikey);
secbool se_xmr_get_tx_key(const uint8_t *rand, const uint8_t *hash,
                          uint8_t *out);
secbool se_node_sign_digest(const uint8_t *hash, uint8_t *sig, uint8_t *by);
int se_ecdsa_sign_digest(const uint8_t curve, const uint8_t canonical,
                         const uint8_t *digest, uint8_t *sig, uint8_t *pby);

int se_secp256k1_sign_digest(const uint8_t canonical, const uint8_t *digest,
                             uint8_t *sig, uint8_t *pby);
int se_nist256p1_sign_digest(const uint8_t *digest, uint8_t *sig, uint8_t *pby);

int se_ed25519_sign(const uint8_t *msg, uint16_t msg_len, uint8_t *sig);
int se_ed25519_sign_ext(const uint8_t *msg, uint16_t msg_len, uint8_t *sig);
int se_ed25519_sign_keccak(const uint8_t *msg, uint16_t msg_len, uint8_t *sig);

int se_get_shared_key(const char *curve, const uint8_t *peer_public_key,
                      uint8_t *session_key);

secbool se_derive_tweak_private_keys(const uint8_t *root_hash);
int se_bip340_sign_digest(const uint8_t *digest, uint8_t sig[64]);
int se_bch_schnorr_sign_digest(const uint8_t *digest, uint8_t sig[64]);

int se_aes256_encrypt(const uint8_t *data, uint16_t data_len, const uint8_t *iv,
                      uint8_t *value, uint16_t value_len, uint8_t *out);
int se_aes256_decrypt(const uint8_t *data, uint16_t data_len, const uint8_t *iv,
                      uint8_t *value, uint16_t value_len, uint8_t *out);

int se_nem_aes256_encrypt(const uint8_t *ed25519_public_key, const uint8_t *iv,
                          const uint8_t *salt, uint8_t *payload, uint16_t size,
                          uint8_t *out);
int se_nem_aes256_decrypt(const uint8_t *ed25519_public_key, const uint8_t *iv,
                          const uint8_t *salt, uint8_t *payload, uint16_t size,
                          uint8_t *out);
int se_slip21_node(uint8_t *data);
int se_slip21_fido_node(uint8_t *data);

secbool se_authorization_set(const uint32_t authorization_type,
                             const uint8_t *authorization,
                             uint32_t authorization_len);
secbool se_authorization_get_type(uint32_t *authorization_type);
secbool se_authorization_get_data(uint8_t *authorization_data,
                                  uint32_t *authorization_len);
void se_authorization_clear(void);

secbool se_fingerprint_state(void);
secbool se_fingerprint_lock(void);
secbool se_fingerprint_unlock(void);

secbool se_fp_write(uint32_t offset, const void *val_dest, uint32_t len,
                    uint8_t index, uint8_t total);
secbool se_fp_read(uint32_t offset, void *val_dest, uint32_t len, uint8_t index,
                   uint8_t total);

int se_lite_card_ecdh(const uint8_t *publickey, uint8_t *sessionkey);

secbool se_gen_fido_seed(uint8_t *percent);
secbool se_u2f_register(const uint8_t app_id[32], const uint8_t challenge[32],
                        uint8_t key_handle[64], uint8_t pub_key[65],
                        uint8_t sign[64]);
secbool se_u2f_gen_handle_and_node(const uint8_t app_id[32],
                                   uint8_t key_handle[64], HDNode *out);
secbool se_u2f_validate_handle(const uint8_t app_id[32],
                               const uint8_t key_handle[64]);
secbool se_u2f_authenticate(const uint8_t app_id[32],
                            const uint8_t key_handle[64],
                            const uint8_t challenge[32], uint8_t *u2f_counter,
                            uint8_t sign[64]);
secbool se_derive_fido_keys(HDNode *out, const char *curve,
                            const uint32_t *address_n, size_t address_n_count,
                            uint32_t *fingerprint);
secbool se_fido_hdnode_sign_digest(const uint8_t *hash, uint8_t *sig);
secbool se_fido_att_sign_digest(const uint8_t *hash, uint8_t *sig);
secbool se_get_fido2_resident_credentials(uint32_t index, uint8_t *dest,
                                          uint16_t *dst_len);
secbool se_set_fido2_resident_credentials(uint32_t index, const uint8_t *src,
                                          uint16_t len);
secbool se_delete_fido2_resident_credentials(uint32_t index);
secbool se_delete_all_fido2_credentials(void);

secbool session_generate_seed_percent(uint8_t *percent);
#endif
