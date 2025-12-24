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
#include "hardware_version.h"
#include "nordic_dfu.h"
#include "spi_legacy.h"

#define MSG_HEADER1_LEN 9
#define MSG_HEADER2_LEN 1

secbool msg_parse_header(const uint8_t* buf, uint16_t* msg_id,
                         uint32_t* msg_size) {
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
static bool _usb_write(pb_ostream_t* stream, const pb_byte_t* buf,
                       size_t count) {
  usb_write_state* state = (usb_write_state*)(stream->state);

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

static void _usb_write_flush(usb_write_state* state) {
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
                         const pb_msgdesc_t* fields, const void* msg) {
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

typedef struct {
  uint8_t iface_num;
  uint8_t packet_index;
  uint8_t packet_pos;
  uint8_t* buf;
} usb_read_state;

static void _usb_webusb_read_retry(uint8_t iface_num, uint8_t* buf) {
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
static bool _usb_read(pb_istream_t* stream, uint8_t* buf, size_t count) {
  usb_read_state* state = (usb_read_state*)(stream->state);

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

static void _usb_read_flush(usb_read_state* state) { (void)state; }

static secbool _recv_msg(uint8_t iface_num, uint32_t msg_size, uint8_t* buf,
                         const pb_msgdesc_t* fields, void* msg) {
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

static bool process_OneKeyInfo(const OneKeyInfoReq* const req,
                               OneKeyInfoResp* const resp) {
  // message OneKeyInfoResp {

  // required float protocol_version = 10;
  MSG_ASSIGN(MSG_ASSIGN_VAL, resp->protocol_version, 1.0);

  if (MSG_TEST_HAS_P(req, targets) && MSG_TEST_HAS_P(req, types)) {
    if (MSG_TEST_HAS_TRUE(req->targets, hw)) {
      //     optional OneKeyHardwareInfo hw = 100;
      // --->
      // message OneKeyHardwareInfo {
      //     optional OneKeyDeviceType device_type = 10;
      MSG_ASSIGN_HAS(MSG_ASSIGN_VAL, resp->hw, device_type,
                     OneKeyDeviceType_PRO);
      //     optional string serial_no = 11;
      char* serial = NULL;
      if (device_get_serial(&serial))
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->hw, serial_no, serial,
                       strlen(serial));
      //     optional string version = 100;
      MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->hw, hardware_version,
                     hw_ver_to_str(get_hw_ver()), -1);
      //     optional bytes raw_adc = 101;
      MSG_ASSIGN_HAS(MSG_ASSIGN_VAL, resp->hw, hardware_version_raw_adc,
                     get_hw_ver_adc_raw());
      // }
      // <---
      MSG_HAS_P(resp, hw);
    }

    if (MSG_TEST_HAS_TRUE(req->targets, fw)) {
      //     optional OneKeyMainMcuInfo fw = 200;
      // --->
      // message OneKeyMainMcuInfo {
      //     optional OneKeyFwImgInfo board = 10;
      if (MSG_TEST_HAS_TRUE(req->types, version)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.board, version,
                       get_boardloader_version(),
                       strlen(get_boardloader_version()));
      }
      if (MSG_TEST_HAS_TRUE(req->types, build_id)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.board, build_id,
                       get_boardloader_build_id(),
                       strlen(get_boardloader_build_id()));
      }
      if (MSG_TEST_HAS_TRUE(req->types, hash)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, resp->fw.board, hash,
                       get_boardloader_hash(), 32);
      }
      MSG_HAS(resp->fw, board);

      //     optional OneKeyFwImgInfo boot = 20;
      if (MSG_TEST_HAS_TRUE(req->types, version)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.boot, version,
                       (VERSTR(VERSION_MAJOR) "." VERSTR(
                           VERSION_MINOR) "." VERSTR(VERSION_PATCH)),
                       strlen((VERSTR(VERSION_MAJOR) "." VERSTR(
                           VERSION_MINOR) "." VERSTR(VERSION_PATCH))));
      }
      if (MSG_TEST_HAS_TRUE(req->types, build_id)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.boot, build_id,
                       (char*)BUILD_COMMIT, strlen((char*)BUILD_COMMIT));
      }
      if (MSG_TEST_HAS_TRUE(req->types, hash)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, resp->fw.boot, hash,
                       get_bootloader_hash(), 32);
      }
      MSG_HAS(resp->fw, boot);

      //     optional OneKeyFwImgInfo app = 30;
      vendor_header* vhdr = (vendor_header*)FIRMWARE_START;
      image_header* hdr = (image_header*)(FIRMWARE_START + vhdr->hdrlen);
      const char* ver_str = format_ver("%d.%d.%d", hdr->onekey_version);
      if (MSG_TEST_HAS_TRUE(req->types, version)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.app, version, ver_str,
                       strlen(ver_str));
      }
      if (MSG_TEST_HAS_TRUE(req->types, build_id)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->fw.app, build_id,
                       (char*)(hdr->build_id), strlen((char*)(hdr->build_id)));
      }
      if (MSG_TEST_HAS_TRUE(req->types, hash)) {
        MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, resp->fw.app, hash,
                       get_firmware_hash(), 32);
      }
      MSG_HAS(resp->fw, app);

      if (MSG_TEST_HAS_TRUE(req->types, specific)) {
        // N/A
      }
      // }
      // <---
      MSG_HAS_P(resp, fw);
    }

    if (MSG_TEST_HAS_TRUE(req->targets, bt)) {
      //     optional OneKeyBluetoothInfo bt = 300;
      // --->

      ble_get_dev_info();

      // message OneKeyBluetoothInfo {
      //     optional OneKeyFwImgInfo boot = 20;
      // MSG_HAS(resp->bt, boot);
      // Note: not implemented

      //     optional OneKeyFwImgInfo app = 30;
      if (MSG_TEST_HAS_TRUE(req->types, version)) {
        if (ble_ver_state())
          MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->bt.app, version,
                         ble_get_ver(), strlen(ble_get_ver()));
      }
      if (MSG_TEST_HAS_TRUE(req->types, build_id)) {
        if (ble_build_state())
          MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->bt.app, build_id,
                         (char*)ble_get_build(), 8);
      }
      if (MSG_TEST_HAS_TRUE(req->types, hash)) {
        if (ble_hash_state())
          MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, resp->bt.app, hash, ble_get_hash(),
                         32);
      }
      MSG_HAS(resp->bt, app);

      if (MSG_TEST_HAS_TRUE(req->types, specific)) {
        //     optional string adv_name = 100;
        if (ble_name_state())
          MSG_ASSIGN_HAS(MSG_ASSIGN_STRING, resp->bt, adv_name, ble_get_name(),
                         BLE_NAME_LEN);
        //     optional bytes mac = 110;
        if (ble_mac_state())
          MSG_ASSIGN_HAS(MSG_ASSIGN_BYTES, resp->bt, mac, ble_get_mac(), 6);
      }
      // }
      // <---
      MSG_HAS_P(resp, bt);
    }

    if (MSG_TEST_HAS_TRUE(req->targets, se1)) {
      //     optional OneKeySEInfo se1 = 400;
      // --->
      MSG_ASSIGN_SE_INFO(resp->se1, se01);
      MSG_HAS_P(resp, se1);
      // <---
    }

    if (MSG_TEST_HAS_TRUE(req->targets, se2)) {
      //     optional OneKeySEInfo se2 = 410;
      // --->
      MSG_ASSIGN_SE_INFO(resp->se2, se02);
      MSG_HAS_P(resp, se2);
      // <---
    }

    if (MSG_TEST_HAS_TRUE(req->targets, se3)) {
      //     optional OneKeySEInfo se3 = 420;
      // --->
      MSG_ASSIGN_SE_INFO(resp->se3, se03);
      MSG_HAS_P(resp, se3);
      // <---
    }

    if (MSG_TEST_HAS_TRUE(req->targets, se4)) {
      //     optional OneKeySEInfo se4 = 430;
      // --->
      MSG_ASSIGN_SE_INFO(resp->se4, se04);
      MSG_HAS_P(resp, se4);
      // <---
    }
  }
  // }

  return true;
}

void send_success(uint8_t iface_num, const char* text) {
  MSG_SEND_INIT(Success);
  MSG_SEND_ASSIGN_STRING(message, text);
  MSG_SEND(Success);
}

void send_failure(uint8_t iface_num, FailureType type, const char* text) {
  MSG_SEND_INIT(Failure);
  MSG_SEND_ASSIGN_VALUE(code, type);
  MSG_SEND_ASSIGN_STRING(message, text);
  MSG_SEND(Failure);
}

void send_user_abort(uint8_t iface_num, const char* msg) {
  MSG_SEND_INIT(Failure);
  MSG_SEND_ASSIGN_VALUE(code, FailureType_Failure_ActionCancelled);
  MSG_SEND_ASSIGN_STRING(message, msg);
  MSG_SEND(Failure);
}

static void send_msg_features(uint8_t iface_num,
                              const vendor_header* const vhdr,
                              const image_header* const hdr,
                              const OneKeyInfoReq* const inforeq) {
  MSG_SEND_INIT(Features);
  MSG_SEND_ASSIGN_VALUE(bootloader_mode, true);

  // legacy compatibility
  MSG_SEND_ASSIGN_VALUE(onekey_device_type, OneKeyDeviceType_PRO);
  char* serial = NULL;
  if (device_get_serial(&serial))
    MSG_SEND_ASSIGN_STRING(onekey_serial_no, serial);

  if (device_is_factory_mode()) {
    MSG_CONSTRUCT(ok_factory_status, OneKeyFactoryStatus);
    MSG_ASSIGN(MSG_ASSIGN_VAL, ok_factory_status.device_sn_set,
               device_serial_set());
    MSG_ASSIGN(MSG_ASSIGN_VAL, ok_factory_status.se_cert_set,
               se_has_cerrificate());
    MSG_SEND_ASSIGN_VALUE(ok_factory_status, ok_factory_status);
  }

  if ((inforeq != NULL)) {
    MSG_CONSTRUCT(onekey_info_resp, OneKeyInfoResp);
    process_OneKeyInfo(inforeq, &onekey_info_resp);
    MSG_SEND_ASSIGN_VALUE(ok_dev_info_resp, onekey_info_resp);
  }

  MSG_SEND(Features);
}

void process_msg_StartSession(uint8_t iface_num, uint32_t msg_size,
                              uint8_t* buf, const vendor_header* const vhdr,
                              const image_header* const hdr) {
  MSG_RECV_INIT(StartSession);
  MSG_RECV(StartSession);

  if (MSG_TEST_HAS(msg_recv, ok_dev_info_req))
    send_msg_features(iface_num, vhdr, hdr, &(msg_recv.ok_dev_info_req));
  else
    send_msg_features(iface_num, vhdr, hdr, NULL);
}

void process_msg_GetFeatures(uint8_t iface_num, uint32_t msg_size, uint8_t* buf,
                             const vendor_header* const vhdr,
                             const image_header* const hdr) {
  MSG_RECV_INIT(GetFeatures);
  MSG_RECV(GetFeatures);

  if (MSG_TEST_HAS(msg_recv, ok_dev_info_req))
    send_msg_features(iface_num, vhdr, hdr, &(msg_recv.ok_dev_info_req));
  else
    send_msg_features(iface_num, vhdr, hdr, NULL);
}

void process_msg_Ping(uint8_t iface_num, uint32_t msg_size, uint8_t* buf) {
  MSG_RECV_INIT(Ping);
  MSG_RECV(Ping);

  MSG_SEND_INIT(Success);
  MSG_SEND_ASSIGN_STRING(message, msg_recv.message);
  MSG_SEND(Success);
}

void process_msg_OneKeyReboot(uint8_t iface_num, uint32_t msg_size,
                              uint8_t* buf) {
  MSG_RECV_INIT(OneKeyReboot);
  MSG_RECV(OneKeyReboot);

  switch (msg_recv.reboot_type) {
    case OneKeyRebootType_Normal: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type Normal accepted!");
      MSG_SEND(Success);
    }
      hal_delay(100);
      *BOOT_TARGET_FLAG_ADDR = BOOT_TARGET_NORMAL;
      restart();
      break;
    case OneKeyRebootType_Boardloader: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type Boardloader accepted!");
      MSG_SEND(Success);
    }
      hal_delay(100);
      reboot_to_board();
      break;
    case OneKeyRebootType_BootLoader: {
      MSG_SEND_INIT(Success);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type BootLoader accepted!");
      MSG_SEND(Success);
    }
      hal_delay(100);
      reboot_to_boot();
      break;

    default: {
      MSG_SEND_INIT(Failure);
      MSG_SEND_ASSIGN_STRING(message, "Reboot type invalid!");
      MSG_SEND(Failure);
    } break;
  }
}

int process_msg_WipeDevice(uint8_t iface_num, uint32_t msg_size, uint8_t* buf) {
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

void process_msg_unknown(uint8_t iface_num, uint32_t msg_size, uint8_t* buf) {
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
                                    uint8_t* buf) {
  MSG_RECV_INIT(DeviceInfoSettings);
  MSG_RECV(DeviceInfoSettings);

  if (msg_recv.has_serial_no) {
    if (!device_set_serial((char*)msg_recv.serial_no)) {
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
                               uint8_t* buf) {
  MSG_RECV_INIT(GetDeviceInfo);
  MSG_RECV(GetDeviceInfo);

  MSG_SEND_INIT(DeviceInfo);

  char* serial;
  if (device_get_serial(&serial)) {
    MSG_SEND_ASSIGN_STRING(serial_no, serial);
  }
  MSG_SEND(DeviceInfo);
}

void process_msg_WriteSEPrivateKey(uint8_t iface_num, uint32_t msg_size,
                                   uint8_t* buf) {
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
                                 uint8_t* buf) {
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
                                   uint8_t* buf) {
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
                                  uint8_t* buf) {
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
                               uint8_t* buf) {
  MSG_RECV_INIT(SESignMessage);
  MSG_RECV(SESignMessage);

  uint8_t sign[64] = {0};

  MSG_SEND_INIT(SEMessageSignature);

  if (se_sign_message_with_write_key((uint8_t*)msg_recv.message.bytes,
                                     msg_recv.message.size, sign)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(signature, sign, 64);
    MSG_SEND(SEMessageSignature);
    return;
  }

  if (se_sign_message_with_write_key((uint8_t*)msg_recv.message.bytes,
                                     msg_recv.message.size, sign)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(signature, sign, 64);
    MSG_SEND(SEMessageSignature);
    return;
  }

  if (se_sign_message((uint8_t*)msg_recv.message.bytes, msg_recv.message.size,
                      sign)) {
    MSG_SEND_ASSIGN_REQUIRED_BYTES(signature, sign, 64);
    MSG_SEND(SEMessageSignature);
  } else {
    send_failure(iface_num, FailureType_Failure_ProcessError, "SE sign failed");
  }
}
