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
#include "emmc_fs.h"
#include "flash.h"
#include "i2c.h"
#include "image.h"
#include "lowlevel.h"
#include "mipi_lcd.h"
#include "mpu.h"
#include "qspi_flash.h"
#include "secbool.h"
#include "spi.h"
#include "spi_legacy.h"
#include "systick.h"
#include "touch.h"
#include "version.h"

#include "payload.h"

#include STM32_HAL_H

// helper macros
#define FORCE_IGNORE_RETURN(x) \
  { __typeof__(x) __attribute__((unused)) d = (x); }
#define _TO_STR(x) #x
#define TO_STR(x) _TO_STR(x)

// defines
#define VERSION_STR \
  TO_STR(VERSION_MAJOR) "." TO_STR(VERSION_MINOR) "." TO_STR(VERSION_PATCH)
#define PIXEL_STEP 5

#if PRODUCTION
const uint8_t * const BOOTLOADER_KEYS[] = {
    (const uint8_t *)"\x15\x4b\x8a\xb2\x61\xcc\x88\x79\x48\x3f\x68\x9a\x2d\x41\x24\x3a\xe7\xdb\xc4\x02\x16\x72\xbb\xd2\x5c\x33\x8a\xe8\x4d\x93\x11\x54",
    (const uint8_t *)"\xa9\xe6\x5e\x07\xfe\x6d\x39\xa8\xa8\x4e\x11\xa9\x96\xa0\x28\x3f\x88\x1e\x17\x5c\xba\x60\x2e\xb5\xac\x44\x2f\xb7\x5b\x39\xe8\xe0",
    (const uint8_t *)"\x6c\x88\x05\xab\xb2\xdf\x9d\x36\x79\xf1\xd2\x8a\x40\xcd\x99\x03\x99\xb9\x9f\xc3\xee\x4e\x06\x57\xd8\x1d\x38\x1e\xa1\x48\x8a\x12",
    (const uint8_t *)"\x3e\xd7\x97\x79\x06\x4d\x56\x57\x1b\x29\xbc\xaa\x73\x4c\xbb\x6d\xb6\x1d\x2e\x62\x65\x66\x62\x8e\xcf\x4c\x89\xe1\xdb\x45\xea\xec",
    (const uint8_t *)"\x54\xa4\x06\x33\xbf\xd9\xe6\x0b\x8a\x39\x12\x65\xb2\xe0\x06\x37\x4a\xbe\x63\x1d\x1e\x11\x07\x33\x2b\xca\x56\xbf\x9f\x8c\x5c\x99",
    (const uint8_t *)"\x4b\x71\x13\x4f\x18\xe0\x07\x87\xc5\x83\xd4\x07\x42\xcc\x18\x8e\x17\xfc\x85\xad\xe4\xcb\x47\x2d\xae\x5e\xf8\xe0\x69\xf0\xfe\xc5",
    (const uint8_t *)"\x2e\xcf\x80\xc8\x2b\x44\x98\x48\xc0\x00\x33\x50\x92\x13\x95\x51\xbf\xe4\x7b\x3c\x73\x17\xb4\x99\x50\xf6\x5e\x1d\x82\x43\x20\x24",
};
const uint8_t BOOTLOADER_KEY_M = 4;
const uint8_t BOOTLOADER_KEY_N = 7;
#else
const uint8_t * const BOOTLOADER_KEYS[] = {
    (const uint8_t *)"\x57\x11\x4f\x0a\xa6\x69\xd2\xf8\x37\xe0\x40\xab\x9b\xb5\x1c\x00\x99\x12\x09\xf8\x4b\xfd\x7b\xf0\xf8\x93\x67\x62\x46\xfb\xa2\x4a",
    (const uint8_t *)"\xdc\xae\x8e\x37\xdf\x5c\x24\x60\x27\xc0\x3a\xa9\x51\xbd\x6e\xc6\xca\xa7\xad\x32\xc1\x66\xb1\xf5\x48\xa4\xef\xcd\x88\xca\x3c\xa5",
    (const uint8_t *)"\x77\x29\x12\xab\x61\xd1\xdc\x4f\x91\x33\x32\x5e\x57\xe1\x46\xab\x9f\xac\x17\xa4\x57\x2c\x6f\xcd\xf3\x55\xf8\x00\x36\x10\x00\x04",
};
const uint8_t BOOTLOADER_KEY_M = 2;
const uint8_t BOOTLOADER_KEY_N = 3;
#endif

#define USB_IFACE_NUM 0

#if PRODUCTION
#error "Build for production is NOT allowed!!!"
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
      if (flash_fix_ecc_fault_BOARDLOADER(SCB->BFAR)) {
        error_shutdown("Internal flash ECC error", "Cleanup successful",
                       "Boardloader reinstall required", "Contact HW Team.");
      } else {
        error_shutdown("Internal flash ECC error", "Cleanup failed",
                       "Contact HW Team.", "");
      }
    }
  }

  // normal route
  error_shutdown("Internal error", "(BF)", NULL, NULL);
}

void UsageFault_Handler(void) {
  error_shutdown("Internal error", "(UF)", NULL, NULL);
}

static secbool flash_write_words_unsafe(uint8_t sector, uint32_t offset,
                                        uint32_t data[8]) {
  uint32_t flash_word[8];
  int retry = 0;

  uint32_t address = (uint32_t)flash_get_address(sector, offset, 4);
  if (address == 0) {
    return secfalse;
  }
  if (address % 32) {  // we write only at 4-byte boundary
    return secfalse;
  }
  if (offset % 32) {  // we write only at 4-byte boundary
    return secfalse;
  }
  for (int i = 0; i < 8; i++) {
    if (flash_word[i] !=
        (flash_word[i] & *((const uint32_t *)(address + 4 * i)))) {
      return secfalse;
    }
  }
rewrite:
  retry++;
  if (retry > 3) {
    return secfalse;
  }
  memcpy(flash_word, data, 32);

  if (sector >= FLASH_SECTOR_BOARDLOADER &&
      sector <= FLASH_SECTOR_OTP_EMULATOR) {
    if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, address,
                                    (uint32_t)&flash_word)) {
      goto rewrite;
    }
  } else {
    if (HAL_OK !=
        qspi_flash_write_buffer_unsafe((uint8_t *)&flash_word,
                                       address - QSPI_FLASH_BASE_ADDRESS, 32)) {
      goto rewrite;
    }
  }

  for (int i = 0; i < 8; i++) {
    if (flash_word[i] != *((const uint32_t *)(address + 4 * i))) {
      goto rewrite;
    }
  }
  return sectrue;
}

static secbool boardloader_update(bool auto_reboot) {
  // update process
  secbool temp_state;

  display_backlight(255);
  display_clear();
  display_printf("OneKey Bootloader " VERSION_STR "\n");
  display_printf("=====================\n");
  display_printf("Boardloader Update\n");
  display_printf("!!! DO NOT POWER OFF !!!\n");
  display_printf("\r\n");

  // display_printf("Touch to start\n");
  // while(!touch_click()){}

  // erase
  display_printf("\rErasing: ");
  temp_state = flash_erase(FLASH_SECTOR_BOARDLOADER);
  if (temp_state != sectrue) {
    display_printf(" fail\n");
    while (true) {
      hal_delay(100);
    }  // die here
  } else
    display_printf(" done\n");

  // unlock
  display_printf("\rPreparing Write: ");
  temp_state = flash_unlock_write();
  if (temp_state != sectrue) {
    display_printf(" fail\n");
    while (true) {
      hal_delay(100);
    }  // die here
  } else
    display_printf(" done\n");

  // write
  size_t processed_bytes = 0;
  uint16_t last_progress = 0;
  uint16_t current_progress = 0;
  display_printf("\rWriting: %u%%", current_progress);
  for (size_t sector_offset = 0;
       sector_offset < flash_sector_size(FLASH_SECTOR_BOARDLOADER);
       sector_offset += 32) {
    temp_state =
        flash_write_words_unsafe(FLASH_SECTOR_BOARDLOADER, sector_offset,
                                 (uint32_t *)(payload + processed_bytes));
    if (temp_state != sectrue) break;
    processed_bytes += ((sizeof(payload) - processed_bytes) > 32)
                           ? 32  // since we could only write 32 byte a time
                           : (sizeof(payload) - processed_bytes);

    current_progress = (uint16_t)(processed_bytes * 100 / sizeof(payload));
    if ((last_progress != current_progress)) {
      display_printf("\rWriting: %u%%", current_progress);
      last_progress = current_progress;
      hal_delay(100);  // slow down a little to reduce lcd refresh flickering
    }
  }
  if (temp_state != sectrue) {
    display_printf(" fail\n");
    while (true) {
      hal_delay(100);
    }  // die here
  } else
    display_printf(" done\n");

  // lock
  display_printf("\rFinishing Write: ");
  temp_state = flash_lock_write();
  if (temp_state != sectrue) {
    display_printf(" fail\n");
    while (true) {
      hal_delay(100);
    }  // die here
  } else
    display_printf(" done\n");

  // reboot
  if (auto_reboot) {
    for (int i = 3; i >= 0; i--) {
      display_printf("\rRestarting in %d second(s)", i);
      hal_delay(1000);
    }

    restart();
  }
  return sectrue;
}

int main(void) {
  SystemCoreClockUpdate();
  mpu_config_off();
  dwt_init();

  // user interface
  lcd_para_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  // lcd_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  lcd_pwm_init();
  display_clear();
  touch_init();

  // fault handler
  bus_fault_enable();  // it's here since requires user interface
  set_handle_flash_ecc_error(sectrue);

  boardloader_update(false);

  set_handle_flash_ecc_error(secfalse);
  bus_fault_disable();

  // self destruct (the cruppy way)
  FORCE_IGNORE_RETURN(flash_erase(FLASH_SECTOR_BOOTLOADER_2));

  restart();

  return 0;
}
