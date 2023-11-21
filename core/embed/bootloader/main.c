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

#include "common.h"
#include "compiler_traits.h"
#include "device.h"
#include "display.h"
#include "flash.h"
#include "image.h"
#include "mini_printf.h"
#include "mipi_lcd.h"
#include "mpu.h"
#include "nand_flash.h"
#include "qspi_flash.h"
#include "random_delays.h"
#include "se_thd89.h"
#include "secbool.h"
#include "thd89.h"
#include "thd89_boot.h"
#ifdef TREZOR_MODEL_T
#include "touch.h"
#endif
#if defined TREZOR_MODEL_R
#include "button.h"
#include "rgb_led.h"
#endif
#include "usb.h"
#include "version.h"

#include "ble.h"
#include "bootui.h"
#include "device.h"
#include "i2c.h"
#include "messages.h"
#include "mpu.h"
#include "spi.h"
#include "spi_legacy.h"
#include "sys.h"
#include "usart.h"

#define MSG_NAME_TO_ID(x) MessageType_MessageType_##x

#if defined(STM32H747xx)
#include "stm32h7xx_hal.h"
#endif

#include "camera.h"
#include "emmc_wrapper.h"
#include "fp_sensor_wrapper.h"
#include "nfc.h"

#if PRODUCTION
const uint8_t BOOTLOADER_KEY_M = 4;
const uint8_t BOOTLOADER_KEY_N = 7;
#else
const uint8_t BOOTLOADER_KEY_M = 2;
const uint8_t BOOTLOADER_KEY_N = 3;
#endif

const uint8_t * const BOOTLOADER_KEYS[] = {
#if PRODUCTION
    (const uint8_t *)"\x15\x4b\x8a\xb2\x61\xcc\x88\x79\x48\x3f\x68\x9a\x2d\x41\x24\x3a\xe7\xdb\xc4\x02\x16\x72\xbb\xd2\x5c\x33\x8a\xe8\x4d\x93\x11\x54",
    (const uint8_t *)"\xa9\xe6\x5e\x07\xfe\x6d\x39\xa8\xa8\x4e\x11\xa9\x96\xa0\x28\x3f\x88\x1e\x17\x5c\xba\x60\x2e\xb5\xac\x44\x2f\xb7\x5b\x39\xe8\xe0",
    (const uint8_t *)"\x6c\x88\x05\xab\xb2\xdf\x9d\x36\x79\xf1\xd2\x8a\x40\xcd\x99\x03\x99\xb9\x9f\xc3\xee\x4e\x06\x57\xd8\x1d\x38\x1e\xa1\x48\x8a\x12",
    (const uint8_t *)"\x3e\xd7\x97\x79\x06\x4d\x56\x57\x1b\x29\xbc\xaa\x73\x4c\xbb\x6d\xb6\x1d\x2e\x62\x65\x66\x62\x8e\xcf\x4c\x89\xe1\xdb\x45\xea\xec",
    (const uint8_t *)"\x54\xa4\x06\x33\xbf\xd9\xe6\x0b\x8a\x39\x12\x65\xb2\xe0\x06\x37\x4a\xbe\x63\x1d\x1e\x11\x07\x33\x2b\xca\x56\xbf\x9f\x8c\x5c\x99",
    (const uint8_t *)"\x4b\x71\x13\x4f\x18\xe0\x07\x87\xc5\x83\xd4\x07\x42\xcc\x18\x8e\x17\xfc\x85\xad\xe4\xcb\x47\x2d\xae\x5e\xf8\xe0\x69\xf0\xfe\xc5",
    (const uint8_t *)"\x2e\xcf\x80\xc8\x2b\x44\x98\x48\xc0\x00\x33\x50\x92\x13\x95\x51\xbf\xe4\x7b\x3c\x73\x17\xb4\x99\x50\xf6\x5e\x1d\x82\x43\x20\x24",
#else
    (const uint8_t *)"\x57\x11\x4f\x0a\xa6\x69\xd2\xf8\x37\xe0\x40\xab\x9b\xb5\x1c\x00\x99\x12\x09\xf8\x4b\xfd\x7b\xf0\xf8\x93\x67\x62\x46\xfb\xa2\x4a",
    (const uint8_t *)"\xdc\xae\x8e\x37\xdf\x5c\x24\x60\x27\xc0\x3a\xa9\x51\xbd\x6e\xc6\xca\xa7\xad\x32\xc1\x66\xb1\xf5\x48\xa4\xef\xcd\x88\xca\x3c\xa5",
    (const uint8_t *)"\x77\x29\x12\xab\x61\xd1\xdc\x4f\x91\x33\x32\x5e\x57\xe1\x46\xab\x9f\xac\x17\xa4\x57\x2c\x6f\xcd\xf3\x55\xf8\x00\x36\x10\x00\x04",
#endif
};

#define USB_IFACE_NUM 0

#if !PRODUCTION

static void camera_test() {
  display_printf("TouchPro Demo Mode\n");
  display_printf("======================\n\n");
  display_printf("GC2145 Init...");
  dbgprintf_Wait("%d, %s", __LINE__, (camera_init() == 0) ? "success" : "fail");

  unsigned int tcnt = 0;
  unsigned short* tbuf = (unsigned short*)0xD0200000;
  for (tcnt = 0; tcnt < (480 * 800); tcnt++) *(tbuf + tcnt) = COLOR_BLACK;
  dma2d_copy_buffer((uint32_t*)tbuf, (uint32_t*)DISPLAY_MEMORY_BASE, 0, 0, 480,
                    800);
  dbgprintf_Wait("%d", __LINE__);
  if (camera_is_online()) {
    display_printf("GC2145 Online @ 0x%x  ID: 0x%x\n", GC2145_ADDR,
                   camera_get_id());
  } else
    display_printf("GC2145 Offline!\n");
  display_printf("\nOutput: %d x %d", WIN_W, WIN_H);

  unsigned char camera_err = 0;

  unsigned short CameraFrameCnt_last = 0;

#define SHOW_PRINT_FPS 0
#if SHOW_PRINT_FPS
  unsigned int time_tick = 0;
  unsigned char fps;
  unsigned short CFC_last = 0;
#endif
  while (1) {
#if SHOW_PRINT_FPS
    if ((HAL_GetTick() - time_tick) > 900) {
      fps = CameraFrameCnt - CFC_last;
      CFC_last = CameraFrameCnt;
      time_tick = HAL_GetTick();
      display_printf("Frame= %d\n", fps);
      HAL_Delay(100);
    }
#endif
    if (CameraFrameCnt != CameraFrameCnt_last) {
      dma2d_copy_buffer((uint32_t*)cam_buf, (uint32_t*)DISPLAY_MEMORY_BASE, 0,
                        320, WIN_W, WIN_H);
      CameraFrameCnt_last = CameraFrameCnt;
    }
    if ((camera_err == 0) && ((unsigned short)dcmi_get_error())) {
      camera_err = 1;
      display_printf("\nstate=%d  err=%d\n", dcmi_get_state(),
                     (unsigned short)dcmi_get_error());
      fb_fill_rect(15, 762, 150, 21, COLOR_RED);
      display_text(15, 780, "camera err!", 11, FONT_NORMAL, COLOR_WHITE,
                   COLOR_RED);
    }
  }
}

static void nfc_test() {
  display_printf("TouchPro Demo Mode\n");
  display_printf("======================\n\n");

  display_printf("NFC PN532 Library Init...");
  nfc_init();
  display_printf("Done\n");

  pn532->PowerOn();

  PN532_FW_VER fw_ver;
  display_printf("NFC PN532 Get FW Ver...");
  if (!pn532->GetFirmwareVersion(&fw_ver)) {
    display_printf("Fail\n");
    while (true)
      ;  // die here
  }
  display_printf("Success\n");
  display_printf("IC:0x%02X, Ver:0x%02X, Rev:0x%02X, Support:0x%02X \n",
                 fw_ver.IC, fw_ver.Ver, fw_ver.Rev, fw_ver.Support);

  display_printf("NFC PN532 Config...");
  if (!pn532->SAMConfiguration(PN532_SAM_Normal, 0x14, true)) {
    display_printf("Fail\n");
    while (true)
      ;  // die here
  }
  display_printf("Success\n");

  // card cmd define
  uint8_t capdu_getSN[] = {0x80, 0xcb, 0x80, 0x00, 0x05,
                           0xdf, 0xff, 0x02, 0x81, 0x01};
  // uint8_t capdu_getBackupState[] = {0x80, 0x6a, 0x00, 0x00};
  // uint8_t capdu_getPINState[] = {0x80, 0xcb, 0x80, 0x00, 0x05, 0xdf, 0xff,
  // 0x02, 0x81, 0x05};

  // InListPassiveTarget
  PN532_InListPassiveTarget_Params ILPT_params = {
      .MaxTg = 1,
      .BrTy = PN532_InListPassiveTarget_BrTy_106k_typeA,
      .InitiatorData_len = 0,
  };
  PN532_InListPassiveTarget_Results ILPT_result = {0};

  // InDataExchange
  uint8_t InDataExchange_status = 0xff;
  uint8_t buf_rapdu[PN532_InDataExchange_BUFF_SIZE];
  uint16_t len_rapdu = PN532_InDataExchange_BUFF_SIZE;

  while (true) {
    // detect card
    display_printf("Checking for card...");
    if (pn532->InListPassiveTarget(ILPT_params, &ILPT_result) &&
        ILPT_result.NbTg == 1) {
      display_printf("Detected\n");
      if (pn532->InDataExchange(1, capdu_getSN, sizeof(capdu_getSN),
                                &InDataExchange_status, buf_rapdu,
                                &len_rapdu)) {
        display_printf("DataExchanging...");
        if (InDataExchange_status == 0x00) {
          display_printf("Success\n");
          display_printf("CardSN: %s\n", (char*)buf_rapdu);
          print_buffer(buf_rapdu, len_rapdu);
        } else {
          display_printf("Fail\n");
        }
      }
      break;
    } else {
      display_printf("LS Timeout\n");
    }

    hal_delay(300);
  }

  while (true)
    ;
}

// static void fp_display_image(uint8_t *pu8ImageBuf)
// {
// }

static void fp_test() {
  display_printf("TouchPro Demo Mode\n");
  display_printf("======================\n\n");
  char fpver[32];
  FpLibVersion(fpver);
  display_printf("FP Lib - %s\n", fpver);
  display_printf("FP Init...");

  ExecuteCheck_ADV_FP(fpsensor_gpio_init(), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(fpsensor_spi_init(), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(fpsensor_hard_reset(), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(fpsensor_init(), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(fpsensor_adc_init(12, 12, 16, 3), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(fpsensor_set_config_param(0xC0, 8), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });
  ExecuteCheck_ADV_FP(FpAlgorithmInit(TEMPLATE_ADDR_START), FPSENSOR_OK,
                      { dbgprintf_Wait("err=%d", FP_ret); });

  display_printf("done\n");

  uint8_t image_data[88 * 112 + 2];
  memzero(image_data, sizeof(image_data));
  uint32_t wrote_bytes = 0;

  while (wrote_bytes == 0) {
    if (FpsDetectFinger() == 1) {
      display_printf("finger detected\n");

      switch (FpsGetImageData(image_data)) {
        case 0:
          if (emmc_fs_file_write("0:imagedata.bin", 0, image_data, 88 * 112 + 2,
                                 &wrote_bytes, true, false)) {
            display_printf("image write success\n");
          } else {
            display_printf("image write fail\n");
          }
          break;
        case 1:
          display_printf("no fingerprint\n");
          break;
        case 2:
          display_printf("fingerprint too small\n");
          break;
        default:
          display_printf("unknow error\n");
          break;
      }
    }

    fpsensor_delay_ms(100);
  }

  dbgprintf("wrote %lu bytes", wrote_bytes);
  dbgprintf_Wait("fp_test exiting...");
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
static void bus_fault_enable() { SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk; }
static void bus_fault_disable() { SCB->SHCSR &= ~SCB_SHCSR_BUSFAULTENA_Msk; }

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
        error_shutdown("Internal error", "Cleanup failed",
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
static void usb_init_all(secbool usb21_landing) {
  usb_dev_info_t dev_info = {
      .device_class = 0x00,
      .device_subclass = 0x00,
      .device_protocol = 0x00,
      .vendor_id = 0x1209,
      .product_id = 0x4F4A,
      .release_num = 0x0200,
      .manufacturer = "OneKey",
      .product = "ONEKEY Touch Boot",
      .serial_number = "000000000000000000000000",
      .interface = "ONEKEY Interface",
      .usb21_enabled = sectrue,
      .usb21_landing = usb21_landing,
  };

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

  usb_start();
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

      // check usb
      if (USB_PACKET_SIZE == spi_slave_poll(buf)) {
        host_channel = CHANNEL_SLAVE;
        break;
      }
      // check bluetooth
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
        ble_power_button_state_clear();
        ui_progress_bar_visible_clear();
        ui_fadeout();
        ui_bootloader_first(NULL);
        ui_fadein();
        memzero(buf, USB_PACKET_SIZE);
        continue;
      }
      // no packet, no pwer button pressed
      else {
        ui_bootloader_page_switch(hdr);
        static uint32_t tickstart = 0;
        if ((HAL_GetTick() - tickstart) >= 1000) {
          ui_title_update();
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
#if PRODUCTION_MODEL == 'H'
        ui_wipe_confirm(hdr);
#else
        ui_screen_wipe_confirm();
#endif
        ui_fadein();
#if PRODUCTION_MODEL == 'H'
        int response = ui_input_poll(INPUT_CONFIRM | INPUT_CANCEL, true);
#else
        int response = ui_user_input(INPUT_CONFIRM | INPUT_CANCEL);
#endif
        if (INPUT_CANCEL == response) {
          ui_fadeout();
#if PRODUCTION_MODEL == 'H'
          ui_bootloader_first(hdr);
#else
          ui_screen_firmware_info(vhdr, hdr);
#endif
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
          ui_screen_done(0, sectrue);
          ui_fadein();
          usb_stop();
          usb_deinit();
          while (!touch_click()) {
          }
          restart();
          return secfalse;  // shutdown
        }
        break;
      case MSG_NAME_TO_ID(FirmwareErase):  // FirmwareErase
        process_msg_FirmwareErase(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(FirmwareUpload):  // FirmwareUpload
        r = process_msg_FirmwareUpload(USB_IFACE_NUM, msg_size, buf);
        if (r < 0 && r != -4) {  // error, but not user abort (-4)
          ui_fadeout();
          ui_screen_fail();
          ui_fadein();
          usb_stop();
          usb_deinit();
          while (!touch_click()) {
          }
          restart();
          return secfalse;    // shutdown
        } else if (r == 0) {  // last chunk received
          // ui_screen_install_progress_upload(1000);
          ui_fadeout();
          ui_screen_done(4, sectrue);
          ui_fadein();
          ui_screen_done(3, secfalse);
          hal_delay(1000);
          ui_screen_done(2, secfalse);
          hal_delay(1000);
          ui_screen_done(1, secfalse);
          hal_delay(1000);
          usb_stop();
          usb_deinit();
          ui_fadeout();
          return sectrue;  // jump to firmware
        }
        break;
      case MSG_NAME_TO_ID(FirmwareErase_ex):  // erase ble update buffer
        process_msg_FirmwareEraseBLE(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(GetFeatures):  // GetFeatures
        process_msg_GetFeatures(USB_IFACE_NUM, msg_size, buf, vhdr, hdr);
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
}

secbool bootloader_usb_loop_factory(const vendor_header* const vhdr,
                                    const image_header* const hdr) {
  // if both are NULL, we don't have a firmware installed
  // let's show a webusb landing page in this case
  usb_init_all((vhdr == NULL && hdr == NULL) ? sectrue : secfalse);

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
      case MSG_NAME_TO_ID(EmmcFixPermission):  // EmmcFixPermission
        process_msg_EmmcFixPermission(USB_IFACE_NUM, msg_size, buf);
        break;
      case MSG_NAME_TO_ID(EmmcPathInfo):  // EmmcPathInfo
        process_msg_EmmcPathInfo(USB_IFACE_NUM, msg_size, buf);
        break;
      // case MSG_NAME_TO_ID(EmmcFileRead): // EmmcFileRead
      //   process_msg_EmmcFileRead(USB_IFACE_NUM, msg_size, buf);
      //   break;
      // case MSG_NAME_TO_ID(EmmcFileWrite): // EmmcFileWrite
      //   process_msg_EmmcFileWrite(USB_IFACE_NUM, msg_size, buf);
      //   break;
      // case MSG_NAME_TO_ID(EmmcFileDelete): // EmmcFileDelete
      //   process_msg_EmmcFileDelete(USB_IFACE_NUM, msg_size, buf);
      //   break;
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

secbool load_vendor_header_keys(const uint8_t* const data,
                                vendor_header* const vhdr) {
  return load_vendor_header(data, BOOTLOADER_KEY_M, BOOTLOADER_KEY_N,
                            BOOTLOADER_KEYS, vhdr);
}

static secbool check_vendor_header_lock(const vendor_header* const vhdr) {
  uint8_t lock[FLASH_OTP_BLOCK_SIZE];
  ensure(flash_otp_read(FLASH_OTP_BLOCK_VENDOR_HEADER_LOCK, 0, lock,
                        FLASH_OTP_BLOCK_SIZE),
         NULL);
  if (0 ==
      memcmp(lock,
             "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
             "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF",
             FLASH_OTP_BLOCK_SIZE)) {
    return sectrue;
  }
  uint8_t hash[32];
  vendor_header_hash(vhdr, hash);
  return sectrue * (0 == memcmp(lock, hash, 32));
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

static secbool validate_firmware_headers(vendor_header* const vhdr,
                                         image_header* const hdr) {
  set_handle_flash_ecc_error(sectrue);
  secbool result = secfalse;
  while (true) {
    // check
    if (sectrue !=
        load_vendor_header_keys((const uint8_t*)FIRMWARE_START, vhdr))
      break;

    if (sectrue != check_vendor_header_lock(vhdr)) break;

    if (sectrue !=
        load_image_header((const uint8_t*)(FIRMWARE_START + vhdr->hdrlen),
                          FIRMWARE_IMAGE_MAGIC, FIRMWARE_IMAGE_MAXSIZE,
                          vhdr->vsig_m, vhdr->vsig_n, vhdr->vpub, hdr))
      break;

    // passed, return true
    result = sectrue;
    break;
  }
  set_handle_flash_ecc_error(secfalse);
  return result;
}

static secbool validate_firmware_code(vendor_header* const vhdr,
                                      image_header* const hdr) {
  set_handle_flash_ecc_error(sectrue);
  secbool result =
      check_image_contents(hdr, IMAGE_HEADER_SIZE + vhdr->hdrlen,
                           FIRMWARE_SECTORS, FIRMWARE_SECTORS_COUNT);
  set_handle_flash_ecc_error(secfalse);
  return result;
}

int main(void) {
  volatile uint32_t stay_in_bootloader_flag = *STAY_IN_FLAG_ADDR;

  SystemCoreClockUpdate();
  mpu_config_bootloader();

  // user interface
  lcd_para_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  lcd_pwm_init();
  touch_init();

  // fault handler
  bus_fault_enable(); // it's here since requires user interface

  // storages
  qspi_flash_init();
  qspi_flash_config();
  qspi_flash_memory_mapped();
  ensure_emmcfs(emmc_fs_init(), "emmc_fs_init");
  ensure_emmcfs(emmc_fs_mount(true, false), "emmc_fs_mount");

  // bt/pm
  ble_usart_init();
  spi_slave_init();

  // misc/feedback
  random_delays_init();
  // motor_init(); //need to be refactored as motor type changed

#if !PRODUCTION
  // write_dev_dummy_config();
  // serial_set = device_serial_set();
  // UNUSED(write_dev_dummy_config);

  // char fp_ver[8];
  // FpAlgorithmLibVer(fp_ver);
  // dbgprintf_Wait("fp_ver=%s", fp_ver);
  // dbgprintf_Wait("throw back -> %08X", compile_test(TEST_MAGIC_A));

  // camera_test();
  UNUSED(camera_test);

  // nfc_test();
  UNUSED(nfc_test);

  // fp_test();
  UNUSED(fp_test);
#endif

  // se
  thd89_init();
  uint8_t se_state;
  ensure(se_get_state(&se_state) ? sectrue : secfalse, "get se state failed");

  device_para_init();

  bool serial_set = false, cert_set = false;

  if (!serial_set) {
    serial_set = device_serial_set();
  }

  if (!cert_set) {
    cert_set = se_has_cerrificate();
  }

  if (!serial_set || !cert_set) {
    display_clear();
    device_set_factory_mode(true);
    ui_bootloader_factory();
    if (bootloader_usb_loop_factory(NULL, NULL) != sectrue) {
      return 1;
    }
  }

#if PRODUCTION
  // device function test
  device_test(false);

  // burnin test
  jpeg_init();
  device_burnin_test(false);

  // check bootloader downgrade
  check_bootloader_version();
#endif

  secbool stay_in_bootloader = secfalse;  // flag to stay in bootloader

  if (stay_in_bootloader_flag == STAY_IN_BOOTLOADER_FLAG) {
    *STAY_IN_FLAG_ADDR = 0;
    stay_in_bootloader = sectrue;
  }
  if (se_state == THD89_STATE_BOOT) {
    stay_in_bootloader = sectrue;
  }

  // delay to detect touch or skip if we know we are staying in bootloader
  // anyway
  uint32_t touched = 0;
  if (stay_in_bootloader != sectrue) {
    for (int i = 0; i < 100; i++) {
      touched = touch_is_detected() | touch_read();
      if (touched) {
        break;
      }
      hal_delay(1);
    }
  }

  vendor_header vhdr;
  image_header hdr;

  // check stay_in_bootloader flag
  if (stay_in_bootloader == sectrue) {
    display_clear();
    if (sectrue == validate_firmware_headers(&vhdr, &hdr)) {
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
  }

  // check if firmware valid
  if (sectrue == validate_firmware_headers(&vhdr, &hdr)) {
    if (sectrue == validate_firmware_code(&vhdr, &hdr)) {
      // __asm("nop"); // all good, do nothing
    } else {
      display_clear();
      ui_bootloader_first(&hdr);
      if (bootloader_usb_loop(&vhdr, &hdr) != sectrue) {
        return 1;
      }
    }
  } else {
    display_clear();
    ui_bootloader_first(NULL);
    if (bootloader_usb_loop(NULL, NULL) != sectrue) {
      return 1;
    }
  }

  // check if firmware valid again to make sure
  ensure(validate_firmware_headers(&vhdr, &hdr), "invalid firmware header");
  ensure(validate_firmware_code(&vhdr, &hdr), "invalid firmware code");

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

    display_clear();
  }

  // jump_to_unprivileged(FIRMWARE_START + vhdr.hdrlen + IMAGE_HEADER_SIZE);

  bus_fault_disable();

  mpu_config_off();

  jump_to(FIRMWARE_START + vhdr.hdrlen + IMAGE_HEADER_SIZE);

  return 0;
}
