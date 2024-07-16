#ifndef _SCP_11_H_
#define _SCP_11_H_

#include "tag.h"
#include "tlv.h"

#define SCP11_MAX_CERTIFICATE_SIZE 256
#define SPC11_SESSION_KEY_SIZE 16

typedef struct {
  TLV_FIELD(GPC_TLV_SCP11CRT_ENTITY);
  TLV_FIELD(GPC_TLV_SCP11CRT_SN);
  TLV_FIELD(GPC_TLV_SCP11CRT_CAKLOCID);
  TLV_FIELD(GPC_TLV_SCP11CRT_SUBJECTID);
  TLV_FIELD(GPC_TLV_SCP11CRT_KEYUSAGE);
  TLV_FIELD(GPC_TLV_SCP11CRT_EFFEDATE);
  TLV_FIELD(GPC_TLV_SCP11CRT_EXPEDATE);
  TLV_FIELD(GPC_TLV_SCP11CRT_DISC_53);
  TLV_FIELD(GPC_TLV_SCP11CRT_DISC_73);
  TLV_FIELD(GPC_TLV_SCP11CRT_BF_RESTR);
  TLV_FIELD(GPC_TLV_SCP11CRT_PUBKEY);
  TLV_FIELD(GPC_TLV_SCP11CRT_SIGNATURE);

  uint8_t raw[SCP11_MAX_CERTIFICATE_SIZE];
  uint16_t raw_len;
} scp11_certificate;

// typedef struct {
//   TLV_VAR(GPC_TLV_SHAREDINFO_SCP_ID_PARAM);
//   TLV_VAR(GPC_TLV_SHAREDINFO_KEYUSAGE);
//   TLV_VAR(GPC_TLV_SHAREDINFO_KEYTYPE);
//   TLV_VAR(GPC_TLV_SHAREDINFO_KEYLENGTH);
//   TLV_VAR(GPC_TLV_SHAREDINFO_HOSTID);
// } scp11_shared_info;

// typedef struct {
//   uint8_t scp_id_param[2];
//   uint8_t key_usage;
//   uint8_t key_type;
//   uint8_t key_length;
//   uint8_t host_id[8];
// } scp11_shared_info_data;

typedef struct {
  uint8_t len;
  uint8_t *value;
} scp11_shared_info;

typedef struct {
  uint8_t key_dek[SPC11_SESSION_KEY_SIZE];
  uint8_t s_enc[SPC11_SESSION_KEY_SIZE];
  uint8_t s_mac[SPC11_SESSION_KEY_SIZE];
  uint8_t s_rmac[SPC11_SESSION_KEY_SIZE];
  uint8_t s_dek[SPC11_SESSION_KEY_SIZE];
} scp11_session_key;

typedef struct {
  uint8_t oce_private_key[32];
  uint8_t oce_public_key[65];
  uint8_t oce_temp_private_key[32];
  uint8_t oce_temp_public_key[65];
  uint8_t sd_public_key[65];
} scp11_mutual_auth;

typedef struct {
  TLV_FIELD(GPC_TLV_MA_PK);
  TLV_FIELD(GPC_TLV_MA_RECEIPT);
  uint8_t data[128];
  uint16_t data_len;
} scp11_response_msg;

typedef struct {
  bool is_secure_channel_opened;
  scp11_certificate sd_cert;
  scp11_certificate oce_cert;
  scp11_mutual_auth mutual_auth;
  scp11_shared_info shared_info;
  scp11_response_msg response_msg;
  scp11_session_key session_key;
} scp11_context;

bool scp11_certificate_parse_and_verify(uint8_t *cert_raw, uint16_t cert_len,
                                        scp11_certificate *cert);
bool scp11_get_mutual_auth_data(uint8_t *data, uint8_t *data_len,
                                scp11_context scp11_ctx);
bool scp11_open_secure_channel(scp11_context *scp11_ctx);
void scp11_close_secure_channel(scp11_context *scp11_ctx);
bool scp11_get_secure_channel_status(scp11_context *scp11_ctx);
void scp11_init(scp11_context *scp11_ctx);

#endif
