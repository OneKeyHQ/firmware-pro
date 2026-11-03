
#include <stdio.h>
#include <string.h>

#include "cmac.h"
#include "ecdsa.h"
#include "memzero.h"
#include "nist256p1.h"
#include "rand.h"
#include "scp11.h"
#include "sha2.h"

#include "manage_card.h"
#include "se_thd89.h"

// ########################################################################
// OCE private keys
//
// The SCP11 handshake needs a static ECDH between the card's SD public key and
// the private key belonging to our own OCE certificate. For both card
// generations that key lives in the SE and never leaves it: the SE runs the
// ECDH for us (se_lite_card_ecdh / se_seed_card_ecdh). The SE carries a QA or a
// PROD Seed Card key depending on its own build, so the certificate picked
// below for this build must be the one issued for that key - a mismatch shows
// up as a receipt failure at the end of the handshake, never earlier.

// ########################################################################
// per-card credentials
//
// root_pubkey_*  : CA key that signed both the card's SD certificate and our
//                  own OCE certificate for that card generation.
// device_certificate_* : our OCE certificate, sent to the card during the
//                  handshake. Its public key must be the one belonging to the
//                  card-reader key held by the SE.

const static uint8_t root_pubkey_lite_card[SCP11_P256_PUBKEY_SIZE] = {
    "\x04\x21\x46\xE7\x94\x1C\x2E\xBE\xBD\xC4\x7A\x1B\xFA\x52\x9A\x81\x5F\x2C"
    "\x3C\x55\x75\x78\x62\xC3\x78\x23\x60\x21\xD4\x99\xC3\x2E\xD5\x2F\x93\x54"
    "\xB7\x0D\x81\x38\xAD\x52\x74\x76\xB7\x26\x21\x2B\x97\xD6\x78\x77\xD5\x5F"
    "\x45\x9C\xB7\xE2\xD5\xF8\x5E\xF9\xD7\x93\x02"};

const static uint8_t device_certificate_lite_card[] = {
    "\x7F\x21\x81\xDB\x93\x10\x43\x45\x52\x54\x5F\x4F\x43\x45\x5F\x45\x43\x4B"
    "\x41\x30\x30\x31\x42\x0D\x6A\x75\x62\x69\x74\x65\x72\x77\x61\x6C\x6C\x65"
    "\x74\x5F\x20\x0D\x6A\x75\x62\x69\x74\x65\x72\x77\x61\x6C\x6C\x65\x74\x95"
    "\x02\x00\x80\x5F\x25\x04\x20\x20\x05\x25\x5F\x24\x04\x20\x25\x05\x24\x53"
    "\x00\xBF\x20\x00\x7F\x49\x46\xB0\x41\x04\x08\xCC\xB4\x9E\xB9\x10\x57\x28"
    "\x75\x72\xE6\x87\x06\xF3\xCB\x4C\x27\xCE\x19\xAD\x94\xC4\x0B\x2A\x37\xC5"
    "\x94\xE5\x1B\xC0\x9E\xAD\x96\x34\x94\x66\x30\x6C\x58\x63\xF6\xE8\xBE\xB3"
    "\xF0\xEA\x99\x71\x18\x48\x16\x32\x01\xBF\xE8\xC7\x88\x43\x3D\x45\x81\x64"
    "\x69\xE5\xF0\x01\x00\x5F\x37\x47\x30\x45\x02\x21\x00\x87\x9E\xEB\x7E\xE0"
    "\x96\x2B\x44\xBD\x3D\x87\x01\x16\x1A\x26\x34\x77\xCC\x2F\x08\xD7\x68\x1A"
    "\xF8\x54\x6F\xBC\x17\xEB\x3E\x99\x65\x02\x20\x16\x00\xFA\x7A\x74\x1B\x0E"
    "\xFE\x7C\x14\x3D\x73\x71\x3E\x80\x31\xAF\xBB\x3F\x1C\x0B\x6D\x69\x04\x80"
    "\x20\xD2\x73\xE4\x8A\xAF\x5E"};

#if PRODUCTION
const static uint8_t root_pubkey_seed_card[SCP11_P256_PUBKEY_SIZE] = {
    "\x04\x04\x93\x29\x58\x5F\x05\x20\x7C\x9E\x0E\x4E\x26\x18\x9B\xD7"
    "\x66\xC3\x36\xA6\xD3\x0E\xBD\x54\x86\x57\xEC\xE3\x93\x09\x67\xAC"
    "\xD7\xB3\x9A\x12\x43\x79\xCD\x51\x67\x10\x4A\x39\x98\x23\xD4\xE7"
    "\x79\xC2\x71\x31\xC7\xDA\x69\x7D\x73\x90\x96\x1E\x5D\xBF\x1B\x4B"
    "\xC5"};

const static uint8_t device_certificate_seed_card[] = {
    "\x7F\x21\x81\xC8\x93\x08\xAA\xBB\xFF\xAA\xBB\xFF\xAA\xBB\x42\x08"
    "\x53\x65\x65\x64\x43\x61\x72\x64\x5F\x20\x0C\x4F\x43\x45\x5F\x53"
    "\x65\x65\x64\x43\x61\x72\x64\x95\x02\x00\x80\x5F\x25\x04\x20\x26"
    "\x08\x04\x5F\x24\x04\x21\x26\x07\x11\x7F\x49\x46\xB0\x41\x04\xDF"
    "\xE8\x5A\x40\x3B\xD4\x9A\xBC\xDB\x31\x4E\x27\xCA\x98\x60\xCB\x80"
    "\xAB\x21\xAE\x36\x3B\x14\xD7\x6D\x37\x46\x68\x67\x90\xC1\xB4\xBC"
    "\x94\xC7\x2E\x2C\x9D\x61\x9C\x4B\xF7\x89\x68\x9F\x09\x9D\x60\x8C"
    "\xC0\x77\x4C\x96\xB0\x87\x52\x72\x82\xD9\xAC\x25\x39\xCF\x6A\xF0"
    "\x01\x00\x5F\x37\x47\x30\x45\x02\x20\x71\xF5\x76\x86\xE3\x29\x79"
    "\xAD\x1D\xD5\x81\x10\x60\x1B\x32\xBC\x64\x7C\x23\xC0\x68\x21\xF2"
    "\x65\x0F\x52\x72\xA2\xEA\xA9\xB9\x28\x02\x21\x00\xF6\x63\xEF\x13"
    "\x0F\x84\x80\x08\xFE\x98\x3A\xE8\x3A\x41\xBD\x02\x58\x39\xBB\x84"
    "\x9D\x1E\x18\xBD\x4C\x13\x56\xDC\x54\x6F\x53\xDE"};
#else
// QA test credentials - pair with the QA Seed Card key in a QA SE build.
const static uint8_t root_pubkey_seed_card[SCP11_P256_PUBKEY_SIZE] = {
    "\x04\x7E\xB9\xEB\x9C\x45\x7D\xCB\x9A\xA2\x31\x4D\x07\x2E\xB4\x7C"
    "\xA7\x1B\x28\x2B\xE6\x37\xCA\x1B\xE7\xF4\xDE\x99\x0C\xDF\x53\xBA"
    "\x0D\x2D\x11\x4B\xDE\x46\xEA\x14\x3D\x10\xE0\x5F\xF5\xB0\xF4\xF8"
    "\x37\x0B\xDF\x23\x1B\xF0\x65\x3F\x62\xF6\xC6\x3D\x3F\x45\xA6\x54"
    "\x8A"};

const static uint8_t device_certificate_seed_card[] = {
    "\x7F\x21\x81\xC8\x93\x08\xAA\xBB\xFF\xAA\xBB\xFF\xAA\xBB\x42\x08"
    "\x53\x65\x65\x64\x43\x61\x72\x64\x5F\x20\x0C\x4F\x43\x45\x5F\x53"
    "\x65\x65\x64\x43\x61\x72\x64\x95\x02\x00\x80\x5F\x25\x04\x20\x26"
    "\x06\x11\x5F\x24\x04\x21\x26\x05\x18\x7F\x49\x46\xB0\x41\x04\x5E"
    "\xAC\x0A\x98\xCA\xAC\x9E\x7D\x60\x8D\x71\x8A\x41\x3C\xF9\x70\x95"
    "\x82\x47\x0D\x90\x9F\x5D\xF4\x17\x94\x61\xB0\x9F\x1E\xB6\x7A\xC6"
    "\x10\x3B\x8A\x47\x2F\xAA\x08\xAA\xBC\x47\xC8\x32\xFF\x94\xE2\xA0"
    "\xEA\x53\x31\x08\xCB\xF5\x08\xC0\x14\xF3\xA5\x60\x60\xD7\xC5\xF0"
    "\x01\x00\x5F\x37\x47\x30\x45\x02\x21\x00\xDF\x68\xD5\x12\xE5\x13"
    "\x65\xDF\xC3\x74\x08\xF5\x19\xC6\xC2\xE8\x34\xC7\x82\xAA\xD7\x53"
    "\xE6\x58\x0F\xE4\xB4\xA2\xDA\x76\xA4\xF6\x02\x20\x7C\x58\x1D\xB7"
    "\xB2\x30\xB9\x93\xAC\x7A\xE7\xF4\xA6\xE6\x66\x60\x72\x5B\x53\x82"
    "\xBB\x0D\x0E\x2B\x36\xC5\x5F\xB3\xE6\xB3\xFD\x5A"};
#endif

const uint8_t shared_info_buffer[] = {GPC_TLV_SHAREDINFO_SCP_ID_PARAM,
                                      0x02,
                                      0x11,
                                      0x07,
                                      GPC_TLV_SHAREDINFO_KEYUSAGE,
                                      0x01,
                                      0x3c,
                                      GPC_TLV_SHAREDINFO_KEYTYPE,
                                      0x01,
                                      0x88,
                                      GPC_TLV_SHAREDINFO_KEYLENGTH,
                                      0x01,
                                      0x10,
                                      GPC_TLV_SHAREDINFO_HOSTID,
                                      0x08,
                                      0x70,
                                      0x72,
                                      0x6F,
                                      0x2D,
                                      0x6C,
                                      0x69,
                                      0x74,
                                      0x65};

const uint8_t shared_info_data_buffer[] = {0x3c, 0x88, 0x10, 0x08, 0x70, 0x72,
                                           0x6F, 0x2D, 0x6C, 0x69, 0x74, 0x65};

#define SET_CERT_TLV_FIELD(cert, tag, val, length) \
  do {                                             \
    cert->lv_##tag.value = val;                    \
    cert->lv_##tag.len = length;                   \
  } while (0)

bool scp11_get_pubkey(uint8_t *input, uint8_t inpue_len, uint8_t *pubkey) {
  uint8_t *p = input;
  uint8_t *end = input + inpue_len;
  uint16_t tag;
  uint16_t value_len = 0;
  uint16_t offset = 0;

  if (inpue_len == 0) {
    return false;
  }

  while (p < end) {
    if (!tlv_parse_tag(p, end, &tag, &offset)) {
      return false;
    }
    p += offset;

    if (!tlv_parse_length(p, end, &value_len, &offset)) {
      return false;
    }

    p += offset;

    switch (tag) {
      case GPC_TLV_PK_Q:
        if (value_len != 65) return false;
        memcpy(pubkey, p, value_len);
        break;
      case GPC_TLV_PK_PARAM:
        break;
      default:
        break;
    }
    p += value_len;
  }

  return true;
}

static bool scp11_equal_const_time(const uint8_t *a, const uint8_t *b,
                                   size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= a[i] ^ b[i];
  }
  return diff == 0;
}

static bool scp11_get_signature(uint8_t *input, uint8_t inpue_len,
                                uint8_t *signature) {
  uint8_t *p = input;
  uint8_t *end = input + inpue_len;
  uint16_t value_len = 0;
  uint16_t offset = 0;

  if (inpue_len == 0) {
    return false;
  }

  if (*p != 0x30) {
    return false;
  }
  p++;

  if (!tlv_parse_length(p, end, &value_len, &offset)) {
    return false;
  }

  p += offset;

  for (int i = 0; i < 2; i++) {
    if (p >= end || *p != 0x02) {
      return false;
    }
    p++;
    if (!tlv_parse_length(p, end, &value_len, &offset)) {
      return false;
    }
    p += offset;

    if (value_len > 32 && p < end && *p == 0x00) {
      p++;
      value_len--;
    }

    // NOTE: DER drops leading zero bytes, so r or s can be shorter than 32
    if (value_len == 0 || value_len > 32 || value_len > end - p) {
      return false;
    }
    memset(signature + i * 32, 0, 32 - value_len);
    memcpy(signature + i * 32 + (32 - value_len), p, value_len);
    p += value_len;
  }

  return true;
}

const uint8_t *scp11_root_pubkey(uint8_t version) {
  return version == MANAGE_CARD_VERSION_SEED ? root_pubkey_seed_card
                                             : root_pubkey_lite_card;
}

bool scp11_certificate_parse_and_verify(uint8_t *cert_raw, uint16_t cert_len,
                                        const uint8_t *verify_pubkey,
                                        scp11_certificate *cert) {
  uint16_t tag;
  uint8_t tag_len, len_len;
  uint16_t value_len = 0;
  uint16_t offset = 0;

  if (cert_len == 0 || verify_pubkey == NULL) {
    return false;
  }

  uint8_t *p = cert_raw;
  uint8_t *end = cert_raw + cert_len;

  SHA256_CTX ctx = {0};
  uint8_t digest[32] = {0}, signature[64] = {0};
  sha256_Init(&ctx);

  while (p < end) {
    if (!tlv_parse_tag(p, end, &tag, &offset)) {
      return false;
    }
    p += offset;
    tag_len = offset;

    if (!tlv_parse_length(p, end, &value_len, &offset)) {
      return false;
    }

    p += offset;
    len_len = offset;

    bool is_nested = tag_len == 1 ? tag & 0x20 : (tag >> 8) & 0x20;

    if (is_nested) {
      switch (tag) {
        case GPC_TLV_SCP11CRT_ENTITY:
          SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_ENTITY, p, value_len);
          break;
        case GPC_TLV_SCP11CRT_PUBKEY:
          SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_PUBKEY, p, value_len);
          sha256_Update(&ctx, p - tag_len - len_len,
                        tag_len + len_len + value_len);
          p += value_len;
          break;
        case GPC_TLV_SCP11CRT_BF_RESTR:
          SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_BF_RESTR, p, value_len);
          sha256_Update(&ctx, p - tag_len - len_len,
                        tag_len + len_len + value_len);
          p += value_len;
          break;
        default:
          break;
      }

      continue;
    }

    switch (tag) {
      case GPC_TLV_SCP11CRT_SN:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_SN, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_CAKLOCID:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_CAKLOCID, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_SUBJECTID:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_SUBJECTID, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_KEYUSAGE:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_KEYUSAGE, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_EFFEDATE:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_EFFEDATE, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_EXPEDATE:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_EXPEDATE, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_DISC_53:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_DISC_53, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_DISC_73:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_DISC_73, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_BF_RESTR:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_BF_RESTR, p, value_len);
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
      case GPC_TLV_SCP11CRT_SIGNATURE:
        SET_CERT_TLV_FIELD(cert, GPC_TLV_SCP11CRT_SIGNATURE, p, value_len);
        break;
      default:
        sha256_Update(&ctx, p - tag_len - len_len,
                      tag_len + len_len + value_len);
        break;
    }
    p += value_len;
  }
  sha256_Final(&ctx, digest);

  if (!scp11_get_signature(cert->lv_GPC_TLV_SCP11CRT_SIGNATURE.value,
                           cert->lv_GPC_TLV_SCP11CRT_SIGNATURE.len,
                           signature)) {
    return false;
  }

  return ecdsa_verify_digest(&nist256p1, verify_pubkey, signature, digest) == 0;
}

bool scp11_get_mutual_auth_data(uint8_t *data, uint8_t *data_len,
                                const scp11_context *scp11_ctx) {
  uint8_t *p = data;

  uint8_t len = sizeof(shared_info_buffer) + 2 + 68;

  if (*data_len < len) {
    return false;
  }

  *p++ = GPC_TLV_MA_CR;
  *p++ = scp11_ctx->shared_info.len;
  memcpy(p, scp11_ctx->shared_info.value, scp11_ctx->shared_info.len);
  p += scp11_ctx->shared_info.len;
  *p++ = GPC_TLV_MA_PK >> 8;
  *p++ = GPC_TLV_MA_PK & 0xff;
  *p++ = 0x41;
  memcpy(p, scp11_ctx->mutual_auth.oce_temp_public_key, 65);
  p += 65;

  *data_len = len;

  return true;
}

bool scp11_parse_response_msg(scp11_response_msg *resp_msg) {
  uint8_t *p = resp_msg->data;
  uint8_t *end = resp_msg->data + resp_msg->data_len;
  uint16_t tag, value_len, offset;

  if (!tlv_parse_tag(p, end, &tag, &offset)) {
    return false;
  }

  if (tag != GPC_TLV_MA_PK) {
    return false;
  }
  p += offset;

  if (!tlv_parse_length(p, end, &value_len, &offset)) {
    return false;
  }
  p += offset;

  resp_msg->lv_GPC_TLV_MA_PK.value = p;
  resp_msg->lv_GPC_TLV_MA_PK.len = value_len;

  p += value_len;

  if (!tlv_parse_tag(p, end, &tag, &offset)) {
    return false;
  }
  p += offset;

  if (tag != GPC_TLV_MA_RECEIPT) {
    return false;
  }

  if (!tlv_parse_length(p, end, &value_len, &offset)) {
    return false;
  }
  p += offset;

  if (value_len != 16) {
    return false;
  }

  resp_msg->lv_GPC_TLV_MA_RECEIPT.value = p;
  resp_msg->lv_GPC_TLV_MA_RECEIPT.len = value_len;

  return true;
}

// Static ECDH: the card's SD public key against the private key of our own OCE
// certificate. Both keys are held by the SE; only the SE-side key slot differs
// between the two card generations. The SE takes the raw 64-byte point and
// returns the 65-byte shared point that the KDF below expects.
static bool scp11_static_ecdh(const scp11_context *scp11_ctx,
                              uint8_t *shsss_key, uint8_t version) {
  const uint8_t *sd_pubkey_raw = scp11_ctx->mutual_auth.sd_public_key + 1;
  if (version == MANAGE_CARD_VERSION_SEED) {
    return se_seed_card_ecdh(sd_pubkey_raw, shsss_key) == 0;
  }
  return se_lite_card_ecdh(sd_pubkey_raw, shsss_key) == 0;
}

// SCP11 session key derivation. Both generations hash
//   z || counter || shared info || card id
// but disagree on how z is built, which certificate field identifies the card,
// and whether that field is length-prefixed.
static bool scp11_derive_session_keys(scp11_context *scp11_ctx,
                                      const uint8_t *z, uint16_t z_len,
                                      const tlv_value_t *card_id,
                                      bool length_prefixed) {
  uint8_t kdf_input[64 + 4 + sizeof(shared_info_data_buffer) + 1 + 32] = {0};
  uint8_t digest[32] = {0};
  uint16_t len = 0, counter_offset = 0;

  if (z_len > 64 || card_id->value == NULL || card_id->len == 0 ||
      card_id->len > 32) {
    return false;
  }

  memcpy(kdf_input, z, z_len);
  len = z_len;
  kdf_input[len++] = 0x00;
  kdf_input[len++] = 0x00;
  kdf_input[len++] = 0x00;
  counter_offset = len;
  kdf_input[len++] = 0x01;
  memcpy(kdf_input + len, shared_info_data_buffer,
         sizeof(shared_info_data_buffer));
  len += sizeof(shared_info_data_buffer);
  if (length_prefixed) {
    kdf_input[len++] = (uint8_t)card_id->len;
  }
  memcpy(kdf_input + len, card_id->value, card_id->len);
  len += card_id->len;

  sha256_Raw(kdf_input, len, digest);
  memcpy(scp11_ctx->session_key.key_dek, digest, SPC11_SESSION_KEY_SIZE);
  memcpy(scp11_ctx->session_key.s_enc, digest + SPC11_SESSION_KEY_SIZE,
         SPC11_SESSION_KEY_SIZE);

  kdf_input[counter_offset] = 0x02;
  sha256_Raw(kdf_input, len, digest);
  memcpy(scp11_ctx->session_key.s_mac, digest, SPC11_SESSION_KEY_SIZE);
  memcpy(scp11_ctx->session_key.s_rmac, digest + SPC11_SESSION_KEY_SIZE,
         SPC11_SESSION_KEY_SIZE);

  memzero(digest, sizeof(digest));
  memzero(kdf_input, sizeof(kdf_input));
  return true;
}

bool scp11_open_secure_channel(scp11_context *scp11_ctx, uint8_t version) {
  uint8_t shsss_key[SCP11_P256_PUBKEY_SIZE] = {0};
  uint8_t shses_key[SCP11_P256_PUBKEY_SIZE] = {0};
  uint8_t z[64] = {0};
  uint8_t mac_data[255] = {0}, mac[16] = {0};
  uint8_t mutual_auth_len = sizeof(mac_data);
  uint16_t mac_data_len = 0, receipt_offset = 0;
  bool ret = false;

  if (!scp11_parse_response_msg(&scp11_ctx->response_msg)) {
    printf("scp11: mutual auth response did not parse\n");
    goto cleanup;
  }

  if (!scp11_get_pubkey(scp11_ctx->sd_cert.lv_GPC_TLV_SCP11CRT_PUBKEY.value,
                        scp11_ctx->sd_cert.lv_GPC_TLV_SCP11CRT_PUBKEY.len,
                        scp11_ctx->mutual_auth.sd_public_key)) {
    goto cleanup;
  }
  if (scp11_ctx->mutual_auth.sd_public_key[0] != 0x04) {
    printf("scp11: SD public key is not an uncompressed point\n");
    goto cleanup;
  }

  if (!scp11_static_ecdh(scp11_ctx, shsss_key, version)) {
    printf("scp11: static ECDH failed in the SE (locked, or key mismatch)\n");
    goto cleanup;
  }

  if (0 != ecdh_multiply(&nist256p1,
                         scp11_ctx->mutual_auth.oce_temp_private_key,
                         scp11_ctx->mutual_auth.sd_public_key, shses_key)) {
    goto cleanup;
  }

  if (version == MANAGE_CARD_VERSION_SEED) {
    // Seed Card: z is the two shared secrets' X coordinates back to back, and
    // the card is named by the certificate subject id with a length prefix.
    memcpy(z, shses_key + 1, 32);
    memcpy(z + 32, shsss_key + 1, 32);
    if (!scp11_derive_session_keys(
            scp11_ctx, z, 64, &scp11_ctx->sd_cert.lv_GPC_TLV_SCP11CRT_SUBJECTID,
            true)) {
      goto cleanup;
    }
  } else {
    // Lite Card: z is SHA-1 of each shared secret, card named by CA-KLOC-ID.
    sha1_Raw(shses_key + 1, 32, z);
    sha1_Raw(shsss_key + 1, 32, z + 20);
    if (!scp11_derive_session_keys(
            scp11_ctx, z, 40, &scp11_ctx->sd_cert.lv_GPC_TLV_SCP11CRT_CAKLOCID,
            false)) {
      goto cleanup;
    }
  }

  // The receipt is a CMAC over our mutual-auth request followed by everything
  // in the card's answer up to and including its ephemeral public key.
  if (!scp11_get_mutual_auth_data(mac_data, &mutual_auth_len, scp11_ctx)) {
    goto cleanup;
  }
  mac_data_len = mutual_auth_len;
  receipt_offset = (uint16_t)(scp11_ctx->response_msg.lv_GPC_TLV_MA_PK.value -
                              scp11_ctx->response_msg.data) +
                   scp11_ctx->response_msg.lv_GPC_TLV_MA_PK.len;
  if (receipt_offset > sizeof(mac_data) - mac_data_len) {
    goto cleanup;
  }
  memcpy(mac_data + mac_data_len, scp11_ctx->response_msg.data, receipt_offset);
  mac_data_len += receipt_offset;

  AES128_CMAC(scp11_ctx->session_key.key_dek, mac_data, mac_data_len, mac);

  if (!scp11_equal_const_time(
          mac, scp11_ctx->response_msg.lv_GPC_TLV_MA_RECEIPT.value,
          SCP11_RECEIPT_SIZE)) {
    printf("scp11: receipt mismatch - session keys disagree with the card\n");
    goto cleanup;
  }

  scp11_ctx->is_secure_channel_opened = true;
  ret = true;

cleanup:
  memzero(scp11_ctx->mutual_auth.oce_temp_private_key,
          sizeof(scp11_ctx->mutual_auth.oce_temp_private_key));
  memzero(shsss_key, sizeof(shsss_key));
  memzero(shses_key, sizeof(shses_key));
  memzero(z, sizeof(z));
  memzero(mac, sizeof(mac));
  return ret;
}

bool scp11_init(scp11_context *scp11_ctx, uint8_t version) {
  const uint8_t *device_certificate = NULL;
  uint16_t device_certificate_len = 0;

  if (version == MANAGE_CARD_VERSION_SEED) {
    device_certificate = device_certificate_seed_card;
    device_certificate_len = sizeof(device_certificate_seed_card) - 1;
  } else {
    device_certificate = device_certificate_lite_card;
    device_certificate_len = sizeof(device_certificate_lite_card) - 1;
  }
  if (device_certificate_len > SCP11_MAX_CERTIFICATE_SIZE) {
    return false;
  }

  memset(scp11_ctx, 0, sizeof(*scp11_ctx));

  scp11_ctx->shared_info.value = (uint8_t *)shared_info_buffer;
  scp11_ctx->shared_info.len = sizeof(shared_info_buffer);

  memcpy(scp11_ctx->oce_cert.raw, device_certificate, device_certificate_len);
  scp11_ctx->oce_cert.raw_len = device_certificate_len;

  scp11_ctx->response_msg.data_len = sizeof(scp11_ctx->response_msg.data);

  // The OCE certificate is signed by the same CA as the card's, so parsing it
  // through the same path also proves the built-in certificate is intact.
  if (!scp11_certificate_parse_and_verify(
          scp11_ctx->oce_cert.raw, scp11_ctx->oce_cert.raw_len,
          scp11_root_pubkey(version), &scp11_ctx->oce_cert)) {
    return false;
  }

  if (!scp11_get_pubkey(scp11_ctx->oce_cert.lv_GPC_TLV_SCP11CRT_PUBKEY.value,
                        scp11_ctx->oce_cert.lv_GPC_TLV_SCP11CRT_PUBKEY.len,
                        scp11_ctx->mutual_auth.oce_public_key)) {
    return false;
  }

  random_buffer(scp11_ctx->mutual_auth.oce_temp_private_key,
                sizeof(scp11_ctx->mutual_auth.oce_temp_private_key));

  ecdsa_get_public_key65(&nist256p1,
                         scp11_ctx->mutual_auth.oce_temp_private_key,
                         scp11_ctx->mutual_auth.oce_temp_public_key);
  return true;
}

void scp11_close_secure_channel(scp11_context *scp11_ctx) {
  // The same context is reused across taps and card generations, so stale
  // session keys and a stale SD certificate must not reach the next handshake.
  memzero(scp11_ctx, sizeof(*scp11_ctx));
}

bool scp11_get_secure_channel_status(scp11_context *scp11_ctx) {
  return scp11_ctx->is_secure_channel_opened;
}
