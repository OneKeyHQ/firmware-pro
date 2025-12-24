/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>

#include "boot_msg_pb.h"

#include "image.h"
#include "secbool.h"

// defines

#define USB_TIMEOUT 500
#define USB_PACKET_SIZE 64

#define FIRMWARE_UPLOAD_CHUNK_RETRY_COUNT 2

// macros

#define STR(X) #X
#define VERSTR(X) STR(X)

// -- legacy msg_send
#define MSG_SEND_INIT(TYPE) TYPE msg_send = TYPE##_init_default
#define MSG_SEND_ASSIGN_REQUIRED_VALUE(FIELD, VALUE) \
  { msg_send.FIELD = VALUE; }
#define MSG_SEND_ASSIGN_VALUE(FIELD, VALUE) \
  {                                         \
    msg_send.has_##FIELD = true;            \
    msg_send.FIELD = VALUE;                 \
  }
#define MSG_SEND_ASSIGN_STRING(FIELD, VALUE)                    \
  {                                                             \
    msg_send.has_##FIELD = true;                                \
    memzero(msg_send.FIELD, sizeof(msg_send.FIELD));            \
    strncpy(msg_send.FIELD, VALUE, sizeof(msg_send.FIELD) - 1); \
  }
#define MSG_SEND_ASSIGN_STRING_LEN(FIELD, VALUE, LEN)                     \
  {                                                                       \
    msg_send.has_##FIELD = true;                                          \
    memzero(msg_send.FIELD, sizeof(msg_send.FIELD));                      \
    strncpy(msg_send.FIELD, VALUE, MIN(LEN, sizeof(msg_send.FIELD) - 1)); \
  }
#define MSG_SEND_ASSIGN_BYTES(FIELD, VALUE, LEN)                  \
  {                                                               \
    msg_send.has_##FIELD = true;                                  \
    memzero(msg_send.FIELD.bytes, sizeof(msg_send.FIELD.bytes));  \
    memcpy(msg_send.FIELD.bytes, VALUE,                           \
           MIN(LEN, sizeof(msg_send.FIELD.bytes)));               \
    msg_send.FIELD.size = MIN(LEN, sizeof(msg_send.FIELD.bytes)); \
  }

#define MSG_SEND_ASSIGN_REQUIRED_BYTES(FIELD, VALUE, LEN)         \
  {                                                               \
    memzero(msg_send.FIELD.bytes, sizeof(msg_send.FIELD.bytes));  \
    memcpy(msg_send.FIELD.bytes, VALUE,                           \
           MIN(LEN, sizeof(msg_send.FIELD.bytes)));               \
    msg_send.FIELD.size = MIN(LEN, sizeof(msg_send.FIELD.bytes)); \
  }
#define MSG_SEND(TYPE) \
  _send_msg(iface_num, MessageType_MessageType_##TYPE, TYPE##_fields, &msg_send)

// -- legacy msg_recv
#define MSG_RECV_INIT(TYPE) TYPE msg_recv = TYPE##_init_default
#define MSG_RECV_CALLBACK(FIELD, CALLBACK, ARGUMENT) \
  {                                                  \
    msg_recv.FIELD.funcs.decode = &CALLBACK;         \
    msg_recv.FIELD.arg = (void*)ARGUMENT;            \
  }
#define MSG_RECV(TYPE) \
  _recv_msg(iface_num, msg_size, buf, TYPE##_fields, &msg_recv)

// -- msg new
#define MSG_CONSTRUCT(VAR, TYPE) TYPE VAR = TYPE##_init_default
#define MSG_HAS(VAR, FIELD) VAR.has_##FIELD = true;
#define MSG_HAS_P(VAR, FIELD) VAR->has_##FIELD = true;
#define MSG_ASSIGN_VAL(VAR_FIELD, VALUE) VAR_FIELD = VALUE;
#define MSG_ASSIGN_BYTES(VAR_FIELD, VALUE, SIZE)                        \
  {                                                                     \
    memzero(VAR_FIELD.bytes, sizeof(VAR_FIELD.bytes));                  \
    memcpy(VAR_FIELD.bytes, VALUE, MIN(SIZE, sizeof(VAR_FIELD.bytes))); \
    VAR_FIELD.size = MIN(SIZE, sizeof(VAR_FIELD.bytes));                \
  }
#define MSG_ASSIGN_STRING(VAR_FIELD, VALUE, SIZE)                  \
  {                                                                \
    memzero(VAR_FIELD, sizeof(VAR_FIELD));                         \
    if (SIZE > 0)                                                  \
      strncpy(VAR_FIELD, VALUE, MIN(SIZE, sizeof(VAR_FIELD) - 1)); \
    else                                                           \
      strncpy(VAR_FIELD, VALUE, sizeof(VAR_FIELD) - 1);            \
  }
#define MSG_ASSIGN(HANDLER, VAR_FIELD, VALUE, ...) \
  HANDLER(VAR_FIELD, VALUE, ##__VA_ARGS__);
#define MSG_ASSIGN_HAS(HANDLER, VAR, FIELD, VALUE, ...)   \
  {                                                       \
    MSG_ASSIGN(HANDLER, VAR.FIELD, VALUE, ##__VA_ARGS__); \
    MSG_HAS(VAR, FIELD);                                  \
  }
#define MSG_ASSIGN_HAS_P(HANDLER, VAR, FIELD, VALUE, ...)  \
  {                                                        \
    MSG_ASSIGN(HANDLER, VAR->FIELD, VALUE, ##__VA_ARGS__); \
    MSG_HAS_P(VAR, FIELD);                                 \
  }
#define MSG_TEST_HAS(VAR, FIELD) (VAR.has_##FIELD)
#define MSG_TEST_HAS_P(VAR, FIELD) (VAR->has_##FIELD)
#define MSG_TEST_HAS_TRUE(VAR, FIELD) (VAR.has_##FIELD && VAR.FIELD)
#define MSG_TEST_HAS_TRUE_P(VAR, FIELD) (VAR->has_##FIELD && VAR->FIELD)

#define MSG_ASSIGN_SE_INFO(VAR, seXX)                                          \
  {                                                                            \
    uint8_t se_state;                                                          \
    if (seXX##_get_state(&se_state)) {                                         \
      MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, VAR.boot, build_id,                    \
                     (char*)seXX##_get_boot_build_id(), 8);                    \
      MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, VAR.boot, hash, seXX##_get_boot_hash(), \
                     32);                                                      \
                                                                               \
      if (se_state == 0x00) {                                                  \
        /* BOOT */                                                             \
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, VAR.boot, version,                   \
                       (char*)seXX##_get_version(),                            \
                       strlen((char*)seXX##_get_version()));                   \
        MSG_HAS(VAR, boot);                                                    \
      } else {                                                                 \
        /* APP */                                                              \
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, VAR.app, version,                    \
                       (char*)seXX##_get_version(),                            \
                       strlen((char*)seXX##_get_version()));                   \
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, VAR.app, build_id,                   \
                       (char*)seXX##_get_build_id(), 8);                       \
        MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, VAR.app, hash, seXX##_get_hash(),     \
                       32);                                                    \
        MSG_HAS(VAR, app);                                                     \
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, VAR.boot, version,                   \
                       (char*)seXX##_get_version(),                            \
                       strlen((char*)seXX##_get_version()));                   \
        MSG_HAS(VAR, boot);                                                    \
      }                                                                        \
      MSG_ASSIGN_HAS(MSG_ASSIGN_VAL, VAR, state, se_state);                    \
    }                                                                          \
    MSG_ASSIGN_HAS(MSG_ASSIGN_VAL, VAR, type, OneKeySeType_THD89);             \
  }

void send_failure(uint8_t iface_num, FailureType type, const char* text);
void send_success(uint8_t iface_num, const char* text);

void send_user_abort(uint8_t iface_num, const char* msg);

secbool msg_parse_header(const uint8_t* buf, uint16_t* msg_id,
                         uint32_t* msg_size);

void process_msg_StartSession(uint8_t iface_num, uint32_t msg_size,
                              uint8_t* buf, const vendor_header* const vhdr,
                              const image_header* const hdr);
void process_msg_GetFeatures(uint8_t iface_num, uint32_t msg_size, uint8_t* buf,
                             const vendor_header* const vhdr,
                             const image_header* const hdr);
void process_msg_Ping(uint8_t iface_num, uint32_t msg_size, uint8_t* buf);
void process_msg_OneKeyReboot(uint8_t iface_num, uint32_t msg_size,
                              uint8_t* buf);
int process_msg_WipeDevice(uint8_t iface_num, uint32_t msg_size, uint8_t* buf);

void process_msg_DeviceInfoSettings(uint8_t iface_num, uint32_t msg_size,
                                    uint8_t* buf);
void process_msg_GetDeviceInfo(uint8_t iface_num, uint32_t msg_size,
                               uint8_t* buf);
void process_msg_WriteSEPrivateKey(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t* buf);
void process_msg_ReadSEPublicKey(uint8_t iface_num, uint32_t msg_size,
                                 uint8_t* buf);
void process_msg_WriteSEPublicCert(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t* buf);
void process_msg_ReadSEPublicCert(uint8_t iface_num, uint32_t msg_size,
                                  uint8_t* buf);
void process_msg_SESignMessage(uint8_t iface_num, uint32_t msg_size,
                               uint8_t* buf);
void process_msg_unknown(uint8_t iface_num, uint32_t msg_size, uint8_t* buf);

#endif
