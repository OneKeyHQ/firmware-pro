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

#include "blake2s.h"
#include "br_check.h"
#include "common.h"
#include "device.h"
#include "display.h"
#include "flash.h"
#include "image.h"
#include "se_thd89.h"
#include "secbool.h"
#include "thd89_boot.h"
#include "usb.h"
#include "version.h"

#include "bootui.h"
#include "messages.h"

#include "memzero.h"

#include "ble.h"
#include "bootui.h"
#include "nordic_dfu.h"
#include "spi_legacy.h"

#define MSG_HEADER1_LEN 9
#define MSG_HEADER2_LEN 1

secbool msg_parse_header(const uint8_t *buf, uint16_t *msg_id,
                         uint32_t *msg_size) {
  if (buf[0] != '?' || buf[1] != '#' || buf[2] != '#') {
    return secfalse;
  }
  *msg_id = (buf[3] << 8) + buf[4];
  *msg_size = (buf[5] << 24) + (buf[6] << 16) + (buf[7] << 8) + buf[8];
  return sectrue;
}

typedef struct {
  uint8_t iface_num;
  uint8_t packet_index;
  uint8_t packet_pos;
  uint8_t buf[USB_PACKET_SIZE];
} usb_write_state;

/* we don't use secbool/sectrue/secfalse here as it is a nanopb api */
static bool _usb_write(pb_ostream_t *stream, const pb_byte_t *buf,
                       size_t count) {
  usb_write_state *state = (usb_write_state *)(stream->state);

  size_t written = 0;
  // while we have data left
  while (written < count) {
    size_t remaining = count - written;
    // if all remaining data fit into our packet
    if (state->packet_pos + remaining <= USB_PACKET_SIZE) {
      // append data from buf to state->buf
      memcpy(state->buf + state->packet_pos, buf + written, remaining);
      // advance position
      state->packet_pos += remaining;
      // and return
      return true;
    } else {
      // append data that fits
      memcpy(state->buf + state->packet_pos, buf + written,
             USB_PACKET_SIZE - state->packet_pos);
      written += USB_PACKET_SIZE - state->packet_pos;
      // send packet
      int r;
      if (host_channel == CHANNEL_USB) {
        r = usb_webusb_write_blocking(state->iface_num, state->buf,
                                      USB_PACKET_SIZE, USB_TIMEOUT);
      } else {
        hal_delay(5);
        r = spi_slave_send(state->buf, USB_PACKET_SIZE, USB_TIMEOUT);
      }
      ensure(sectrue * (r == USB_PACKET_SIZE), NULL);
      // prepare new packet
      state->packet_index++;
      memzero(state->buf, USB_PACKET_SIZE);
      state->buf[0] = '?';
      state->packet_pos = MSG_HEADER2_LEN;
    }
  }

  return true;
}

static void _usb_write_flush(usb_write_state *state) {
  // if packet is not filled up completely
  if (state->packet_pos < USB_PACKET_SIZE) {
    // pad it with zeroes
    memzero(state->buf + state->packet_pos,
            USB_PACKET_SIZE - state->packet_pos);
  }
  // send packet
  int r;
  if (host_channel == CHANNEL_USB) {
    r = usb_webusb_write_blocking(state->iface_num, state->buf, USB_PACKET_SIZE,
                                  USB_TIMEOUT);
  } else {
    hal_delay(5);
    r = spi_slave_send(state->buf, USB_PACKET_SIZE, USB_TIMEOUT);
  }
  ensure(sectrue * (r == USB_PACKET_SIZE), NULL);
}

static secbool _send_msg(uint8_t iface_num, uint16_t msg_id,
                         const pb_msgdesc_t *fields, const void *msg) {
  // determine message size by serializing it into a dummy stream
  pb_ostream_t sizestream = {.callback = NULL,
                             .state = NULL,
                             .max_size = SIZE_MAX,
                             .bytes_written = 0,
                             .errmsg = NULL};
  if (false == pb_encode(&sizestream, fields, msg)) {
    return secfalse;
  }
  const uint32_t msg_size = sizestream.bytes_written;

  usb_write_state state = {
      .iface_num = iface_num,
      .packet_index = 0,
      .packet_pos = MSG_HEADER1_LEN,
      .buf =
          {
              '?',
              '#',
              '#',
              (msg_id >> 8) & 0xFF,
              msg_id & 0xFF,
              (msg_size >> 24) & 0xFF,
              (msg_size >> 16) & 0xFF,
              (msg_size >> 8) & 0xFF,
              msg_size & 0xFF,
          },
  };

  pb_ostream_t stream = {.callback = &_usb_write,
                         .state = &state,
                         .max_size = SIZE_MAX,
                         .bytes_written = 0,
                         .errmsg = NULL};

  if (false == pb_encode(&stream, fields, msg)) {
    return secfalse;
  }

  _usb_write_flush(&state);

  return sectrue;
}

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

#define STR(X) #X
#define VERSTR(X) STR(X)

typedef struct {
  uint8_t iface_num;
  uint8_t packet_index;
  uint8_t packet_pos;
  uint8_t *buf;
} usb_read_state;

static void _usb_webusb_read_retry(uint8_t iface_num, uint8_t *buf) {
  for (int retry = 0;; retry++) {
    int r =
        usb_webusb_read_blocking(iface_num, buf, USB_PACKET_SIZE, USB_TIMEOUT);
    if (r != USB_PACKET_SIZE) {  // reading failed
      if (r == 0 && retry < 10) {
        // only timeout => let's try again
      } else {
        // error
        error_shutdown("Error reading", "from USB.", "Try different",
                       "USB cable.");
      }
    }
    return;  // success
  }
}

/* we don't use secbool/sectrue/secfalse here as it is a nanopb api */
static bool _usb_read(pb_istream_t *stream, uint8_t *buf, size_t count) {
  usb_read_state *state = (usb_read_state *)(stream->state);

  size_t read = 0;
  // while we have data left
  while (read < count) {
    size_t remaining = count - read;
    // if all remaining data fit into our packet
    if (state->packet_pos + remaining <= USB_PACKET_SIZE) {
      // append data from buf to state->buf
      memcpy(buf + read, state->buf + state->packet_pos, remaining);
      // advance position
      state->packet_pos += remaining;
      // and return
      return true;
    } else {
      // append data that fits
      memcpy(buf + read, state->buf + state->packet_pos,
             USB_PACKET_SIZE - state->packet_pos);
      read += USB_PACKET_SIZE - state->packet_pos;
      if (host_channel == CHANNEL_USB) {
        // read next packet (with retry)
        _usb_webusb_read_retry(state->iface_num, state->buf);
      } else {
        if (spi_slave_poll(state->buf) == 0) {
          spi_read_retry(state->buf);
        }
      }
      // prepare next packet
      state->packet_index++;
      state->packet_pos = MSG_HEADER2_LEN;
    }
  }

  return true;
}

static void _usb_read_flush(usb_read_state *state) { (void)state; }

static secbool _recv_msg(uint8_t iface_num, uint32_t msg_size, uint8_t *buf,
                         const pb_msgdesc_t *fields, void *msg) {
  usb_read_state state = {.iface_num = iface_num,
                          .packet_index = 0,
                          .packet_pos = MSG_HEADER1_LEN,
                          .buf = buf};

  pb_istream_t stream = {.callback = &_usb_read,
                         .state = &state,
                         .bytes_left = msg_size,
                         .errmsg = NULL};

  if (false == pb_decode_noinit(&stream, fields, msg)) {
    return secfalse;
  }

  _usb_read_flush(&state);

  return sectrue;
}

#define MSG_RECV_INIT(TYPE) TYPE msg_recv = TYPE##_init_default
#define MSG_RECV_CALLBACK(FIELD, CALLBACK, ARGUMENT) \
  {                                                  \
    msg_recv.FIELD.funcs.decode = &CALLBACK;         \
    msg_recv.FIELD.arg = (void *)ARGUMENT;           \
  }
#define MSG_RECV(TYPE) \
  _recv_msg(iface_num, msg_size, buf, TYPE##_fields, &msg_recv)

void send_success(uint8_t iface_num, const char *text) {
  MSG_SEND_INIT(Success);
  MSG_SEND_ASSIGN_STRING(message, text);
  MSG_SEND(Success);
}

void send_failure(uint8_t iface_num, FailureType type, const char *text) {
  MSG_SEND_INIT(Failure);
  MSG_SEND_ASSIGN_VALUE(code, type);
  MSG_SEND_ASSIGN_STRING(message, text);
  MSG_SEND(Failure);
}

void send_user_abort(uint8_t iface_num, const char *msg) {
  MSG_SEND_INIT(Failure);
  MSG_SEND_ASSIGN_VALUE(code, FailureType_Failure_ActionCancelled);
  MSG_SEND_ASSIGN_STRING(message, msg);
  MSG_SEND(Failure);
}

static void send_msg_features(uint8_t iface_num,
                              const vendor_header *const vhdr,
                              const image_header *const hdr) {
  MSG_SEND_INIT(Features);
  if (device_is_factory_mode()) {
    uint32_t init_state = 0;
    MSG_SEND_ASSIGN_STRING(vendor, "onekey.so");
    MSG_SEND_ASSIGN_REQUIRED_VALUE(major_version, VERSION_MAJOR);
    MSG_SEND_ASSIGN_REQUIRED_VALUE(minor_version, VERSION_MINOR);
    MSG_SEND_ASSIGN_REQUIRED_VALUE(patch_version, VERSION_PATCH);
    MSG_SEND_ASSIGN_VALUE(bootloader_mode, true);
    MSG_SEND_ASSIGN_STRING(model, "factory");
    init_state |= device_serial_set() ? 1 : 0;
    init_state |= se_has_cerrificate() ? (1 << 2) : 0;
    MSG_SEND_ASSIGN_VALUE(initstates, init_state);
    MSG_SEND_ASSIGN_VALUE(onekey_device_type, OneKeyDeviceType_PRO);

  } else {
    MSG_SEND_ASSIGN_STRING(vendor, "onekey.so");
    MSG_SEND_ASSIGN_REQUIRED_VALUE(major_version, VERSION_MAJOR);
    MSG_SEND_ASSIGN_REQUIRED_VALUE(minor_version, VERSION_MINOR);
    MSG_SEND_ASSIGN_REQUIRED_VALUE(patch_version, VERSION_PATCH);
    MSG_SEND_ASSIGN_VALUE(bootloader_mode, true);
    MSG_SEND_ASSIGN_STRING(model, "T");
    if (vhdr && hdr) {
      MSG_SEND_ASSIGN_VALUE(firmware_present, true);
      MSG_SEND_ASSIGN_VALUE(fw_major, (hdr->version & 0xFF));
      MSG_SEND_ASSIGN_VALUE(fw_minor, ((hdr->version >> 8) & 0xFF));
      MSG_SEND_ASSIGN_VALUE(fw_patch, ((hdr->version >> 16) & 0xFF));
      MSG_SEND_ASSIGN_STRING_LEN(fw_vendor, vhdr->vstr, vhdr->vstr_len);
      const char *ver_str = format_ver("%d.%d.%d", hdr->onekey_version);
      MSG_SEND_ASSIGN_STRING_LEN(onekey_version, ver_str, strlen(ver_str));
      MSG_SEND_ASSIGN_STRING_LEN(onekey_firmware_version, ver_str,
                                 strlen(ver_str));

    } else {
      MSG_SEND_ASSIGN_VALUE(firmware_present, false);
    }
    if (ble_name_state()) {
      MSG_SEND_ASSIGN_STRING_LEN(ble_name, ble_get_name(), BLE_NAME_LEN);
      MSG_SEND_ASSIGN_STRING_LEN(onekey_ble_name, ble_get_name(), BLE_NAME_LEN);
    }
    if (ble_ver_state()) {
      char *ble_version = ble_get_ver();
      MSG_SEND_ASSIGN_STRING_LEN(ble_ver, ble_version, strlen(ble_version));
      MSG_SEND_ASSIGN_STRING_LEN(onekey_ble_version, ble_version,
                                 strlen(ble_version));
    }
    if (ble_switch_state()) {
      MSG_SEND_ASSIGN_VALUE(ble_enable, ble_get_switch());
    }

    uint8_t state;
    char *se_version;

#define GET_SE_INFO(se_prefix)                                               \
  do {                                                                       \
    if (se_prefix##_get_state(&state)) {                                     \
      MSG_SEND_ASSIGN_VALUE(onekey_##se_prefix##_state, state);              \
      se_version = se_prefix##_get_version();                                \
      if (se_version) {                                                      \
        MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_version, se_version, \
                                   strlen(se_version));                      \
      }                                                                      \
    }                                                                        \
  } while (0)

    GET_SE_INFO(se01);
    GET_SE_INFO(se02);
    GET_SE_INFO(se03);
    GET_SE_INFO(se04);

    char *serial = NULL;
    if (device_get_serial(&serial)) {
      MSG_SEND_ASSIGN_STRING_LEN(serial_no, serial, strlen(serial));
      MSG_SEND_ASSIGN_STRING_LEN(onekey_serial_no, serial, strlen(serial));
    }
    char *board_version = get_boardloader_version();
    MSG_SEND_ASSIGN_STRING_LEN(boardloader_version, board_version,
                               strlen(board_version));
    MSG_SEND_ASSIGN_STRING_LEN(onekey_board_version, board_version,
                               strlen(board_version));

    int boot_version_len = strlen((VERSTR(VERSION_MAJOR) "." VERSTR(
        VERSION_MINOR) "." VERSTR(VERSION_PATCH)));
    MSG_SEND_ASSIGN_STRING_LEN(onekey_boot_version,
                               (VERSTR(VERSION_MAJOR) "." VERSTR(
                                   VERSION_MINOR) "." VERSTR(VERSION_PATCH)),
                               boot_version_len);
    MSG_SEND_ASSIGN_VALUE(onekey_device_type, OneKeyDeviceType_PRO);
    MSG_SEND_ASSIGN_VALUE(onekey_se_type, OneKeySeType_THD89);
  }

  MSG_SEND(Features);
}

static void send_msg_features_ex(uint8_t iface_num,
                                 const vendor_header *const vhdr,
                                 const image_header *const hdr) {
  MSG_SEND_INIT(OnekeyFeatures);

  if (vhdr && hdr) {
    const char *ver_str = format_ver("%d.%d.%d", hdr->onekey_version);
    MSG_SEND_ASSIGN_STRING_LEN(onekey_firmware_version, ver_str,
                               strlen(ver_str));

    uint8_t *fimware_hash = get_firmware_hash();
    MSG_SEND_ASSIGN_BYTES(onekey_firmware_hash, fimware_hash, 32);
  }
  if (ble_name_state()) {
    MSG_SEND_ASSIGN_STRING_LEN(onekey_ble_name, ble_get_name(), BLE_NAME_LEN);
  }
  if (ble_ver_state()) {
    char *ble_version = ble_get_ver();
    MSG_SEND_ASSIGN_STRING_LEN(onekey_ble_version, ble_version,
                               strlen(ble_version));
  }
  uint8_t state;
  char *se_version, *se_build_id;
  uint8_t *se_hash;

#define GET_SE_INFO_EX(se_prefix)                                              \
  do {                                                                         \
    if (se_prefix##_get_state(&state)) {                                       \
      MSG_SEND_ASSIGN_VALUE(onekey_##se_prefix##_state, state);                \
                                                                               \
      se_hash = se_prefix##_get_boot_hash();                                   \
      if (se_hash) {                                                           \
        MSG_SEND_ASSIGN_BYTES(onekey_##se_prefix##_boot_hash, se_hash, 32);    \
      }                                                                        \
                                                                               \
      se_build_id = se_prefix##_get_boot_build_id();                           \
      if (se_build_id) {                                                       \
        MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_boot_build_id,         \
                                   se_build_id, strlen(se_build_id));          \
      }                                                                        \
                                                                               \
      if (state) {                                                             \
        /* APP */                                                              \
        se_version = se_prefix##_get_version();                                \
        if (se_version) {                                                      \
          MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_version, se_version, \
                                     strlen(se_version));                      \
        }                                                                      \
        se_hash = se_prefix##_get_hash();                                      \
        if (se_hash) {                                                         \
          MSG_SEND_ASSIGN_BYTES(onekey_##se_prefix##_hash, se_hash, 32);       \
        }                                                                      \
        se_build_id = se_prefix##_get_build_id();                              \
        if (se_build_id) {                                                     \
          MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_build_id,            \
                                     se_build_id, strlen(se_build_id));        \
        }                                                                      \
        se_version = se_prefix##_get_boot_version();                           \
        if (se_version) {                                                      \
          MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_boot_version,        \
                                     se_version, strlen(se_version));          \
        }                                                                      \
      } else {                                                                 \
        /* BOOT */                                                             \
        se_version = se_prefix##_get_version();                                \
        if (se_version) {                                                      \
          MSG_SEND_ASSIGN_STRING_LEN(onekey_##se_prefix##_boot_version,        \
                                     se_version, strlen(se_version));          \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  } while (0)

  GET_SE_INFO_EX(se01);
  GET_SE_INFO_EX(se02);
  GET_SE_INFO_EX(se03);
  GET_SE_INFO_EX(se04);

  char *board_build_id = get_boardloader_build_id();
  MSG_SEND_ASSIGN_STRING_LEN(onekey_board_build_id, board_build_id,
                             strlen(board_build_id));

  char *serial = NULL;
  if (device_get_serial(&serial)) {
    MSG_SEND_ASSIGN_STRING_LEN(onekey_serial_no, serial, strlen(serial));
  }
  char *board_version = get_boardloader_version();
  MSG_SEND_ASSIGN_STRING_LEN(onekey_board_version, board_version,
                             strlen(board_version));
  uint8_t *board_hash = get_boardloader_hash();
  MSG_SEND_ASSIGN_BYTES(onekey_board_hash, board_hash, 32);

  int boot_version_len = strlen((VERSTR(VERSION_MAJOR) "." VERSTR(
      VERSION_MINOR) "." VERSTR(VERSION_PATCH)));
  MSG_SEND_ASSIGN_STRING_LEN(onekey_boot_version,
                             (VERSTR(VERSION_MAJOR) "." VERSTR(
                                 VERSION_MINOR) "." VERSTR(VERSION_PATCH)),
                             boot_version_len);
  uint8_t *boot_hash = get_bootloader_hash();
  MSG_SEND_ASSIGN_BYTES(onekey_boot_hash, boot_hash, 32);
  MSG_SEND_ASSIGN_STRING_LEN(onekey_boot_build_id, (char *)BUILD_COMMIT,
                             strlen((char *)BUILD_COMMIT));
  MSG_SEND_ASSIGN_VALUE(onekey_device_type, OneKeyDeviceType_PRO);
  MSG_SEND_ASSIGN_VALUE(onekey_se_type, OneKeySeType_THD89);

  MSG_SEND(OnekeyFeatures);
}

void process_msg_Initialize(uint8_t iface_num, uint32_t msg_size, uint8_t *buf,
                            const vendor_header *const vhdr,
                            const image_header *const hdr) {
  MSG_RECV_INIT(Initialize);
  MSG_RECV(Initialize);
  send_msg_features(iface_num, vhdr, hdr);
}

void process_msg_GetFeatures(uint8_t iface_num, uint32_t msg_size, uint8_t *buf,
                             const vendor_header *const vhdr,
                             const image_header *const hdr) {
  MSG_RECV_INIT(GetFeatures);
  MSG_RECV(GetFeatures);
  send_msg_features(iface_num, vhdr, hdr);
}

void process_msg_OnekeyGetFeatures(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t *buf,
                                   const vendor_header *const vhdr,
                                   const image_header *const hdr) {
  MSG_RECV_INIT(OnekeyGetFeatures);
  MSG_RECV(OnekeyGetFeatures);
  send_msg_features_ex(iface_num, vhdr, hdr);
}

void process_msg_Ping(uint8_t iface_num, uint32_t msg_size, uint8_t *buf) {
  MSG_RECV_INIT(Ping);
  MSG_RECV(Ping);

  MSG_SEND_INIT(Success);
  MSG_SEND_ASSIGN_STRING(message, msg_recv.message);
  MSG_SEND(Success);
}

void process_msg_Reboot(uint8_t iface_num, uint32_t msg_size, uint8_t *buf) {
  MSG_RECV_INIT(Reboot);
  MSG_RECV(Reboot);

  switch (msg_recv.reboot_type) {
    case RebootType_Normal: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type Normal accepted!");
      MSG_SEND(Success);
    }
      *BOOT_TARGET_FLAG_ADDR = BOOT_TARGET_NORMAL;
      restart();
      break;
    case RebootType_Boardloader: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type Boardloader accepted!");
      MSG_SEND(Success);
    }
      reboot_to_board();
      break;
    case RebootType_BootLoader: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type BootLoader accepted!");
      MSG_SEND(Success);
    }
      reboot_to_boot();
      break;

    default: {
      MSG_SEND_INIT(Failure);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type invalid!");
      MSG_SEND(Failure);
    } break;
  }
}

secbool load_vendor_header_keys(const uint8_t *const data,
                                vendor_header *const vhdr);

int process_msg_WipeDevice(uint8_t iface_num, uint32_t msg_size, uint8_t *buf) {
  ui_screen_wipe_progress(0, 1000);
  if (sectrue != se_reset_storage()) {
    MSG_SEND_INIT(Failure);
    MSG_SEND_ASSIGN_VALUE(code, FailureType_Failure_ProcessError);
    MSG_SEND_ASSIGN_STRING(message, "Wipe device failed");
    MSG_SEND(Failure);
    return -1;
  } else {
    ui_screen_wipe_progress(1000, 1000);
    MSG_SEND_INIT(Success);
    MSG_SEND(Success);
    return 0;
  }
}

void process_msg_unknown(uint8_t iface_num, uint32_t msg_size, uint8_t *buf) {
  // consume remaining message
  int remaining_chunks = 0;

  if (msg_size > (USB_PACKET_SIZE - MSG_HEADER1_LEN)) {
    // calculate how many blocks need to be read to drain the message (rounded
    // up to not leave any behind)
    remaining_chunks = (msg_size - (USB_PACKET_SIZE - MSG_HEADER1_LEN) +
                        ((USB_PACKET_SIZE - MSG_HEADER2_LEN) - 1)) /
                       (USB_PACKET_SIZE - MSG_HEADER2_LEN);
  }

  for (int i = 0; i < remaining_chunks; i++) {
    // read next packet (with retry)
    _usb_webusb_read_retry(iface_num, buf);
  }

  MSG_SEND_INIT(Failure);
  MSG_SEND_ASSIGN_VALUE(code, FailureType_Failure_UnexpectedMessage);
  MSG_SEND_ASSIGN_STRING(message, "Unexpected message");
  MSG_SEND(Failure);
}

void process_msg_DeviceInfoSettings(uint8_t iface_num, uint32_t msg_size,
                                    uint8_t *buf) {
  MSG_RECV_INIT(DeviceInfoSettings);
  MSG_RECV(DeviceInfoSettings);

  if (msg_recv.has_serial_no) {
    if (!device_set_serial((char *)msg_recv.serial_no)) {
      send_failure(iface_num, FailureType_Failure_ProcessError,
                   "Set serial failed");
    } else {
      device_para_init();
      send_success(iface_num, "Set applied");
    }
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError, "serial null");
  }
}

void process_msg_GetDeviceInfo(uint8_t iface_num, uint32_t msg_size,
                               uint8_t *buf) {
  MSG_RECV_INIT(GetDeviceInfo);
  MSG_RECV(GetDeviceInfo);

  MSG_SEND_INIT(DeviceInfo);

  char *serial;
  if (device_get_serial(&serial)) {
    MSG_SEND_ASSIGN_STRING(serial_no, serial);
  }
  MSG_SEND(DeviceInfo);
}

void process_msg_WriteSEPrivateKey(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t *buf) {
  MSG_RECV_INIT(WriteSEPrivateKey);
  MSG_RECV(WriteSEPrivateKey);

  if (msg_recv.private_key.size != 32) {
    send_failure(iface_num, FailureType_Failure_ProcessError,
                 "Private key size invalid");
    return;
  }

  if (se_set_private_key_extern(msg_recv.private_key.bytes)) {
    send_success(iface_num, "Write private key success");
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError,
                 "Write private key failed");
  }
}

void process_msg_ReadSEPublicKey(uint8_t iface_num, uint32_t msg_size,
                                 uint8_t *buf) {
  uint8_t pubkey[64] = {0};
  MSG_RECV_INIT(ReadSEPublicKey);
  MSG_RECV(ReadSEPublicKey);

  MSG_SEND_INIT(SEPublicKey);
  if (se_get_pubkey(pubkey)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(public_key, pubkey, 64);
    MSG_SEND(SEPublicKey);
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError,
                 "Get SE pubkey Failed");
  }
}

void process_msg_WriteSEPublicCert(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t *buf) {
  MSG_RECV_INIT(WriteSEPublicCert);
  MSG_RECV(WriteSEPublicCert);

  if (se_write_certificate(msg_recv.public_cert.bytes,
                           msg_recv.public_cert.size)) {
    send_success(iface_num, "Write certificate success");
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError,
                 "Write certificate Failed");
  }
}

void process_msg_ReadSEPublicCert(uint8_t iface_num, uint32_t msg_size,
                                  uint8_t *buf) {
  MSG_RECV_INIT(ReadSEPublicCert);
  MSG_RECV(ReadSEPublicCert);

  uint8_t cert[512] = {0};
  uint16_t cert_len = sizeof(cert);

  MSG_SEND_INIT(SEPublicCert);
  if (se_read_certificate(cert, &cert_len)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(public_cert, cert, cert_len);
    MSG_SEND(SEPublicCert);
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError,
                 "Get certificate failed");
  }
}

void process_msg_SESignMessage(uint8_t iface_num, uint32_t msg_size,
                               uint8_t *buf) {
  MSG_RECV_INIT(SESignMessage);
  MSG_RECV(SESignMessage);

  uint8_t sign[64] = {0};

  MSG_SEND_INIT(SEMessageSignature);

  if (se_sign_message_with_write_key((uint8_t *)msg_recv.message.bytes,
                                     msg_recv.message.size, sign)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(signature, sign, 64);
    MSG_SEND(SEMessageSignature);
    return;
  }

  if (se_sign_message((uint8_t *)msg_recv.message.bytes, msg_recv.message.size,
                      sign)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(signature, sign, 64);
    MSG_SEND(SEMessageSignature);
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError, "SE sign failed");
  }
}
