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

#include <string.h>
#include <sys/types.h>

#include "adc.h"
#include "common.h"
#include "compiler_traits.h"
#include "device.h"
#include "display.h"
#include "flash.h"
#include "fw_keys.h"
#include "hardware_version.h"
#include "image.h"
#include "lowlevel.h"
#include "mini_printf.h"
#include "mipi_lcd.h"
#include "mpu.h"
#include "qspi_flash.h"
#include "random_delays.h"
#include "sdram.h"
#include "se_thd89.h"
#include "secbool.h"
#include "systick.h"
#include "thd89.h"
#include "thd89_boot.h"
#include "touch.h"
#include "usb.h"
#include "usbd_desc.h"
#include "version.h"

#include "ble.h"
#include "bootui.h"
#include "device.h"
#include "i2c.h"
#include "jpeg_dma.h"
#include "messages.h"
#include "motor.h"
#include "spi.h"
#include "spi_legacy.h"
#include "usart.h"

#define MSG_NAME_TO_ID(x) MessageType_MessageType_##x

#if defined(STM32H747xx)
#include "stm32h7xx_hal.h"
#endif

#include "camera.h"
#include "emmc_wrapper.h"

#define USB_IFACE_NUM 0

#if !PRODUCTION

// DO NOT USE THIS UNLESS YOU KNOW WHAT YOU ARE DOING
// Warning: this is for developers to setup a dummy config only!
// configuration to SE is permanent, there is no way to reset it!
static void write_dev_dummy_serial() {
  if (!device_serial_set()) {
    // device_set_serial("TCTestSerialNumberXXXXXXXXXXXXX");
    device_set_serial("PRA50I0000 ES");
  }
}
static void write_dev_dummy_cert() {
  uint8_t dummy_cert[] = {
      0x30, 0x82, 0x01, 0x58, 0x30, 0x82, 0x01, 0x0A, 0xA0, 0x03, 0x02, 0x01,
      0x02, 0x02, 0x08, 0x44, 0x9F, 0x65, 0xB6, 0x90, 0xE4, 0x90, 0x09, 0x30,
      0x05, 0x06, 0x03, 0x2B, 0x65, 0x70, 0x30, 0x36, 0x31, 0x0F, 0x30, 0x0D,
      0x06, 0x03, 0x55, 0x04, 0x0A, 0x13, 0x06, 0x4F, 0x6E, 0x65, 0x4B, 0x65,
      0x79, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x13, 0x02,
      0x4E, 0x41, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C,
      0x0D, 0x4F, 0x4E, 0x45, 0x4B, 0x45, 0x59, 0x5F, 0x44, 0x45, 0x56, 0x5F,
      0x43, 0x41, 0x30, 0x22, 0x18, 0x0F, 0x39, 0x39, 0x39, 0x39, 0x31, 0x32,
      0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35, 0x39, 0x5A, 0x18, 0x0F, 0x39,
      0x39, 0x39, 0x39, 0x31, 0x32, 0x33, 0x31, 0x32, 0x33, 0x35, 0x39, 0x35,
      0x39, 0x5A, 0x30, 0x2A, 0x31, 0x28, 0x30, 0x26, 0x06, 0x03, 0x55, 0x04,
      0x03, 0x13, 0x1F, 0x54, 0x43, 0x54, 0x65, 0x73, 0x74, 0x53, 0x65, 0x72,
      0x69, 0x61, 0x6C, 0x4E, 0x75, 0x6D, 0x62, 0x65, 0x72, 0x58, 0x58, 0x58,
      0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x30, 0x59,
      0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06,
      0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00,
      0x04, 0x20, 0x32, 0xF5, 0xC1, 0x3B, 0x55, 0x5C, 0x8B, 0xF7, 0xE0, 0xB4,
      0x8A, 0x83, 0x5C, 0x67, 0xD3, 0xC2, 0x04, 0xB7, 0x90, 0x2F, 0x49, 0x78,
      0xF8, 0x5D, 0x2B, 0xFE, 0xA1, 0xAF, 0x0B, 0xCA, 0x6F, 0x94, 0xD3, 0x20,
      0xD9, 0x04, 0x5B, 0xD7, 0x0B, 0xB2, 0x8D, 0xA7, 0xF1, 0x8D, 0x39, 0xA9,
      0xC5, 0x44, 0x53, 0x67, 0x5C, 0xA9, 0x6D, 0x5F, 0x45, 0x74, 0x77, 0x32,
      0x38, 0x8D, 0x91, 0x5F, 0xE2, 0xA3, 0x0F, 0x30, 0x0D, 0x30, 0x0B, 0x06,
      0x03, 0x55, 0x1D, 0x0F, 0x04, 0x04, 0x03, 0x02, 0x07, 0x80, 0x30, 0x05,
      0x06, 0x03, 0x2B, 0x65, 0x70, 0x03, 0x41, 0x00, 0x9F, 0x5D, 0x95, 0xFB,
      0x4A, 0xAD, 0xE6, 0xC6, 0x3B, 0x8E, 0x15, 0xB0, 0xBD, 0x0D, 0xF0, 0x70,
      0x81, 0x4E, 0x05, 0x9A, 0xAD, 0xC4, 0xE4, 0x6E, 0x44, 0xDE, 0xF1, 0xDB,
      0x51, 0xCB, 0x85, 0xB7, 0x5F, 0xAF, 0x55, 0xEB, 0x28, 0x9A, 0x66, 0x95,
      0xAA, 0x08, 0x66, 0x8E, 0x84, 0xC1, 0x22, 0x5D, 0x34, 0x75, 0xF3, 0x01,
      0x2F, 0x6D, 0x33, 0x21, 0x35, 0x1E, 0x54, 0xEC, 0x71, 0xEC, 0x3D, 0x04};
  UNUSED(dummy_cert);

  if (!se_has_cerrificate()) {
    if (!se_write_certificate(dummy_cert, sizeof(dummy_cert)))
      ensure(secfalse, "set cert failed");
  }
}

#endif

// this is mainly for ignore/supress faults during flash read (for check
// purpose). if bus fault enabled, it will catched by BusFault_Handler, then we
// could ignore it. if bus fault disabled, it will elevate to hard fault, this
// is not what we want
static secbool handle_flash_ecc_error = secfalse;
static void set_handle_flash_ecc_error(secbool val) {
  handle_flash_ecc_error = val;
}

// fault handlers
void HardFault_Handler(void) {
  error_shutdown("Internal error", "(HF)", NULL, NULL);
}

void MemManage_Handler_MM(void) {
  error_shutdown("Internal error", "(MM)", NULL, NULL);
}

void MemManage_Handler_SO(void) {
  error_shutdown("Internal error", "(SO)", NULL, NULL);
}

void BusFault_Handler(void) {
  // if want handle flash ecc error
  if (handle_flash_ecc_error == sectrue) {
    // dbgprintf_Wait("Internal flash ECC error detected at 0x%X", SCB->BFAR);

    // check if it's triggered by flash DECC
    if (flash_check_ecc_fault()) {
      // reset flash controller error flags
      flash_clear_ecc_fault(SCB->BFAR);

      // reset bus fault error flags
      SCB->CFSR &= ~(SCB_CFSR_BFARVALID_Msk | SCB_CFSR_PRECISERR_Msk);
      __DSB();
      SCB->SHCSR &= ~(SCB_SHCSR_BUSFAULTACT_Msk);
      __DSB();

      // try to fix ecc error and reboot
      if (flash_fix_ecc_fault_FIRMWARE(SCB->BFAR)) {
        error_shutdown("Internal flash ECC error", "Cleanup successful",
                       "Firmware reinstall may required",
                       "If the issue persists, contact support.");
      } else {
        error_shutdown("Internal flash ECC error", "Cleanup failed",
                       "Reboot to try again",
                       "If the issue persists, contact support.");
      }
    }
  }

  // normal route
  error_shutdown("Internal error", "(BF)", NULL, NULL);
}

void UsageFault_Handler(void) {
  error_shutdown("Internal error", "(UF)", NULL, NULL);
}

static secbool get_device_serial(char* serial, size_t len) {
  // init
  uint8_t otp_serial[FLASH_OTP_BLOCK_SIZE] = {0};
  memzero(otp_serial, sizeof(otp_serial));
  memzero(serial, len);

  // get OTP serial
  if (sectrue != flash_otp_is_locked(FLASH_OTP_DEVICE_SERIAL)) return secfalse;

  if (sectrue != flash_otp_read(FLASH_OTP_DEVICE_SERIAL, 0, otp_serial,
                                sizeof(otp_serial))) {
    return secfalse;
  }

  // make sure last element is '\0'
  otp_serial[FLASH_OTP_BLOCK_SIZE - 1] = '\0';

  // check if all is ascii
  for (uint32_t i = 0; i < sizeof(otp_serial); i++) {
    if (otp_serial[i] == '\0') {
      break;
    }
    if (otp_serial[i] < ' ' || otp_serial[i] > '~') {
      return secfalse;
    }
  }

  // copy to output buffer
  memcpy(serial, otp_serial, MIN(len, sizeof(otp_serial)));

  // cutoff by strlen
  serial[strlen(serial)] = '\0';

  return sectrue;
}

static void usb_init_all(secbool usb21_landing) {
  usb_dev_info_t dev_info = {
      .device_class = 0x00,
      .device_subclass = 0x00,
      .device_protocol = 0x00,
      .vendor_id = 0x1209,
      .product_id = 0x4F4A,
      .release_num = 0x0200,
      .manufacturer = "OneKey Limited",
      .product = "OneKey Pro",
      .serial_number = "000000000000000000000000",
      .interface = "Bootloader Interface",
      .usb21_enabled = sectrue,
      .usb21_landing = usb21_landing,
  };

  static char serial[USB_SIZ_STRING_SERIAL];

  if (sectrue == get_device_serial(serial, sizeof(serial))) {
    dev_info.serial_number = serial;
  }

  static uint8_t rx_buffer[USB_PACKET_SIZE];

  static const usb_webusb_info_t webusb_info = {
      .iface_num = USB_IFACE_NUM,
      .ep_in = USB_EP_DIR_IN | 0x01,
      .ep_out = USB_EP_DIR_OUT | 0x01,
      .subclass = 0,
      .protocol = 0,
      .max_packet_len = sizeof(rx_buffer),
      .rx_buffer = rx_buffer,
      .polling_interval = 1,
  };

  usb_init(&dev_info);

  ensure(usb_webusb_add(&webusb_info), NULL);

  // usb start after vbus connected
  // usb_start();
}

static void usb_switch(void) {
  static bool usb_opened = false;
  static uint32_t counter0 = 0, counter1 = 0;

  if (usb_3320_host_connected()) {
    counter0++;
    counter1 = 0;
    if (counter0 > 5) {
      counter0 = 0;
      if (!usb_opened) {
        usb_start();
        usb_opened = true;
      }
    }
  } else {
    counter0 = 0;
    counter1++;
    if (counter1 > 5) {
      counter1 = 0;
      if (usb_opened) {
        usb_stop();
        usb_opened = false;
      }
    }
  }
}

static void charge_switch(void) {
  static bool charge_configured = false;
  static bool charge_enabled = false;

  if (!ble_charging_state()) {
    ble_cmd_req(BLE_PWR, BLE_PWR_CHARGING);
    return;
  }

  if (ble_get_charge_type() == CHARGE_BY_USB) {
    if (!charge_enabled || !charge_configured) {
      charge_configured = true;
      charge_enabled = true;
      ble_cmd_req(BLE_PWR, BLE_PWR_CHARGE_ENABLE);
    }
  } else {
    if (charge_enabled || !charge_configured) {
      charge_configured = true;
      charge_enabled = false;
      ble_cmd_req(BLE_PWR, BLE_PWR_CHARGE_DISABLE);
    }
  }
}

static secbool bootloader_usb_loop(const vendor_header* const vhdr,
                                   const image_header* const hdr) {
  // if both are NULL, we don't have a firmware installed
  // let's show a webusb landing page in this case
  usb_init_all((vhdr == NULL && hdr == NULL) ? sectrue : secfalse);

  uint8_t buf[USB_PACKET_SIZE];
  int r;

  for (;;) {
    while (true) {
      ble_uart_poll();

      usb_switch();

      charge_switch();

      // check bluetooth
      if (USB_PACKET_SIZE == spi_slave_poll(buf)) {
        host_channel = CHANNEL_SLAVE;
        break;
      }
      // check usb
      else if (USB_PACKET_SIZE == usb_webusb_read_blocking(
                                      USB_IFACE_NUM, buf, USB_PACKET_SIZE, 5)) {
        host_channel = CHANNEL_USB;
        break;
      }
      // no packet, check if power button pressed
      // else if ( ble_power_button_state() == 1 ) // short press
      else if (ble_power_button_state() == 2)  // long press
      {
        // give a way to go back to bootloader home page
        if (get_ui_bootloader_page_current() != 0) {
          ble_power_button_state_clear();
          ui_progress_bar_visible_clear();
          ui_fadeout();
          ui_bootloader_first(NULL);
          ui_fadein();
        }
        memzero(buf, USB_PACKET_SIZE);
        continue;
      }
      // no packet, no pwer button pressed
      else {
        ui_bootloader_page_switch(hdr);
        static uint32_t tickstart = 0;
        if ((HAL_GetTick() - tickstart) >= 1000) {
          ui_statusbar_update();
          tickstart = HAL_GetTick();
        }
        continue;
      }
    }

    uint16_t msg_id;
    uint32_t msg_size;
    if (sectrue != msg_parse_header(buf, &msg_id, &msg_size)) {
      // invalid header -> discard
      continue;
    }

    switch (msg_id) {
      case MSG_NAME_TO_ID(Initialize):  // Initialize
        process_msg_Initialize(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
        break;
      case MSG_NAME_TO_ID(Ping):  // Ping
        process_msg_Ping(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(WipeDevice):  // WipeDevice
        ui_fadeout();
        ui_wipe_confirm(hdr);
        ui_fadein();
        int response = ui_input_poll(INPUT_CONFIRM | INPUT_CANCEL, true);
        if (INPUT_CANCEL == response) {
          ui_fadeout();
          ui_bootloader_first(hdr);
          ui_fadein();
          send_user_abort(USB_IFACE_NUM, "Wipe cancelled");
          break;
        }
        ui_fadeout();
        ui_screen_wipe();
        ui_fadein();
        r = process_msg_WipeDevice(USB_IFACE_NUM, msg_size, buf);
        if (r < 0) {  // error
          ui_fadeout();
          ui_screen_fail();
          ui_fadein();
          usb_stop();
          usb_deinit();
          while (!touch_click()) {
          }
          restart();
          return secfalse;  // shutdown
        } else {            // success
          ui_fadeout();
          ui_screen_wipe_done();
          ui_fadein();
          usb_stop();
          usb_deinit();
          while (!ui_input_poll(INPUT_NEXT, true)) {
          }
          restart();
          return secfalse;  // shutdown
        }
        break;
      case MSG_NAME_TO_ID(GetFeatures):  // GetFeatures
        process_msg_GetFeatures(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
        break;
      case MSG_NAME_TO_ID(Reboot):  // Reboot
        process_msg_Reboot(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(FirmwareUpdateEmmc):  // FirmwareUpdateEmmc
        r = process_msg_FirmwareUpdateEmmc(USB_IFACE_NUM, msg_size, buf);
        if (r < 0 && r != -4) {  // error
          ui_fadeout();
          ui_screen_fail();
          ui_fadein();
          while (!touch_click()) {
            hal_delay(10);
          }
          bluetooth_reset();
          // make sure we have latest bluetooth status (and wait for bluetooth
          // become ready)
          ble_refresh_dev_info();
          reboot_to_boot();
          return secfalse;  // shutdown
        }
        break;
      case MSG_NAME_TO_ID(EmmcFixPermission):  // EmmcFixPermission
        process_msg_EmmcFixPermission(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcPathInfo):  // EmmcPathInfo
        process_msg_EmmcPathInfo(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileRead):  // EmmcFileRead
        process_msg_EmmcFileRead(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileWrite):  // EmmcFileWrite
        process_msg_EmmcFileWrite(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileDelete):  // EmmcFileDelete
        process_msg_EmmcFileDelete(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirList):  // EmmcDirList
        process_msg_EmmcDirList(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirMake):  // EmmcDirMake
        process_msg_EmmcDirMake(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirRemove):  // EmmcDirRemove
        process_msg_EmmcDirRemove(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(OnekeyGetFeatures):  // OnekeyGetFeatures
        process_msg_OnekeyGetFeatures(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
        break;
      default:
        process_msg_unknown(USB_IFACE_NUM, msg_size, buf);
        break;
    }
  }
}

secbool bootloader_usb_loop_factory(const vendor_header* const vhdr,
                                    const image_header* const hdr) {
  // if both are NULL, we don't have a firmware installed
  // let's show a webusb landing page in this case
  usb_init_all((vhdr == NULL && hdr == NULL) ? sectrue : secfalse);

  usb_start();

  uint8_t buf[USB_PACKET_SIZE];
  int r;

  for (;;) {
    r = usb_webusb_read_blocking(USB_IFACE_NUM, buf, USB_PACKET_SIZE,
                                 USB_TIMEOUT);
    if (r != USB_PACKET_SIZE) {
      continue;
    }
    host_channel = CHANNEL_USB;

    uint16_t msg_id;
    uint32_t msg_size;
    if (sectrue != msg_parse_header(buf, &msg_id, &msg_size)) {
      // invalid header -> discard
      continue;
    }

    switch (msg_id) {
      case MSG_NAME_TO_ID(Initialize):  // Initialize
        process_msg_Initialize(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
        break;
      case MSG_NAME_TO_ID(Ping):  // Ping
        process_msg_Ping(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(GetFeatures):  // GetFeatures
        process_msg_GetFeatures(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
        break;
      case MSG_NAME_TO_ID(DeviceInfoSettings):  // DeviceInfoSettings
        process_msg_DeviceInfoSettings(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(GetDeviceInfo):  // GetDeviceInfo
        process_msg_GetDeviceInfo(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(WriteSEPrivateKey):  // WriteSEPrivateKey
        process_msg_WriteSEPrivateKey(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(ReadSEPublicKey):  // ReadSEPublicKey
        process_msg_ReadSEPublicKey(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(WriteSEPublicCert):  // WriteSEPublicCert
        process_msg_WriteSEPublicCert(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(ReadSEPublicCert):  // ReadSEPublicCert
        process_msg_ReadSEPublicCert(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(SESignMessage):  // SESignMessage
        process_msg_SESignMessage(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(Reboot):  // Reboot
        process_msg_Reboot(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(FirmwareUpdateEmmc):  // FirmwareUpdateEmmc
        process_msg_FirmwareUpdateEmmc(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFixPermission):  // EmmcFixPermission
        process_msg_EmmcFixPermission(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcPathInfo):  // EmmcPathInfo
        process_msg_EmmcPathInfo(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileRead):  // EmmcFileRead
        process_msg_EmmcFileRead(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileWrite):  // EmmcFileWrite
        process_msg_EmmcFileWrite(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcFileDelete):  // EmmcFileDelete
        process_msg_EmmcFileDelete(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirList):  // EmmcDirList
        process_msg_EmmcDirList(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirMake):  // EmmcDirMake
        process_msg_EmmcDirMake(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcDirRemove):  // EmmcDirRemove
        process_msg_EmmcDirRemove(USB_IFACE_NUM, msg_size, buf);
        break;
      default:
        process_msg_unknown(USB_IFACE_NUM, msg_size, buf);
        break;
    }
  }
  return sectrue;
}

#if PRODUCTION

// protection against bootloader downgrade
static void check_bootloader_version(void) {
  uint8_t bits[FLASH_OTP_BLOCK_SIZE];
  for (int i = 0; i < FLASH_OTP_BLOCK_SIZE * 8; i++) {
    if (i < VERSION_MONOTONIC) {
      bits[i / 8] &= ~(1 << (7 - (i % 8)));
    } else {
      bits[i / 8] |= (1 << (7 - (i % 8)));
    }
  }
  ensure(flash_otp_write(FLASH_OTP_BLOCK_BOOTLOADER_VERSION, 0, bits,
                         FLASH_OTP_BLOCK_SIZE),
         NULL);

  uint8_t bits2[FLASH_OTP_BLOCK_SIZE];
  ensure(flash_otp_read(FLASH_OTP_BLOCK_BOOTLOADER_VERSION, 0, bits2,
                        FLASH_OTP_BLOCK_SIZE),
         NULL);

  ensure(sectrue * (0 == memcmp(bits, bits2, FLASH_OTP_BLOCK_SIZE)),
         "Bootloader downgraded");
}

#endif

static BOOT_TARGET decide_boot_target(vendor_header* const vhdr,
                                      image_header* const hdr,
                                      secbool* vhdr_valid, secbool* hdr_valid,
                                      secbool* fw_valid) {
  // get boot target flag
  BOOT_TARGET boot_target = *BOOT_TARGET_FLAG_ADDR;  // cache flag
  *BOOT_TARGET_FLAG_ADDR = BOOT_TARGET_NORMAL;       // consume(reset) flag

  // if boot target already set to this level, no more checks
  if (boot_target == BOOT_TARGET_BOOTLOADER) return boot_target;

  // check se status
  if (se_get_state() != 0) {
    boot_target = BOOT_TARGET_BOOTLOADER;
    return boot_target;
  }

  // check bluetooth state
  if (bluetooth_detect_dfu()) {
    boot_target = BOOT_TARGET_BOOTLOADER;
    return boot_target;
  }

  // check firmware
  set_handle_flash_ecc_error(sectrue);
  char err_msg[64];
  secbool all_good = verify_firmware(vhdr, hdr, vhdr_valid, hdr_valid, fw_valid,
                                     err_msg, sizeof(err_msg));

  set_handle_flash_ecc_error(secfalse);

  if (all_good != sectrue) {
    boot_target = BOOT_TARGET_BOOTLOADER;
    return boot_target;
  }

  // all check passed, manual set, since default ram value will be random
  boot_target = BOOT_TARGET_NORMAL;

  return boot_target;
}

int main(void) {
  SystemCoreClockUpdate();
  dwt_init();
  mpu_config_bootloader();

  lcd_ltdc_dsi_disable();
  sdram_reinit();
  // lcd_para_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  lcd_ltdc_dsi_enable();
  lcd_pwm_init();
  touch_init();

  adc_init();

  // keep the screen but cover the boot bar
  // display_clear();
  display_bar_radius(160, 352, 160, 4, COLOR_BLACK, COLOR_BLACK, 2);

  // fault handler
  bus_fault_enable();  // it's here since requires user interface

  // storages
  ensure_emmcfs(emmc_fs_init(), "emmc_fs_init");
  ensure_emmcfs(emmc_fs_mount(true, false), "emmc_fs_mount");
  if (get_hw_ver() < HW_VER_3P0A) {
    qspi_flash_init();
    qspi_flash_config();
    qspi_flash_memory_mapped();
  }

  // bt/pm
  ble_usart_init();
  spi_slave_init();
  ble_reset();

  // misc/feedback
  random_delays_init();

  // as they using same i2c bus, both needs to be powered up before any
  // communication
  camera_io_init();
  thd89_io_init();

  // se
  thd89_reset();
  thd89_init();

  uint8_t se_mode = se_get_state();
  // all se in app mode
  if (se_mode == 0) {
    device_para_init();
  }

  if ((!device_serial_set() || !se_has_cerrificate()) && se_mode == 0) {
    display_clear();
    device_set_factory_mode(true);
    ui_bootloader_factory();
    if (bootloader_usb_loop_factory(NULL, NULL) != sectrue) {
      return 1;
    }
  }

#if !PRODUCTION

  // if (!device_serial_set()) {
  //   write_dev_dummy_serial();
  // }
  UNUSED(write_dev_dummy_serial);

  // if (!se_has_cerrificate()) {
  //   write_dev_dummy_cert();
  // }
  UNUSED(write_dev_dummy_cert);

  // if(!device_overwrite_serial("PRA50I0000 QA"))
  // {
  //   dbgprintf_Wait("serial overwrite failed!");
  // }

  // device_test(true);

  device_backup_otp(false);
  // device_restore_otp();

#endif

#if PRODUCTION

  // check bootloader downgrade
  check_bootloader_version();

#endif

  vendor_header vhdr;
  image_header hdr;
  secbool vhdr_valid = secfalse;
  secbool hdr_valid = secfalse;
  secbool fw_valid = secfalse;

  BOOT_TARGET boot_target =
      decide_boot_target(&vhdr, &hdr, &vhdr_valid, &hdr_valid, &fw_valid);
  // boot_target = BOOT_TARGET_BOOTLOADER;

  if (boot_target == BOOT_TARGET_BOOTLOADER) {
    display_clear();

    if (sectrue == vhdr_valid && sectrue == hdr_valid) {
      ui_bootloader_first(&hdr);
      if (bootloader_usb_loop(&vhdr, &hdr) != sectrue) {
        return 1;
      }
    } else {
      ui_bootloader_first(NULL);
      if (bootloader_usb_loop(NULL, NULL) != sectrue) {
        return 1;
      }
    }
  } else if (boot_target == BOOT_TARGET_NORMAL) {
    // check bluetooth key
    device_verify_ble();

    // if all VTRUST flags are unset = ultimate trust => skip the procedure
    if ((vhdr.vtrust & VTRUST_ALL) != VTRUST_ALL) {
      // ui_fadeout();  // no fadeout - we start from black screen
      ui_screen_boot(&vhdr, &hdr);
      ui_fadein();

      int delay = (vhdr.vtrust & VTRUST_WAIT) ^ VTRUST_WAIT;
      if (delay > 1) {
        while (delay > 0) {
          ui_screen_boot_wait(delay);
          hal_delay(1000);
          delay--;
        }
      } else if (delay == 1) {
        hal_delay(1000);
      }

      if ((vhdr.vtrust & VTRUST_CLICK) == 0) {
        ui_screen_boot_click();
        while (touch_read() == 0)
          ;
      }
    }

    display_clear();

    bus_fault_disable();

    mpu_config_off();

    jump_to(FIRMWARE_START + vhdr.hdrlen + hdr.hdrlen);
  }

  error_shutdown("Internal error", "Boot target invalid", "Tap to restart.",
                 "If the issue persists, contact support.");
  return -1;
}
