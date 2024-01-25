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

#include <stdio.h>
#include <string.h>

#include "blake2s.h"
#include "common.h"
#include "compiler_traits.h"
#include "display.h"
#include "emmc.h"
#include "ff.h"
#include "flash.h"
#include "i2c.h"
#include "image.h"
#include "mini_printf.h"
#include "mipi_lcd.h"
#include "qspi_flash.h"
#include "rng.h"
#ifdef TREZOR_MODEL_T
#include "sdcard.h"
#include "sdram.h"
#include "touch.h"
#endif
#include "usb.h"

#include "lowlevel.h"
#include "version.h"

#include "memzero.h"

#include "ble.h"
#include "camera.h"
#include "fingerprint.h"
#include "fp_sensor_wrapper.h"
#include "motor.h"
#include "thd89_boot.h"
#include "systick.h"
#include "thd89.h"
#include "usart.h"
#include "nfc.h"

#define BOARD_MODE 1
#define BOOT_MODE 2
#define PIXEL_STEP 5

#if PRODUCTION
const uint8_t BOARDLOADER_KEY_M = 4;
const uint8_t BOARDLOADER_KEY_N = 7;
#else
const uint8_t BOARDLOADER_KEY_M = 2;
const uint8_t BOARDLOADER_KEY_N = 3;
#endif

static const uint8_t * const BOARDLOADER_KEYS[] = {
#if PRODUCTION
    (const uint8_t
    *)"\x15\x4b\x8a\xb2\x61\xcc\x88\x79\x48\x3f\x68\x9a\x2d\x41\x24\x3a\xe7\xdb\xc4\x02\x16\x72\xbb\xd2\x5c\x33\x8a\xe8\x4d\x93\x11\x54",
    (const uint8_t
    *)"\xa9\xe6\x5e\x07\xfe\x6d\x39\xa8\xa8\x4e\x11\xa9\x96\xa0\x28\x3f\x88\x1e\x17\x5c\xba\x60\x2e\xb5\xac\x44\x2f\xb7\x5b\x39\xe8\xe0",
    (const uint8_t
    *)"\x6c\x88\x05\xab\xb2\xdf\x9d\x36\x79\xf1\xd2\x8a\x40\xcd\x99\x03\x99\xb9\x9f\xc3\xee\x4e\x06\x57\xd8\x1d\x38\x1e\xa1\x48\x8a\x12",
    (const uint8_t
    *)"\x3e\xd7\x97\x79\x06\x4d\x56\x57\x1b\x29\xbc\xaa\x73\x4c\xbb\x6d\xb6\x1d\x2e\x62\x65\x66\x62\x8e\xcf\x4c\x89\xe1\xdb\x45\xea\xec",
    (const uint8_t
    *)"\x54\xa4\x06\x33\xbf\xd9\xe6\x0b\x8a\x39\x12\x65\xb2\xe0\x06\x37\x4a\xbe\x63\x1d\x1e\x11\x07\x33\x2b\xca\x56\xbf\x9f\x8c\x5c\x99",
    (const uint8_t
    *)"\x4b\x71\x13\x4f\x18\xe0\x07\x87\xc5\x83\xd4\x07\x42\xcc\x18\x8e\x17\xfc\x85\xad\xe4\xcb\x47\x2d\xae\x5e\xf8\xe0\x69\xf0\xfe\xc5",
    (const uint8_t
    *)"\x2e\xcf\x80\xc8\x2b\x44\x98\x48\xc0\x00\x33\x50\x92\x13\x95\x51\xbf\xe4\x7b\x3c\x73\x17\xb4\x99\x50\xf6\x5e\x1d\x82\x43\x20\x24",
#else
// TREZOR dev_key
// (const uint8_t
// *)"\xdb\x99\x5f\xe2\x51\x69\xd1\x41\xca\xb9\xbb\xba\x92\xba\xa0\x1f\x9f\x2e\x1e\xce\x7d\xf4\xcb\x2a\xc0\x51\x90\xf3\x7f\xcc\x1f\x9d",
// (const uint8_t
// *)"\x21\x52\xf8\xd1\x9b\x79\x1d\x24\x45\x32\x42\xe1\x5f\x2e\xab\x6c\xb7\xcf\xfa\x7b\x6a\x5e\xd3\x00\x97\x96\x0e\x06\x98\x81\xdb\x12",
// (const uint8_t
// *)"\x22\xfc\x29\x77\x92\xf0\xb6\xff\xc0\xbf\xcf\xdb\x7e\xdb\x0c\x0a\xa1\x4e\x02\x5a\x36\x5e\xc0\xe3\x42\xe8\x6e\x38\x29\xcb\x74\xb6",

// ONEKEY dev_key
  (const uint8_t *)"\x57\x11\x4f\x0a\xa6\x69\xd2\xf8\x37\xe0\x40\xab\x9b\xb5\x1c\x00\x99\x12\x09\xf8\x4b\xfd\x7b\xf0\xf8\x93\x67\x62\x46\xfb\xa2\x4a",
  (const uint8_t *)"\xdc\xae\x8e\x37\xdf\x5c\x24\x60\x27\xc0\x3a\xa9\x51\xbd\x6e\xc6\xca\xa7\xad\x32\xc1\x66\xb1\xf5\x48\xa4\xef\xcd\x88\xca\x3c\xa5",
  (const uint8_t *)"\x77\x29\x12\xab\x61\xd1\xdc\x4f\x91\x33\x32\x5e\x57\xe1\x46\xab\x9f\xac\x17\xa4\x57\x2c\x6f\xcd\xf3\x55\xf8\x00\x36\x10\x00\x04",
#endif
};

// clang-format off
static const uint8_t toi_icon_onekey[] = {
    // magic
    'T', 'O', 'I', 'f',
    // width (16-bit), height (16-bit)
    0x80, 0x00, 0x80, 0x00,
    // compressed data length (32-bit)
    0x4c, 0x05, 0x00, 0x00,
    // compressed data
    0xed, 0x92, 0x2b, 0x8e, 0xeb, 0x30, 0x14, 0x86, 0x0d, 0x0c, 0x0c, 0x02, 0x0c, 0xba, 0x01, 0x2f, 0x21, 0x5b, 0x30, 0x1c, 0x58, 0x38, 0x34, 0x68, 0x34, 0x1a, 0x70, 0x15, 0x05, 0x44, 0x95, 0x49, 0x14, 0x05, 0x54, 0x57, 0x01, 0x55, 0x35, 0x28, 0xb4, 0x30, 0x70, 0xa0, 0xb7, 0x90, 0x25, 0x64, 0x03, 0x03, 0x0c, 0x02, 0x0c, 0x02, 0x7c, 0x55, 0x5d, 0x55, 0xd3, 0x36, 0x7d, 0x24, 0xed, 0xc9, 0x73, 0xf2, 0x85, 0xb5, 0xb6, 0xff, 0xf3, 0x9d, 0x73, 0x10, 0x9a, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x81, 0x85, 0xe1, 0xe5, 0xda, 0xb1, 0x42, 0x9a, 0x14, 0x49, 0x21, 0x75, 0x5e, 0xee, 0x3f, 0x73, 0xc6, 0xfe, 0xb7, 0xcc, 0x97, 0x3a, 0xf5, 0x92, 0x22, 0xa4, 0x8e, 0xc5, 0x89, 0x1d, 0x8d, 0xd9, 0x99, 0x06, 0x9c, 0xbc, 0x7f, 0x24, 0x45, 0xd5, 0xb4, 0x09, 0x99, 0x9f, 0x7a, 0x21, 0xe5, 0x84, 0x06, 0x63, 0xf2, 0xde, 0x6e, 0x32, 0xdf, 0x00, 0x93, 0xf9, 0xdb, 0xcd, 0x72, 0x3d, 0xe4, 0x3e, 0xd0, 0xc0, 0xb1, 0xa4, 0x56, 0xc2, 0xb4, 0x4a, 0xea, 0xbd, 0x7e, 0x0f, 0xaf, 0x0b, 0xfb, 0x99, 0xb7, 0x6d, 0xfe, 0x83, 0x12, 0x49, 0xc1, 0xc9, 0x70, 0xdc, 0xa5, 0x36, 0x3d, 0x20, 0xf5, 0xeb, 0xf7, 0x6f, 0x75, 0x3f, 0x90, 0x97, 0xfd, 0xf5, 0x80, 0xe1, 0x7e, 0xdd, 0x0f, 0xa4, 0x1e, 0xc3, 0xdd, 0xdb, 0xbf, 0x7f, 0x28, 0x61, 0x06, 0xc3, 0x4a, 0xfd, 0xc6, 0xc9, 0x1f, 0x93, 0xf9, 0x5d, 0x6d, 0x01, 0x27, 0x43, 0x9a, 0xfc, 0x0f, 0x79, 0x69, 0x47, 0x5d, 0xec, 0xbd, 0x19, 0x30, 0x6e, 0xdc, 0xae, 0xfd, 0x4a, 0x99, 0x81, 0xb3, 0x52, 0xbf, 0xd9, 0xbe, 0xcd, 0x0e, 0x0c, 0x7b, 0xf3, 0x8f, 0x71, 0x63, 0x78, 0x7b, 0x3b, 0x32, 0x23, 0x82, 0x13, 0x58, 0x7b, 0x86, 0xf3, 0x72, 0x4c, 0xfe, 0x4a, 0x30, 0x0c, 0xe9, 0x9f, 0xf9, 0x66, 0x64, 0x48, 0x0d, 0x67, 0xbf, 0x52, 0x66, 0x84, 0xb8, 0x31, 0x8c, 0x3d, 0xc3, 0x66, 0x94, 0x28, 0xc1, 0x30, 0x84, 0x7f, 0xea, 0x99, 0x91, 0x22, 0xf5, 0xf3, 0xf6, 0x8e, 0x65, 0x46, 0x0c, 0x27, 0xcf, 0xfa, 0xe7, 0x25, 0x5c, 0x35, 0x79, 0x79, 0xeb, 0x1b, 0xe2, 0x06, 0x40, 0x4e, 0x3f, 0x29, 0x6e, 0x25, 0x31, 0x3c, 0xc4, 0x0d, 0x90, 0x1a, 0xae, 0x92, 0x97, 0xb7, 0x3e, 0xfc, 0x9f, 0xd9, 0x00, 0x4e, 0xe0, 0xea, 0x50, 0xe2, 0x76, 0x56, 0x5b, 0xfe, 0xcf, 0x6c, 0xc0, 0x6e, 0x01, 0x57, 0x45, 0x52, 0xf4, 0xe5, 0xbf, 0xdd, 0x3c, 0x66, 0x4f, 0x03, 0x25, 0xe0, 0xaa, 0x78, 0x79, 0xeb, 0xcb, 0x5f, 0x09, 0x1a, 0x3c, 0xe2, 0xef, 0x58, 0x90, 0x55, 0xdc, 0xab, 0xa1, 0x3d, 0x7f, 0x63, 0x1c, 0xeb, 0x11, 0xff, 0xd4, 0x83, 0xab, 0xe0, 0xeb, 0xf3, 0x5e, 0x5a, 0x9b, 0xfe, 0x52, 0x37, 0xb7, 0xa7, 0x01, 0x64, 0x05, 0x21, 0x65, 0xf8, 0xf8, 0xab, 0x6e, 0x43, 0x9b, 0xfe, 0x4a, 0xdc, 0xdb, 0xbe, 0x2a, 0xcb, 0xb5, 0x69, 0x91, 0xa4, 0xe8, 0xd2, 0xdf, 0x18, 0x4e, 0x9a, 0xfa, 0x6f, 0x37, 0x53, 0xf2, 0xdf, 0x6e, 0x9a, 0xfa, 0x67, 0xfe, 0x94, 0xfc, 0x33, 0xbf, 0x99, 0x3d, 0x0d, 0x8c, 0x99, 0x92, 0xbf, 0x31, 0x34, 0x68, 0xe2, 0xcf, 0xc9, 0xd4, 0xfc, 0x39, 0x69, 0xe2, 0x1f, 0xd2, 0xa9, 0xf9, 0xbb, 0x71, 0x13, 0xff, 0xd4, 0x9b, 0x9a, 0x7f, 0x35, 0xf1, 0x16, 0x99, 0x3f, 0x35, 0xff, 0xbc, 0x6c, 0xe2, 0x6f, 0xcc, 0xd4, 0xfc, 0x8d, 0xa9, 0x6f, 0x6f, 0x47, 0x53, 0xf4, 0x67, 0xb8, 0xae, 0x3f, 0x27, 0x53, 0xf4, 0x7f, 0x79, 0xab, 0xeb, 0xef, 0x58, 0x53, 0xf4, 0x77, 0xac, 0xba, 0xfe, 0x21, 0x9d, 0xa2, 0x7f, 0x48, 0xeb, 0xfa, 0xef, 0x16, 0x53, 0xf4, 0xaf, 0x66, 0x5e, 0x23, 0xf5, 0x7e, 0xb7, 0xbf, 0xd4, 0x53, 0xf4, 0x97, 0xba, 0xae, 0x7f, 0xe6, 0xb7, 0x5f, 0x8b, 0x63, 0x9d, 0x7e, 0x6e, 0xdc, 0x76, 0x66, 0x5e, 0xd6, 0xf5, 0xcf, 0x4b, 0x33, 0x41, 0x66, 0xff, 0xba, 0xfe, 0xdd, 0x55, 0xf4, 0xff, 0xeb, 0x2a, 0x6f, 0x28, 0xfe, 0x52, 0xbf, 0x7f, 0xd8, 0x11, 0x0d, 0x7e, 0x12, 0xed, 0xe8, 0xfd, 0x43, 0xea, 0xe9, 0xfb, 0x2b, 0x11, 0xd2, 0x63, 0xef, 0x53, 0x18, 0xde, 0x2d, 0xa6, 0xec, 0x2f, 0x35, 0xc3, 0xf7, 0xb2, 0xdb, 0xec, 0x41, 0xbf, 0xfe, 0x6e, 0x5c, 0x37, 0x3f, 0xa4, 0x53, 0xf3, 0x57, 0x62, 0xb9, 0x46, 0x0d, 0x58, 0xae, 0x95, 0x98, 0x92, 0x7f, 0x33, 0xfb, 0xff, 0x1d, 0x98, 0x8e, 0x7f, 0x48, 0xd1, 0x03, 0xb8, 0x71, 0x5f, 0xfe, 0x79, 0x09, 0x99, 0x9a, 0x14, 0x97, 0x53, 0x68, 0xc0, 0x89, 0x63, 0x39, 0x16, 0x27, 0x34, 0xb8, 0x7c, 0x42, 0x6a, 0xc8, 0x3a, 0xf2, 0xb2, 0x1f, 0x7f, 0x86, 0xab, 0x09, 0x9c, 0x9c, 0xba, 0xa5, 0x9e, 0x1d, 0x55, 0x4f, 0x31, 0xac, 0xc4, 0xd8, 0xfd, 0x2f, 0x4d, 0xff, 0xef, 0x9f, 0x4b, 0x27, 0xff, 0xfe, 0xa9, 0x9e, 0x0c, 0x29, 0x5c, 0x25, 0x99, 0x5f, 0xd7, 0x1f, 0x72, 0xef, 0xaa, 0xd3, 0xbf, 0xfe, 0xba, 0xd4, 0xd5, 0x0d, 0x80, 0xab, 0xa4, 0xfa, 0xfa, 0x35, 0x76, 0x8b, 0xf6, 0x32, 0x2f, 0xcf, 0xfe, 0xfa, 0x0e, 0xc0, 0xcd, 0xe2, 0xeb, 0xb3, 0x7b, 0x7f, 0x37, 0x6e, 0x3a, 0x4f, 0x4e, 0x4e, 0x6f, 0xb8, 0x31, 0x54, 0x2d, 0x49, 0x51, 0xd7, 0x3f, 0xa4, 0x50, 0x99, 0xe7, 0x36, 0xf7, 0x3b, 0x7b, 0xbe, 0x31, 0x9c, 0x40, 0xd5, 0x12, 0xd2, 0xba, 0xfe, 0x8e, 0x05, 0x95, 0x49, 0x83, 0xe3, 0x77, 0x69, 0x70, 0xff, 0x86, 0x12, 0xcd, 0xef, 0xd4, 0xc3, 0xb1, 0xea, 0xfa, 0x2f, 0xd7, 0x30, 0x89, 0x4a, 0x3c, 0x32, 0x4b, 0x3b, 0x3a, 0xbd, 0xa5, 0x44, 0x3b, 0xbb, 0x78, 0x1d, 0x86, 0x61, 0x12, 0xf3, 0xf2, 0x91, 0xbe, 0x9e, 0xcf, 0x29, 0x2f, 0x61, 0xaa, 0x39, 0xef, 0xeb, 0x2d, 0xda, 0xf1, 0x77, 0xac, 0x3a, 0xb7, 0x5e, 0xde, 0xda, 0xf1, 0x47, 0x0d, 0x80, 0xc9, 0x54, 0xe2, 0xf4, 0x55, 0x3b, 0x7a, 0x64, 0x4f, 0x95, 0x80, 0xa8, 0x25, 0xf3, 0x9b, 0xf8, 0xef, 0x16, 0x30, 0x3d, 0xa7, 0xc1, 0xf1, 0xab, 0x34, 0xa8, 0xe3, 0x72, 0x5a, 0x09, 0x0d, 0x60, 0x2a, 0xf9, 0xfa, 0x6c, 0xe2, 0xef, 0xc6, 0x30, 0xa9, 0xe7, 0xb3, 0x94, 0xfa, 0xde, 0x8d, 0xa4, 0x38, 0xbd, 0xc1, 0x09, 0x4c, 0x25, 0x21, 0x6d, 0xe2, 0x0f, 0x95, 0xea, 0xc6, 0x4d, 0xdf, 0x65, 0xb8, 0x9b, 0x49, 0xdc, 0x06, 0x6a, 0xeb, 0xa4, 0x3e, 0x7f, 0x79, 0xbb, 0x69, 0x36, 0xa5, 0xfb, 0x1b, 0x53, 0x0f, 0x1a, 0xa0, 0x46, 0x64, 0x3e, 0x4c, 0xae, 0x1d, 0x9d, 0xbf, 0xbc, 0x5b, 0x5c, 0x3b, 0xbb, 0x5b, 0x9c, 0x9f, 0x65, 0x18, 0xa6, 0x8a, 0xcc, 0x47, 0x0d, 0xb9, 0x3d, 0xa7, 0xfa, 0x24, 0x45, 0xf5, 0xed, 0x90, 0x5e, 0x3a, 0xb9, 0xdd, 0x54, 0x4f, 0x5e, 0xef, 0x55, 0x33, 0x2e, 0xbd, 0x7d, 0x1b, 0x4e, 0x0c, 0x10, 0x0c, 0x57, 0x5f, 0x67, 0x78, 0xb7, 0x50, 0xe2, 0x70, 0x42, 0x89, 0xa4, 0xe0, 0xe4, 0xd2, 0x29, 0xa8, 0x1a, 0x5e, 0xde, 0x9a, 0xfa, 0xd3, 0xe0, 0xa7, 0xbe, 0xe7, 0x90, 0xfa, 0x7a, 0x8f, 0x39, 0x59, 0xae, 0xed, 0x88, 0x06, 0x97, 0xff, 0x97, 0x1a, 0xca, 0xff, 0x5a, 0xc2, 0x2d, 0xe0, 0xd2, 0x57, 0x0a, 0x3d, 0xc0, 0x4a, 0x41, 0xe5, 0x7f, 0x7d, 0x3e, 0x92, 0xef, 0x58, 0x06, 0x8c, 0xd7, 0xef, 0xa6, 0xe9, 0xaf, 0xdf, 0x7d, 0xa6, 0xef, 0xa1, 0x81, 0x12, 0x7d, 0xd5, 0x00, 0x69, 0xaf, 0x04, 0x0d, 0xd0, 0x43, 0x6c, 0x37, 0x06, 0x90, 0x95, 0xaa, 0x9b, 0x0b, 0xb7, 0xf9, 0x7b, 0x92, 0x02, 0x3d, 0x08, 0x27, 0x06, 0x94, 0xbc, 0x64, 0xf8, 0x5e, 0xa6, 0x1d, 0x49, 0x0d, 0x9b, 0xca, 0x09, 0x7a, 0x18, 0xe8, 0x5a, 0xf6, 0xd3, 0xb8, 0xde, 0x03, 0x4e, 0x76, 0x0b, 0xe8, 0x3c, 0xa9, 0xd1, 0x13, 0x40, 0x6f, 0xc0, 0x7f, 0x32, 0x3f, 0xa4, 0x9c, 0xd0, 0xe0, 0x90, 0x42, 0x03, 0x4e, 0x56, 0x0a, 0xbe, 0xd7, 0x7b, 0x5e, 0xbf, 0xd1, 0x53, 0xb4, 0x53, 0xd5, 0x81, 0xbc, 0xcc, 0xcb, 0x76, 0xdf, 0x47, 0x4f, 0xd2, 0xce, 0x06, 0x74, 0xc5, 0xb3, 0xd3, 0x6f, 0x7f, 0x03, 0xda, 0x24, 0x29, 0x10, 0x00, 0x0c, 0x2b, 0x31, 0x4e, 0x7f, 0x86, 0x11, 0x08, 0x6e, 0x3c, 0x46, 0xfb, 0x90, 0x22, 0x30, 0xa4, 0x1e, 0x9b, 0x7d, 0xe6, 0x23, 0x40, 0x18, 0x56, 0x62, 0x4c, 0xf6, 0x79, 0xc9, 0x30, 0x02, 0x85, 0x93, 0x31, 0xf9, 0xdb, 0x11, 0x02, 0xc7, 0x8d, 0xc7, 0x62, 0xef, 0xc6, 0xa8, 0x15, 0x42, 0x3a, 0x06, 0xfb, 0x90, 0xa2, 0xd6, 0x18, 0x7e, 0x07, 0xda, 0xb4, 0xdf, 0xe3, 0xc6, 0x43, 0xb6, 0x6f, 0x6b, 0xf3, 0x8f, 0xb1, 0xa3, 0xbc, 0x1c, 0xa2, 0xbb, 0x12, 0x9c, 0xa0, 0x4e, 0x60, 0x38, 0xf3, 0x87, 0x66, 0x2f, 0x35, 0xc3, 0xa8, 0x43, 0x42, 0x3a, 0xa4, 0xc9, 0x77, 0xb1, 0xf7, 0xd5, 0x2d, 0x48, 0xbd, 0xdf, 0x38, 0xf9, 0x63, 0x1c, 0x2b, 0x2f, 0xfb, 0x75, 0xe7, 0x04, 0xf5, 0x8c, 0x63, 0x49, 0xfd, 0x5b, 0xdd, 0x0f, 0x70, 0xb2, 0x5b, 0x28, 0xd1, 0x95, 0xb9, 0x12, 0xdb, 0xcd, 0x70, 0xdc, 0x0f, 0xd0, 0xc0, 0xb1, 0x52, 0xaf, 0x6d, 0x73, 0xa9, 0x1d, 0x8b, 0x06, 0x68, 0xb0, 0xd0, 0x60, 0xb9, 0xde, 0x6e, 0x32, 0x1f, 0xda, 0x3c, 0xf3, 0xf7, 0x33, 0x1f, 0xb2, 0xf9, 0x79, 0x1f, 0x38, 0x09, 0x69, 0xea, 0x3d, 0xd7, 0x89, 0xbc, 0x4c, 0x8a, 0xf7, 0x8f, 0x31, 0x79, 0x5f, 0xc2, 0x8e, 0x38, 0x71, 0xac, 0x90, 0x26, 0x45, 0xea, 0x49, 0x9d, 0xf9, 0x79, 0x99, 0x97, 0x55, 0xd3, 0xfd, 0x27, 0x75, 0x52, 0x24, 0x45, 0x48, 0x1d, 0x6b, 0xb9, 0x66, 0x18, 0xcd, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x00, 0xf3, 0x0f,
};

static const uint8_t toi_icon_safeos[] = {
    // magic
    'T', 'O', 'I', 'f',
    // width (16-bit), height (16-bit)
    0x8c, 0x00, 0x1e, 0x00,
    // compressed data length (32-bit)
    0x4c, 0x05, 0x00, 0x00,
    // compressed data
    0xAD, 0x92, 0xA1, 0xAE, 0xAC, 0x3C, 0x10, 0xC7, 0x8F, 0x40, 0x20, 0x10, 0x15, 0x2B, 0x10, 0xC7, 0xF0, 0x08, 0xBC, 0x02, 0x8F, 0x80, 0xFC, 0x2C, 0x4F, 0x70, 0x42, 0x56, 0x10, 0x82, 0x21, 0x04, 0x41, 0x4E, 0x10, 0x84, 0xAC, 0xB8, 0xC1, 0xAE, 0x5C, 0xB9, 0x12, 0x8B, 0x44, 0x5E, 0x89, 0xBD, 0xE2, 0x8A, 0x0A, 0x04, 0x02, 0xC1, 0x97, 0x86, 0x34, 0x33, 0x2D, 0x2D, 0xB0, 0xF7, 0xEC, 0xA0, 0x68, 0x67, 0xA6, 0xF3, 0xFF, 0xCD, 0xFF, 0xE3, 0x03, 0x82, 0xA4, 0x9E, 0x59, 0x57, 0x8F, 0x6B, 0x3B, 0xB5, 0xD3, 0xE3, 0x9A, 0x11, 0xCF, 0xFC, 0x78, 0x63, 0xE0, 0xEE, 0x7E, 0xF1, 0x9E, 0x9E, 0x6E, 0x1E, 0x96, 0xCD, 0xC8, 0x27, 0xAE, 0x2B, 0xBF, 0x70, 0x8C, 0xB3, 0xD3, 0x40, 0x65, 0x33, 0x86, 0xA5, 0xAE, 0x8E, 0xA4, 0x31, 0xA5, 0xC9, 0x22, 0xC5, 0x30, 0x07, 0xD6, 0x7B, 0x14, 0xF8, 0x05, 0xEE, 0x9E, 0x91, 0x9F, 0x53, 0x8E, 0xE9, 0x30, 0x2F, 0x8A, 0x68, 0xA7, 0xA3, 0x99, 0x1D, 0xE3, 0x7E, 0x51, 0xD5, 0x6D, 0xB7, 0xE5, 0x18, 0x7D, 0xB4, 0x68, 0x62, 0x98, 0xCF, 0xEE, 0x40, 0x1F, 0x31, 0x15, 0x7B, 0xFE, 0x94, 0x4B, 0x60, 0x6D, 0x77, 0x78, 0x76, 0x66, 0x71, 0x43, 0x62, 0x7C, 0x7F, 0x89, 0xEC, 0xD5, 0xE4, 0xE1, 0x15, 0x37, 0xFF, 0x99, 0xDB, 0xE5, 0x8E, 0x3F, 0xE1, 0x42, 0xD2, 0x66, 0x5C, 0x4E, 0x44, 0x4C, 0xCF, 0xCD, 0x22, 0x46, 0x1F, 0x91, 0x94, 0xE7, 0x8A, 0xAE, 0xA2, 0x49, 0x67, 0x77, 0xB6, 0xC8, 0xF4, 0x67, 0x9E, 0x69, 0x27, 0xDC, 0xFD, 0x7E, 0xB9, 0x5F, 0x8E, 0x9C, 0xBE, 0x47, 0x45, 0xEF, 0xEC, 0x33, 0x64, 0x44, 0x07, 0xFC, 0xF9, 0xEC, 0xEC, 0xDF, 0xBF, 0xC4, 0xAA, 0x66, 0x5C, 0x33, 0x1D, 0x03, 0xEB, 0xF7, 0x4C, 0xDE, 0xC1, 0x33, 0xF1, 0x04, 0x8F, 0xEB, 0xBF, 0x2B, 0x79, 0x8F, 0x4F, 0xD6, 0x78, 0x5C, 0xC5, 0x1D, 0x66, 0xC4, 0x33, 0xD7, 0x0D, 0x93, 0xD4, 0x33, 0xC5, 0x0D, 0x2F, 0x4B, 0x58, 0x8A, 0xD5, 0x81, 0x05, 0x77, 0x75, 0xC5, 0x9D, 0x41, 0xD2, 0xC0, 0xC2, 0xBC, 0xFC, 0x42, 0xCC, 0xDD, 0xBA, 0x22, 0x23, 0x90, 0x0D, 0xC4, 0x5E, 0x0B, 0xCF, 0x84, 0xFE, 0x3F, 0xA5, 0x12, 0x53, 0xAC, 0x3A, 0x23, 0xE0, 0x79, 0x1E, 0x8E, 0x21, 0xB2, 0x71, 0x73, 0x7C, 0xFB, 0xBC, 0xC9, 0xAE, 0x80, 0x3A, 0x70, 0xC1, 0x3A, 0x27, 0x68, 0x97, 0x73, 0xF1, 0x7E, 0x3A, 0x5B, 0x7C, 0x01, 0x77, 0x74, 0x8C, 0xED, 0x7C, 0x2A, 0x2E, 0x9D, 0x7D, 0xE4, 0x2C, 0xD6, 0x4B, 0x7F, 0x8F, 0x9D, 0xCD, 0xB7, 0xAA, 0x0A, 0xBC, 0xCD, 0x76, 0xC2, 0x37, 0xA0, 0x7D, 0xBB, 0x65, 0xC7, 0xA0, 0xC9, 0xEA, 0xC1, 0xD5, 0x49, 0xC0, 0x57, 0xE5, 0x72, 0x92, 0xD2, 0xA4, 0x19, 0x55, 0x5E, 0x21, 0x69, 0x58, 0xB6, 0xD3, 0xDA, 0x8B, 0x75, 0x6B, 0xA7, 0xC0, 0x12, 0x33, 0xEA, 0xAA, 0x19, 0x9B, 0x91, 0x93, 0x65, 0x39, 0xEC, 0x9F, 0x7D, 0xB2, 0x7A, 0xCF, 0x6C, 0x46, 0xDE, 0x69, 0x59, 0xFA, 0xA8, 0xAE, 0x54, 0x7C, 0xB0, 0x13, 0xC2, 0x72, 0x8F, 0x70, 0x5D, 0xA9, 0x7D, 0xFE, 0xE7, 0x93, 0x9F, 0xAA, 0xFA, 0x07, 0x16, 0x4D, 0xC0, 0x83, 0xD0, 0x43, 0x64, 0xBB, 0x1F, 0x61, 0x09, 0x3A, 0x20, 0x86, 0x19, 0xBF, 0x07, 0x53, 0xC8, 0x81, 0x67, 0x75, 0x8C, 0x76, 0x52, 0xE5, 0xC4, 0x54, 0xDE, 0x03, 0xDC, 0xA9, 0x9C, 0x2D, 0x6F, 0x53, 0xA5, 0x0A, 0x26, 0x52, 0x7B, 0x00, 0xFB, 0x3E, 0xB0, 0xF4, 0xB3, 0xE8, 0x22, 0xA6, 0x3A, 0xC5, 0x34, 0x71, 0xF3, 0x57, 0xB8, 0xB8, 0xB9, 0x8A, 0xAF, 0x4A, 0xBD, 0x5F, 0xC0, 0x8D, 0x6A, 0xDB, 0xF2, 0xDE, 0x20, 0x1B, 0xD4, 0x3E, 0x6F, 0x30, 0xE7, 0x51, 0x0F, 0xCC, 0x96, 0xD1, 0xF5, 0x8B, 0x7F, 0xA7, 0xB2, 0xBE, 0xC8, 0xE7, 0x38, 0xE6, 0xE2, 0x18, 0x7A, 0x2A, 0x2C, 0xBE, 0xBF, 0xE0, 0x55, 0xF0, 0x75, 0x67, 0x1F, 0x6F, 0x0E, 0xAB, 0x0A, 0x2C, 0x7E, 0xEA, 0x99, 0x22, 0x77, 0xEC, 0xDB, 0x6D, 0x64, 0x44, 0x9E, 0xE7, 0x71, 0x0D, 0x2C, 0x1D, 0x4F, 0xC7, 0x80, 0xBC, 0x61, 0x0E, 0x4B, 0x37, 0x77, 0x0C, 0xCF, 0xBC, 0x5F, 0xE0, 0x94, 0x7B, 0xF4, 0x79, 0xEB, 0xEC, 0xCE, 0xEE, 0x23, 0x20, 0xC6, 0xFE, 0xD9, 0xC7, 0x3D, 0x35, 0xCC, 0x70, 0x97, 0x11, 0xC6, 0x93, 0xA4, 0x7E, 0x01, 0xA7, 0xD8, 0x59, 0xED, 0xC4, 0xCF, 0xC2, 0xF2, 0x8C, 0xA7, 0x21, 0x9F, 0xCF, 0x23, 0x9E, 0x72, 0x05, 0x7B, 0x74, 0xE4, 0x6C, 0xEE, 0x1D, 0x15, 0x1D, 0xC7, 0xE8, 0xEC, 0xF5, 0xBE, 0x8F, 0xC0, 0xA1, 0x78, 0x9F, 0xC3, 0x8C, 0xF3, 0x61, 0x47, 0xF2, 0x9E, 0xC1, 0x77, 0xC3, 0x8C, 0xDF, 0x21, 0x29, 0xB0, 0x6C, 0x27, 0x7E, 0x0A, 0xEE, 0x3B, 0x76, 0x34, 0x0B, 0xD8, 0x54, 0x33, 0xE2, 0xE9, 0x31, 0x75, 0xD8, 0x8A, 0x8E, 0x0E, 0xA8, 0x92, 0x79, 0xC6, 0x54, 0xE5, 0x99, 0xFB, 0x45, 0xD4, 0xC2, 0xD4, 0x40, 0xD5, 0x39, 0x2E, 0x30, 0x21, 0x38, 0x9D, 0xF7, 0xA7, 0x89, 0xEC, 0x18, 0xE0, 0xA2, 0xDF, 0x2F, 0x8E, 0x8C, 0xA8, 0xB8, 0xE8, 0xC8, 0xAC, 0x5A, 0xE5, 0x39, 0x58, 0x04, 0xD6, 0xEF, 0x5F, 0xBA, 0xFC, 0xAD, 0x6B, 0x18, 0x07, 0xF9, 0xE4, 0x35, 0x2E, 0x70, 0x2E, 0xFA, 0x4B, 0x76, 0x70, 0x58, 0xBE, 0x97, 0xCB, 0xAA, 0x55, 0xC7, 0xE6, 0xFB, 0x4B, 0xD5, 0xCB, 0xCD, 0xEB, 0x4A, 0x55, 0x31, 0xCC, 0x5B, 0x0A, 0x2B, 0x1B, 0x37, 0x0F, 0xAC, 0x8C, 0x34, 0x63, 0x1F, 0xC1, 0x86, 0xCF, 0x71, 0x09, 0x4B, 0x70, 0x71, 0x3B, 0xC9, 0x1F, 0x74, 0xE3, 0xAA, 0x80, 0x8B, 0x6A, 0xAB, 0xDB, 0x78, 0xDE, 0x78, 0x7E, 0x5D, 0xA9, 0xEE, 0xFD, 0xE2, 0x7E, 0xC1, 0x33, 0xEF, 0x93, 0x59, 0xE9, 0x84, 0xA5, 0xEC, 0x1D, 0x99, 0x39, 0x49, 0x63, 0x0A, 0x3B, 0x95, 0xE3, 0x0C, 0x97, 0xBA, 0x5A, 0x4E, 0xC5, 0xF3, 0x76, 0x4E, 0xA7, 0x1C, 0x7D, 0x24, 0x3B, 0x4E, 0x15, 0x9E, 0x79, 0xBF, 0xC8, 0x4E, 0xD8, 0xF7, 0xA3, 0x63, 0xDC, 0x2F, 0x38, 0x1B, 0x3B, 0x26, 0xA6, 0xFB, 0x5A, 0xCE, 0x70, 0x11, 0xBB, 0xEB, 0x83, 0x57, 0x65, 0x04, 0xDC, 0x7B, 0x4C, 0xC5, 0x31, 0xCE, 0xEA, 0x5C, 0x67, 0x04, 0xEA, 0xCB, 0xD2, 0x4E, 0xB8, 0x8F, 0x3A, 0x1F, 0xB2, 0xC1, 0xBD, 0x8F, 0xAB, 0x6A, 0x7E, 0xEC, 0xB0, 0x77, 0x72, 0xE1, 0xEE, 0xC0, 0xB3, 0x1C, 0x2B, 0x85, 0xEE, 0x5B, 0x8A, 0x78, 0xC3, 0x6A, 0xFF, 0xAE, 0x19, 0x7E, 0xD1, 0x4E, 0x34, 0x51, 0x67, 0xB7, 0x13, 0xCF, 0xCD, 0xC8, 0xD6, 0x2B, 0x34, 0x79, 0xDE, 0x32, 0x12, 0x58, 0x2B, 0xD5, 0xD7, 0xB8, 0x84, 0x25, 0x3F, 0x7F, 0xDE, 0x1C, 0x43, 0xFF, 0xC1, 0x5C, 0x34, 0xE1, 0x15, 0x7D, 0x74, 0xDE, 0x2D, 0xCD, 0x88, 0x89, 0xC4, 0x94, 0x26, 0xD8, 0x0F, 0xF8, 0x0E, 0xFA, 0xBB, 0x79, 0x60, 0xF1, 0xBF, 0xEF, 0x2F, 0x55, 0x36, 0xB8, 0x97, 0x73, 0x19, 0x66, 0xF0, 0x9B, 0xC8, 0xF2, 0x35, 0x2E, 0x70, 0xBE, 0xDD, 0xA8, 0x3A, 0x60, 0x16, 0xDD, 0xB4, 0x5C, 0x21, 0xCC, 0xB8, 0x2C, 0xEB, 0xCE, 0xD6, 0x8D, 0x72, 0xAD, 0x6A, 0xBF, 0xFD, 0xF9, 0x04, 0x3F, 0x62, 0x77, 0xC6, 0x74, 0x9B, 0xFB, 0xBC, 0xF1, 0xDB, 0xB0, 0x64, 0xFF, 0x6E, 0xAE, 0x7A, 0x71, 0x75, 0xDD, 0x6B, 0x5C, 0xF0, 0x7E, 0xB6, 0x73, 0x3A, 0x86, 0x6A, 0x76, 0xAC, 0x57, 0x35, 0xED, 0x5A, 0xD9, 0x47, 0x6A, 0xB7, 0x80, 0x16, 0x9A, 0x88, 0xB3, 0xCB, 0x1E, 0x63, 0xB7, 0x7B, 0x6F, 0xFD, 0xF7, 0x17, 0xEE, 0xD6, 0x39, 0x41, 0x25, 0x4D, 0xF4, 0x3B, 0x3A, 0xC3, 0x05, 0xEF, 0x5F, 0x9E, 0xD3, 0x31, 0x58, 0xB7, 0x76, 0x92, 0xD9, 0xE0, 0x2D, 0x32, 0x9F, 0x6D, 0xD9, 0x05, 0x16, 0xD0, 0x66, 0x19, 0xB8, 0x2F, 0xAE, 0x1E, 0xE6, 0xC0, 0x12, 0x2B, 0xBF, 0xBF, 0x44, 0xFF, 0xCA, 0x6F, 0x85, 0xA5, 0x67, 0x3A, 0x06, 0xDB, 0xD7, 0xFD, 0x82, 0xCF, 0xB7, 0x54, 0x61, 0x2A, 0x92, 0xB6, 0x13, 0xEE, 0x72, 0x8E, 0x0B, 0x76, 0x0C, 0x4D, 0x62, 0xBA, 0x6A, 0x20, 0x69, 0x58, 0xC2, 0x79, 0x3B, 0x89, 0x35, 0x61, 0xB9, 0x08, 0xD1, 0x47, 0x75, 0xE5, 0x17, 0xCC, 0xF7, 0x81, 0x55, 0x57, 0x98, 0x89, 0xCA, 0x15, 0x75, 0x85, 0xEF, 0xDB, 0x29, 0xB0, 0xDC, 0x9C, 0x69, 0xF5, 0x0B, 0x3C, 0x7F, 0x46, 0xE4, 0xBD, 0xE9, 0xC3, 0x2F, 0x78, 0x6F, 0xAC, 0xA5, 0x19, 0x03, 0x6B, 0x3B, 0xCF, 0x59, 0x2E, 0x5B, 0x95, 0x34, 0x91, 0x3B, 0xC9, 0x5B, 0x3D, 0x37, 0xED, 0xDA, 0xCB, 0xCD, 0x3F, 0x36, 0xD1, 0x47, 0x47, 0x75, 0xD8, 0x63, 0x47, 0x6F, 0x71, 0x82, 0xFB, 0xB9, 0xA0, 0xE9, 0x2C, 0x97, 0xA3, 0x97, 0xF1, 0xBB, 0x10, 0x7E, 0x31, 0xCC, 0xAF, 0xA8, 0x13, 0x3D, 0xFA, 0xB8, 0xEE, 0xD3, 0x14, 0xEB, 0x3C, 0x53, 0xF7, 0x16, 0x4D, 0xC2, 0x52, 0xEC, 0x7D, 0xBF, 0xA8, 0xF2, 0xEA, 0x0A, 0xCE, 0xCF, 0x73, 0x61, 0x9E, 0x51, 0xBF, 0xBC, 0x7D, 0x17, 0xC2, 0x31, 0xD4, 0x33, 0xF0, 0xCA, 0x8C, 0x90, 0xF4, 0x43, 0x1B, 0xFA, 0x5D, 0xF4, 0x91, 0x8A, 0x66, 0x60, 0x75, 0xF6, 0xB9, 0x17, 0x02, 0x4B, 0xD4, 0xD2, 0xD9, 0x9E, 0x89, 0x79, 0xBD, 0xC2, 0x65, 0x55, 0x29, 0xF6, 0xA3, 0x49, 0x5D, 0xA9, 0xF7, 0x8D, 0xAB, 0xEA, 0x6A, 0x4B, 0xB4, 0xB3, 0xC3, 0x72, 0x8F, 0x89, 0xEE, 0x45, 0x56, 0x19, 0x58, 0xFA, 0x0A, 0x92, 0x7A, 0x66, 0x60, 0x65, 0x24, 0x2C, 0x03, 0xCB, 0xCD, 0xF7, 0x7A, 0xBB, 0x39, 0xCF, 0x3B, 0x52, 0x70, 0x2E, 0x3C, 0xD3, 0x2F, 0xC2, 0xF2, 0xF8, 0x5D, 0x59, 0x21, 0xCC, 0xEB, 0x99, 0xC7, 0x44, 0x44, 0x05, 0xEC, 0xC5, 0x8C, 0x04, 0x96, 0x5F, 0xC8, 0x95, 0xFF, 0x03,
};
// clang-format on

#define BOARD_VERSION "OneKey Boardloader 1.5.0\n"

#if defined(STM32H747xx)

#include "stm32h7xx_hal.h"

extern volatile uint32_t system_reset;

static FATFS fs_instance;
PARTITION VolToPart[FF_VOLUMES] = {
    {0, 1},
    {0, 2},
};
uint32_t *sdcard_buf = (uint32_t *)0x24000000;
uint32_t *sdcard_buf1 = (uint32_t *)0x24000000 + IMAGE_HEADER_SIZE;

void fatfs_init(void) {
  FRESULT res;
  BYTE work[FF_MAX_SS];
  MKFS_PARM mk_para = {
      .fmt = FM_FAT32,
  };

  LBA_t plist[] = {
      BOOT_EMMC_BLOCKS,
      100};  // 1G sectors for 1st partition and left all for 2nd partition

  res = f_mount(&fs_instance, "", 1);
  if (res != FR_OK) {
    if (res == FR_NO_FILESYSTEM) {
      res = f_fdisk(0, plist, work); /* Divide physical drive 0 */
      if (res) {
        display_printf("f_fdisk error%d\n", res);
      }
      res = f_mkfs("0:", &mk_para, work, sizeof(work));
      if (res) {
        display_printf("f_mkfs 0 error%d\n", res);
      }
      res = f_mkfs("1:", &mk_para, work, sizeof(work));
      if (res) {
        display_printf("fatfs Format error");
      }
      f_setlabel("Onekey Data");
    } else {
      display_printf("mount err %d\n", res);
    }
  }
}

int fatfs_check_res(void) {
  FRESULT res;
  FIL fsrc;
  res = f_mount(&fs_instance, "", 1);
  if (res != FR_OK) {
    display_printf("fatfs mount error");
    return 0;
  }
  res = f_open(&fsrc, "/res/.ONEKEY_RESOURCE", FA_READ);
  if (res != FR_OK) {
    f_unmount("");
  }
  return res;
}

secbool check_sd_card_image_contents(const image_header *const hdr,
                                     uint32_t firstskip, FIL fsrc) {
  void *data = sdcard_buf1;

  FRESULT res;
  UINT num_of_read = 0;

  res = f_read(&fsrc, data, IMAGE_CHUNK_SIZE - firstskip, &num_of_read);
  if ((num_of_read != (IMAGE_CHUNK_SIZE - firstskip)) || (res != FR_OK)) {
    f_close(&fsrc);
    return secfalse;
  }

  int remaining = hdr->codelen;
  if (remaining <= IMAGE_CHUNK_SIZE - firstskip) {
    if (sectrue != check_single_hash(hdr->hashes, data,
                                     MIN(remaining, IMAGE_CHUNK_SIZE))) {
      return secfalse;
    } else {
      return sectrue;
    }
  }

  BLAKE2S_CTX ctx;
  uint8_t hash[BLAKE2S_DIGEST_LENGTH];
  blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);

  blake2s_Update(&ctx, data, MIN(remaining, IMAGE_CHUNK_SIZE - firstskip));
  int block = 1;
  int update_flag = 1;
  remaining -= IMAGE_CHUNK_SIZE - firstskip;
  while (remaining > 0) {
    res = f_read(&fsrc, data, MIN(remaining, IMAGE_CHUNK_SIZE), &num_of_read);
    if ((num_of_read != MIN(remaining, IMAGE_CHUNK_SIZE)) || (res != FR_OK)) {
      f_close(&fsrc);
      return secfalse;
    }

    if (remaining - IMAGE_CHUNK_SIZE > 0) {
      if (block % 2) {
        update_flag = 0;
        blake2s_Update(&ctx, data, MIN(remaining, IMAGE_CHUNK_SIZE));
        blake2s_Final(&ctx, hash, BLAKE2S_DIGEST_LENGTH);
        if (0 != memcmp(hdr->hashes + (block / 2) * 32, hash,
                        BLAKE2S_DIGEST_LENGTH)) {
          return secfalse;
        }
      } else {
        blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);
        blake2s_Update(&ctx, data, MIN(remaining, IMAGE_CHUNK_SIZE));
        update_flag = 1;
      }
    } else {
      if (update_flag) {
        blake2s_Update(&ctx, data, MIN(remaining, IMAGE_CHUNK_SIZE));
        blake2s_Final(&ctx, hash, BLAKE2S_DIGEST_LENGTH);
        if (0 != memcmp(hdr->hashes + (block / 2) * 32, hash,
                        BLAKE2S_DIGEST_LENGTH)) {
          return secfalse;
        }
      } else {
        if (sectrue != check_single_hash(hdr->hashes + (block / 2) * 32, data,
                                         MIN(remaining, IMAGE_CHUNK_SIZE))) {
          return secfalse;
        }
      }
    }
    block++;
    remaining -= MIN(remaining, IMAGE_CHUNK_SIZE);
  }
  return sectrue;
}

static secbool check_sdcard(image_header *hdr) {
  FRESULT res;

  res = f_mount(&fs_instance, "", 1);
  if (res != FR_OK) {
    return secfalse;
  }
  uint64_t cap = emmc_get_capacity_in_bytes();
  if (cap < 1024 * 1024) {
    return secfalse;
  }

  memzero(sdcard_buf, IMAGE_HEADER_SIZE);

  FIL fsrc;
  UINT num_of_read = 0;

  res = f_open(&fsrc, "/boot/bootloader.bin", FA_READ);
  if (res != FR_OK) {
    return secfalse;
  }
  res = f_read(&fsrc, sdcard_buf, IMAGE_HEADER_SIZE, &num_of_read);
  if ((num_of_read != IMAGE_HEADER_SIZE) || (res != FR_OK)) {
    f_close(&fsrc);
    return secfalse;
  }

  secbool new_present = secfalse;

  new_present =
      load_image_header((const uint8_t *)sdcard_buf, BOOTLOADER_IMAGE_MAGIC,
                        BOOTLOADER_IMAGE_MAXSIZE, BOARDLOADER_KEY_M,
                        BOARDLOADER_KEY_N, BOARDLOADER_KEYS, hdr);
  if (sectrue == new_present) {
    new_present = check_sd_card_image_contents(hdr, IMAGE_HEADER_SIZE, fsrc);
  }

  f_close(&fsrc);
  return sectrue == new_present ? sectrue : secfalse;
}

static void progress_callback(int pos, int len) { display_printf("."); }

static secbool copy_sdcard(uint32_t code_len) {
  display_backlight(255);

  display_printf(BOARD_VERSION);
  display_printf("=====================\n\n");

  display_printf("new version bootloader found\n\n");

  display_printf("\n\nerasing flash:\n\n");

  // erase all flash (except boardloader)
  static const uint8_t sectors[] = {
      FLASH_SECTOR_BOOTLOADER_1,
      FLASH_SECTOR_BOOTLOADER_2,
  };

  if (sectrue !=
      flash_erase_sectors(sectors, sizeof(sectors), progress_callback)) {
    display_printf(" failed\n");
    return secfalse;
  }
  display_printf(" done\n\n");

  ensure(flash_unlock_write(), NULL);

  // copy bootloader from SD card to Flash
  display_printf("copying new bootloader from SD card\n\n");

  memzero(sdcard_buf, EMMC_BLOCK_SIZE);

  FIL fsrc;
  FRESULT res;
  UINT num_of_read;
  res = f_open(&fsrc, "/boot/bootloader.bin", FA_READ);
  if (res != FR_OK) {
    return secfalse;
  }
  int blocks = (IMAGE_HEADER_SIZE + code_len) / EMMC_BLOCK_SIZE;
  int percent = 0, percent_bak = 0;
  for (int i = 0; i < blocks; i++) {
    percent = (i * 100) / blocks;
    if (percent != percent_bak) {
      percent_bak = percent;
      display_printf("%d ", percent);
    }

    f_lseek(&fsrc, i * EMMC_BLOCK_SIZE);
    res = f_read(&fsrc, sdcard_buf, EMMC_BLOCK_SIZE, &num_of_read);
    if ((num_of_read != EMMC_BLOCK_SIZE) || (res != FR_OK)) {
      f_close(&fsrc);
      return secfalse;
    }
    if (i * EMMC_BLOCK_SIZE < FLASH_FIRMWARE_SECTOR_SIZE) {
      for (int j = 0; j < EMMC_BLOCK_SIZE / (sizeof(uint32_t) * 8); j++) {
        ensure(
            flash_write_words(FLASH_SECTOR_BOOTLOADER_1,
                              i * EMMC_BLOCK_SIZE + j * (sizeof(uint32_t) * 8),
                              (uint32_t *)&sdcard_buf[8 * j]),
            NULL);
      }
    } else {
      for (int j = 0; j < EMMC_BLOCK_SIZE / (sizeof(uint32_t) * 8); j++) {
        ensure(flash_write_words(
                   FLASH_SECTOR_BOOTLOADER_2,
                   (i - FLASH_FIRMWARE_SECTOR_SIZE / EMMC_BLOCK_SIZE) *
                           EMMC_BLOCK_SIZE +
                       j * (sizeof(uint32_t) * 8),
                   (uint32_t *)&sdcard_buf[8 * j]),
               NULL);
      }
    }
  }
  f_close(&fsrc);
  f_unlink("/boot/bootloader.bin");
  ensure(flash_lock_write(), NULL);

  display_printf("\ndone\n\n");
  display_printf("Device will be restart in 3 seconds\n");

  for (int i = 3; i >= 0; i--) {
    display_printf("%d ", i);
    hal_delay(1000);
  }
  HAL_NVIC_SystemReset();
  return sectrue;
}

#define COLOR_BL_BG COLOR_BLACK                   // background
#define COLOR_BL_FG COLOR_WHITE                   // foreground
#define COLOR_BL_FAIL RGB16(0xFF, 0x00, 0x00)     // red
#define COLOR_BL_DONE RGB16(0x00, 0xAE, 0x0B)     // green
#define COLOR_BL_PROCESS RGB16(0x4A, 0x90, 0xE2)  // blue
#define COLOR_BL_GRAY RGB16(0x99, 0x99, 0x99)     // gray

typedef enum {
  SCREEN_TEST = 0,
  TOUCH_TEST,
  SE_TEST,
  SPI_FLASH_TEST,
  EMMC_TEST,
  SDRAM_TEST,
  CAMERA_TEST,
  MOTOR_TEST,
  BLE_TEST,
  FP_TEST,
  NFC_TEST,
  FLASHLED_TEST,
  TEST_NUMS
} TEST_ITEM;

static uint16_t screen_bg[TEST_NUMS + 1];

static void ui_generic_confirm_simple(const char *msg) {
  if (msg == NULL) return;
  display_clear();
  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, msg, -1, FONT_NORMAL,
                      COLOR_WHITE, COLOR_BLACK);

  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);
}

static bool ui_response(void) {
  for (;;) {
    uint32_t evt = touch_click();
    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);

    if (!evt) {
      continue;
    }
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      return false;
    }
    // clicked on Confirm button
    if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
        y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
      return true;
    }
  }
}

void screen_test(void) {
  display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_RED);
  display_text_center(MAX_DISPLAY_RESX / 2, MAX_DISPLAY_RESY / 2,
                      "TOUCH SCREEN", -1, FONT_NORMAL, COLOR_BL_FG, COLOR_RED);
  while (!touch_click()) {
  }

  display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_GREEN);
  display_text_center(MAX_DISPLAY_RESX / 2, MAX_DISPLAY_RESY / 2,
                      "TOUCH SCREEN", -1, FONT_NORMAL, COLOR_BL_FG,
                      COLOR_GREEN);
  while (!touch_click()) {
  }
  display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_BLUE);
  display_text_center(MAX_DISPLAY_RESX / 2, MAX_DISPLAY_RESY / 2,
                      "TOUCH SCREEN", -1, FONT_NORMAL, COLOR_BL_FG, COLOR_BLUE);
  while (!touch_click()) {
  }
  display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_BLACK);
  display_text_center(MAX_DISPLAY_RESX / 2, MAX_DISPLAY_RESY / 2,
                      "TOUCH SCREEN", -1, FONT_NORMAL, COLOR_BL_FG,
                      COLOR_BLACK);
  while (!touch_click()) {
  }
  display_bar(0, 0, MAX_DISPLAY_RESX, MAX_DISPLAY_RESY, COLOR_WHITE);
  display_text_center(MAX_DISPLAY_RESX / 2, MAX_DISPLAY_RESY / 2,
                      "TOUCH SCREEN", -1, FONT_NORMAL, COLOR_BLACK,
                      COLOR_WHITE);
  while (!touch_click()) {
  }

  ui_generic_confirm_simple("SCREEN PASS?");
  if (ui_response()) {
    screen_bg[SCREEN_TEST] = COLOR_GREEN;
  } else {
    screen_bg[SCREEN_TEST] = COLOR_RED;
  }
}

void touch_input_test(void) {
  display_clear();
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 6; j++) {
      display_bar_radius(j * 80, (j % 2) * 80 + i * 160, 80, 80, COLOR_RED,
                         COLOR_WHITE, 16);
    }
  }
  uint32_t pos = 0;
  for (;;) {
    uint32_t evt = touch_read();
    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);

    if (!evt) {
      continue;
    }

    for (int i = 0; i < 5; i++) {
      for (int j = 0; j < 6; j++) {
        if (x > (j * 80) && x < (j * 80 + 80) && y > ((j % 2) * 80 + i * 160) &&
            y < ((j % 2) * 80 + i * 160 + 80)) {
          display_bar_radius(j * 80, (j % 2) * 80 + i * 160, 80, 80,
                             COLOR_GREEN, COLOR_WHITE, 16);
          pos |= 1 << (6 * i + j);
        }
        if (pos == 0x3FFFFFFF) {
          screen_bg[TOUCH_TEST] = COLOR_GREEN;
          return;
        }
      }
    }
  }
}

void camera_test(void) {
  display_clear();

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "CAMERA TEST", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);

  while (1) {
    camera_capture_start();
    if (camera_capture_done()) {
      dma2d_copy_buffer((uint32_t *)CAM_BUF_ADDRESS,
                        (uint32_t *)FMC_SDRAM_LTDC_BUFFER_ADDRESS, 80, 0, WIN_W,
                        WIN_H);
    }
    uint32_t evt = touch_click();

    if (!evt) {
      continue;
    }

    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      screen_bg[CAMERA_TEST] = COLOR_RED;
      return;
    }
    // clicked on Confirm button
    if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
        y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
      screen_bg[CAMERA_TEST] = COLOR_GREEN;
      return;
    }
  }
}

static void _motor_test(void) {
  display_clear();

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "MOTOR TEST", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);

  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);
  while (1) {
    
 
      HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, GPIO_PIN_RESET);
      dwt_delay_us(2083);
      HAL_GPIO_WritePin(GPIOK, GPIO_PIN_2, GPIO_PIN_SET);
      dwt_delay_us(767);
    
   
    uint32_t evt = touch_click();

    if (!evt) {
      continue;
    }

    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      screen_bg[MOTOR_TEST] = COLOR_RED;
      return;
    }
    // clicked on Confirm button
    if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
        y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
      screen_bg[MOTOR_TEST] = COLOR_GREEN;
      return;
    }
  }
}

static void _fp_test(void) {
  display_clear();

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "FP TEST", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);
  uint8_t image_data[88 * 112 + 2];
  int ret = 0;
  bool touched = false;
  while (1) {
    if (FpsDetectFinger() == 1) {
      ret = FpsGetImageData(image_data);
      if (ret == 0) {
        display_fp(196, 10, 88, 112, image_data);
        touched = true;
      }
    }
    if (touched) {
      uint32_t evt = touch_click();

      if (!evt) {
        continue;
      }

      uint16_t x = touch_unpack_x(evt);
      uint16_t y = touch_unpack_y(evt);
      // clicked on Cancel button
      if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
          y < DISPLAY_RESY - 160 + 64) {
        screen_bg[FP_TEST] = COLOR_RED;
        return;
      }
      // clicked on Confirm button
      if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
          y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
        screen_bg[FP_TEST] = COLOR_GREEN;
        return;
      }
    }
  }
}

static void _nfc_test(void){

  display_clear();

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "NFC POLLING CARD", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  // display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
  //                    COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  // display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
  //              COLOR_WHITE, COLOR_GREEN);
  nfc_pwr_ctl(true);
  while(1){
    if (nfc_poll_card()==NFC_STATUS_OPERACTION_SUCCESS){
        if(nfc_select_aid((uint8_t *)"\xD1\x56\x00\x01\x32\x83\x40\x01",8)==NFC_STATUS_OPERACTION_SUCCESS){
            screen_bg[NFC_TEST] = COLOR_GREEN;
            nfc_pwr_ctl(false);
            return;
          }

          if(nfc_select_aid((uint8_t *)"\x6f\x6e\x65\x6b\x65\x79\x2e\x62\x61\x63\x6b\x75\x70\x01",14)==NFC_STATUS_OPERACTION_SUCCESS){
            screen_bg[NFC_TEST] = COLOR_GREEN;
            nfc_pwr_ctl(false);
            return;
          }      
        }
    uint32_t evt = touch_click();

    if (!evt) {
      continue;
    }

    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      screen_bg[NFC_TEST] = COLOR_RED;
      nfc_pwr_ctl(false);
      return;
    }
    // // clicked on Confirm button
    // if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
    //     y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
    //   screen_bg[NFC_TEST] = COLOR_GREEN;
    //   return;
    // }
  }
}

void flashled_test(void){
  uint32_t start,current;
  start = current = HAL_GetTick();
  uint8_t value=1;
  display_clear();

  display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "FLASHLED TEST", -1,
                      FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
  display_bar_radius(32, DISPLAY_RESY - 160, 128, 64, COLOR_RED, COLOR_BLACK,
                     16);
  display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
                     COLOR_GREEN, COLOR_BLACK, 16);
  display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
               COLOR_RED);
  display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1, FONT_NORMAL,
               COLOR_WHITE, COLOR_GREEN);
  ble_set_flashled(value);
  while(1){
    current = HAL_GetTick();
    if(current - start > 1000){
      start = current;
      value = value?0:1;
      ble_set_flashled(value);
    }
    uint32_t evt = touch_click();

    if (!evt) {
      continue;
    }

    uint16_t x = touch_unpack_x(evt);
    uint16_t y = touch_unpack_y(evt);
    // clicked on Cancel button
    if (x >= 32 && x < 32 + 128 && y > DISPLAY_RESY - 160 &&
        y < DISPLAY_RESY - 160 + 64) {
      screen_bg[FLASHLED_TEST] = COLOR_RED;
      ble_set_flashled(0);
      return;
    }
    // clicked on Confirm button
    if (x >= DISPLAY_RESX - 32 - 128 && x < DISPLAY_RESX - 32 &&
        y > DISPLAY_RESY - 160 && y < DISPLAY_RESY - 160 + 64) {
      screen_bg[FLASHLED_TEST] = COLOR_GREEN;
      ble_set_flashled(0);
      return;
    }
  }
}

void se_test(void) {

  if (se_get_state()!=0) {
    screen_bg[SE_TEST] = COLOR_RED;
  } else {
    screen_bg[SE_TEST] = COLOR_GREEN;
  }
}

int spi_flash_test(void) {
  // if (qspi_flash_read_id() == 0) {
  //   screen_bg[SPI_FLASH_TEST] = COLOR_RED;
  // } else {
  //   screen_bg[SPI_FLASH_TEST] = COLOR_GREEN;
  // }
  char show_tip[64] = {0};
  volatile uint32_t write_start_time, write_end_time;
  write_start_time = HAL_GetTick();

  uint8_t flash_data[2048] = {0};
  uint8_t test_data[2048] = {0};
  for (uint32_t i = 0; i < sizeof(test_data); i++) {
    test_data[i] = i;
  }

  for (uint32_t address = 0; address < (1 * 1024 * 1024);
       address += QSPI_SECTOR_SIZE) {
    ensure(qspi_flash_erase_block_64k(address)==HAL_OK?sectrue:secfalse, NULL);

    for (uint32_t offset = 0; offset < QSPI_SECTOR_SIZE;
         offset += sizeof(flash_data)) {
      ensure(qspi_flash_read_buffer(flash_data, address + offset, sizeof(flash_data))==HAL_OK?sectrue:secfalse, NULL);
      for (uint32_t i = 0; i < sizeof(flash_data); i++) {
        if (flash_data[i] != 0xFF) {
          screen_bg[SPI_FLASH_TEST] = COLOR_RED;
          ensure(secfalse,"erase compare failed");
          return 0;
        }
      }
    }

    for (uint32_t offset = 0; offset < QSPI_SECTOR_SIZE;
         offset += sizeof(test_data)) {
      ensure(qspi_flash_write_buffer_unsafe(test_data, address + offset,
                                     sizeof(test_data))==HAL_OK?sectrue:secfalse, NULL);
      memset(flash_data, 0x00, sizeof(flash_data));
      ensure(qspi_flash_read_buffer(flash_data, address + offset, sizeof(flash_data))==HAL_OK?sectrue:secfalse, NULL);
      for (uint32_t i = 0; i < sizeof(flash_data); i++) {
        if (flash_data[i] != i % 256) {
          // ensure(secfalse,"read compare failed");
          display_clear();
          display_printf("write compare failed,address:%d,offset:%d,loc:%d\n",(unsigned)address,(unsigned)offset,(unsigned)i);
          display_printf("%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,",flash_data[0],flash_data[1],flash_data[2],flash_data[3],flash_data[4],flash_data[5],flash_data[6],flash_data[7],flash_data[8],flash_data[9],flash_data[10],flash_data[11],flash_data[12],flash_data[13],flash_data[14],flash_data[15],flash_data[16]);
          while(1);
          screen_bg[SPI_FLASH_TEST] = COLOR_RED;
          return 0;
        }
      }
    }

    qspi_flash_erase_block_64k(address);

    display_bar(0, 130, 480, 30, COLOR_BL_BG);
    mini_snprintf(show_tip, sizeof(show_tip), "SPI TEST... %d%%",
                  (unsigned int)(address * 100) / (1024 * 1024));
    display_text_center(DISPLAY_RESX / 2, 160, show_tip, -1, FONT_NORMAL,
                        COLOR_BL_FG, COLOR_BL_BG);
  }

  display_bar(0, 130, 480, 30, COLOR_BL_BG);
  display_text_center(DISPLAY_RESX / 2, 160, "SPI TEST... 100%", -1,
                      FONT_NORMAL, COLOR_BL_FG, COLOR_BL_BG);

  write_end_time = HAL_GetTick();
  screen_bg[SPI_FLASH_TEST] = COLOR_GREEN;
  return write_end_time - write_start_time;
}

void emmc_test(void) {
  if (emmc_get_capacity_in_bytes() == 0) {
    screen_bg[EMMC_TEST] = COLOR_RED;
  } else {
    screen_bg[EMMC_TEST] = COLOR_GREEN;
  }
}

void sdram_test(void) {
  uint32_t i, j;
  uint32_t write_start_time, write_end_time, read_start_time, read_end_time;
  uint32_t *buffer;

  // char write_info[128] = {0};
  // char read_info[128] = {0};

  write_start_time = HAL_GetTick();
  j = 0;
  buffer = (uint32_t *)(FMC_SDRAM_ADDRESS + 1024 * 1024);

  for (i = ((1024 * 1024 / 4) - (1024 * 1024 / 128)); i > 0; i--) {
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;

    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;

    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;

    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
    *buffer++ = j++;
  }
  write_end_time = HAL_GetTick();

  j = 0;
  buffer = (uint32_t *)(FMC_SDRAM_ADDRESS + 1024 * 1024);
  for (i = 0; i < ((1024 * 1024 * 8) - (1024 * 1024 / 4)); i++) {
    if (*buffer++ != j++) {
      screen_bg[SDRAM_TEST] = COLOR_RED;
      return;
    }
  }

  volatile uint32_t data;
  (void)data;
  buffer = (uint32_t *)FMC_SDRAM_ADDRESS;

  read_start_time = HAL_GetTick();
  for (i = 1024 * 1024 / 4; i > 0; i--) {
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;

    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;

    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;

    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
    data = *buffer++;
  }
  read_end_time = HAL_GetTick();
  // display_clear();

  // snprintf(write_info, sizeof(write_info), "write:time = %dms,speed= %dMB/s",
  //          (unsigned)(write_end_time - write_start_time),
  //          (unsigned)((32 * 1000) / (write_end_time - write_start_time)));
  // snprintf(read_info, sizeof(read_info), "read:time = %dms,speed= %dMB/s",
  //          (unsigned)(read_end_time - read_start_time),
  //          (unsigned)((32 * 1000) / (read_end_time - read_start_time)));

  // display_text(0, 100, write_info, -1, FONT_NORMAL, COLOR_WHITE,
  // COLOR_BLACK); display_text(0, 150, read_info, -1, FONT_NORMAL, COLOR_WHITE,
  // COLOR_BLACK); display_bar_radius(32, DISPLAY_RESY - 160, 128, 64,
  // COLOR_RED, COLOR_BLACK,
  //                    16);
  // display_bar_radius(DISPLAY_RESX - 32 - 128, DISPLAY_RESY - 160, 128, 64,
  //                    COLOR_GREEN, COLOR_BLACK, 16);
  // display_text(80, DISPLAY_RESY - 120, "No", -1, FONT_NORMAL, COLOR_WHITE,
  //              COLOR_RED);
  // display_text(DISPLAY_RESX - 118, DISPLAY_RESY - 120, "Yes", -1,
  // FONT_NORMAL,
  //              COLOR_WHITE, COLOR_GREEN);

  // if (ui_response()) {
  //   screen_bg[SDRAM_TEST] = COLOR_GREEN;
  // } else {
  //   screen_bg[SDRAM_TEST] = COLOR_RED;
  // }

  if ((31 * 1000) / (write_end_time - write_start_time) > 400 &&
      (32 * 1000) / (read_end_time - read_start_time) > 200) {
    screen_bg[SDRAM_TEST] = COLOR_GREEN;
  } else {
    screen_bg[SDRAM_TEST] = COLOR_RED;
  }
}

void ble_test(void) {
  if (!ble_name_state()) {
    ble_cmd_req(BLE_VER, BLE_VER_ADV);
    hal_delay(5);
  }

  if (!ble_battery_state()) {
    ble_cmd_req(BLE_PWR, BLE_PWR_EQ);
    hal_delay(5);
  }
}

uint16_t *p_test_result = (uint16_t *)0x08020000;
uint16_t test_result[16];

#define OFFSET_Y 80
#define FONT_OFFSET 30

void test_menu(void) {
  uint8_t secotrs[1];
  secotrs[0] = 1;

  memcpy(test_result, screen_bg, sizeof(screen_bg));

  ensure(flash_erase_sectors(secotrs, 1, NULL), "erase data sector 1");
  ensure(flash_unlock_write(), NULL);
  ensure(flash_write_words(1, 0, (uint32_t *)test_result), "write test result");
  ensure(flash_lock_write(), NULL);

  display_clear();
  display_text_center(DISPLAY_RESX / 2, 100, "DEVICE TEST", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 6; j++) {
      display_bar_radius((i + 1) * 60 + i * 150, 150 + j * OFFSET_Y, 150, 60,
                         screen_bg[j * 2 + i], COLOR_BLACK, 16);
    }
  }
  display_text_center(135, FONT_OFFSET+OFFSET_Y*2, "SCREEN", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[SCREEN_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*2, "TOUCH", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[TOUCH_TEST]);
  display_text_center(135,  FONT_OFFSET+OFFSET_Y*3, "SE", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[SE_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*3, "SPI-FLASH", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[SPI_FLASH_TEST]);
  display_text_center(135,  FONT_OFFSET+OFFSET_Y*4, "EMMC", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[EMMC_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*4, "SDRAM", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[SDRAM_TEST]);
  display_text_center(135,  FONT_OFFSET+OFFSET_Y*5, "CAMERA", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[CAMERA_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*5, "MOTOR", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[MOTOR_TEST]);
  display_text_center(135,  FONT_OFFSET+OFFSET_Y*6, "BLE", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[BLE_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*6, "FP", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[FP_TEST]);
  display_text_center(135,  FONT_OFFSET+OFFSET_Y*7, "NFC", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[NFC_TEST]);
  display_text_center(345,  FONT_OFFSET+OFFSET_Y*7, "FLASHLED", -1, FONT_NORMAL, COLOR_BL_FG,
                      screen_bg[FLASHLED_TEST]);
}

uint16_t pos_x, pos_y;
uint32_t test_ui_response(void) {
  uint32_t evt = touch_click();
  pos_x = touch_unpack_x(evt);
  pos_y = touch_unpack_y(evt);

  if (!evt) {
    return 0xFF;
  }

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 6; j++) {
      if (pos_x > ((i + 1) * 60 + i * 150) &&
          pos_x < ((i + 1) * 60 + (i + 1) * 150) && pos_y > (150 + j * OFFSET_Y) &&
          pos_y < (150 + j * OFFSET_Y + 80)) {
        return j * 2 + i;
      }
    }
  }
  return 0xFF;
}

void ble_response(void) {
  static bool flag = true;
  char battery_str[32] = {0};
  if (ble_name_state()) {
    display_text(0, 700, ble_get_name(), -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_BG);
    screen_bg[BLE_TEST] = COLOR_GREEN;
    if (flag) {
      flag = false;
      test_menu();
    }
  }
  if (ble_battery_state()) {
    mini_snprintf(battery_str, sizeof(battery_str), "battery %d%%",
                  battery_cap);
    display_text(0, 730, battery_str, -1, FONT_NORMAL, COLOR_BL_FG,
                 COLOR_BL_BG);
  }
}

int main(void) {
  volatile uint32_t startup_mode_flag = *STAY_IN_FLAG_ADDR;

  reset_flags_reset();

  periph_init();

  /* Enable the CPU Cache */
  cpu_cache_enable();

  system_clock_config();

  rng_init();

  flash_option_bytes_init();

  clear_otg_hs_memory();

  flash_otp_init();

  gpio_init();

  sdram_init();

  mpu_config();

  lcd_init(DISPLAY_RESX, DISPLAY_RESY, LCD_PIXEL_FORMAT_RGB565);
  display_clear();
  lcd_pwm_init();

    qspi_flash_init();
  qspi_flash_config();
  // qspi_flash_memory_mapped();


  if (startup_mode_flag != STAY_IN_BOARDLOADER_FLAG &&
      startup_mode_flag != STAY_IN_BOOTLOADER_FLAG) {
    display_image((DISPLAY_RESX - 128) / 2, 190, 128, 128, toi_icon_onekey + 12,
                  sizeof(toi_icon_onekey) - 12);

    display_image((DISPLAY_RESX - 140) / 2, DISPLAY_RESY - 120, 140, 30,
                  toi_icon_safeos + 12, sizeof(toi_icon_safeos) - 12);
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY - 64,
                        "Powered by OneKey", -1, FONT_NORMAL, COLOR_GRAY,
                        COLOR_BLACK);

#if !PRODUCTION
    display_text_center(DISPLAY_RESX / 2, DISPLAY_RESY / 2, "TEST VERSION", -1,
                        FONT_NORMAL, COLOR_RED, COLOR_BLACK);
#endif
  }

  touch_init();

  emmc_init();
  thd89_io_init();
  camera_io_init();
  // se
  thd89_reset();
  thd89_init();
  camera_init();
  ble_usart_init();
  motor_init();
  dwt_init();

  nfc_init();

  fingerprint_init();

  // fatfs_init();
  memcpy(test_result, p_test_result, 32);
  for (int i = 0; i < TEST_NUMS; i++) {
    if (test_result[i] == (COLOR_RED)) {
      screen_bg[i] = COLOR_RED;
    } else if (test_result[i] == (COLOR_GREEN)) {
      screen_bg[i] = COLOR_GREEN;
    } else {
      screen_bg[i] = COLOR_BL_GRAY;
    }
  }

  screen_bg[TEST_NUMS] = COLOR_BLACK;

  display_clear();
  display_text_center(DISPLAY_RESX / 2, 100, "AUTO TEST", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  display_text_center(DISPLAY_RESX / 2, 130, "SE TEST...", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  se_test();

  display_text_center(DISPLAY_RESX / 2, 160, "SPI TEST...", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  spi_flash_test();

  display_text_center(DISPLAY_RESX / 2, 190, "EMMC TEST...", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  emmc_test();

  display_text_center(DISPLAY_RESX / 2, 220, "SDRAM TEST...", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  sdram_test();

  display_text_center(DISPLAY_RESX / 2, 250, "BLE TEST...", -1, FONT_NORMAL,
                      COLOR_BL_FG, COLOR_BL_BG);

  ble_test();

  uint32_t button = 0;

  test_menu();
  while (1) {
    ble_uart_poll();
    ble_response();
    button = test_ui_response();
    if (button != 0xFF) {
      switch (button) {
        case SCREEN_TEST:
          screen_test();
          break;
        case TOUCH_TEST:
          touch_input_test();
          break;
        case SE_TEST:
          se_test();
          break;
        case SPI_FLASH_TEST:
          spi_flash_test();
          break;
        case EMMC_TEST:
          emmc_test();
          break;
        case SDRAM_TEST:
          sdram_test();
          break;
        case CAMERA_TEST:
          camera_test();
          break;
        case MOTOR_TEST:
          _motor_test();
          break;
        case BLE_TEST:
          ble_test();
          break;
        case FP_TEST:
          _fp_test();
          break;
        case NFC_TEST:
          _nfc_test();
          break;
        case FLASHLED_TEST:
          flashled_test();
          break;
        default:
          break;
      }
      test_menu();
    }
  }

  uint32_t mode = 0;
  bool factory_mode = false;

  if (startup_mode_flag == STAY_IN_BOARDLOADER_FLAG) {
    mode = BOARD_MODE;
    *STAY_IN_FLAG_ADDR = 0;
  } else if (fatfs_check_res() != 0) {
    mode = BOARD_MODE;
    factory_mode = true;
  }
  if (startup_mode_flag == STAY_IN_BOOTLOADER_FLAG) {
    mode = BOOT_MODE;
  }

  if (!mode) {
    ble_usart_init();
    bool touched = false;
    uint32_t touch_data, x_start, y_start, x_mov, y_mov;

    touch_data = x_start = y_start = x_mov = y_mov = 0;

    for (int timer = 0; timer < 1600; timer++) {
      ble_uart_poll();

      if (ble_power_button_state() == 2) {
        if (touched) {
          mode = BOARD_MODE;
        } else {
          mode = BOOT_MODE;
        }
        break;
      }
      touch_data = touch_read();
      if (touch_data != 0) {
        if (touch_data & TOUCH_START) {
          x_start = x_mov = (touch_data >> 12) & 0xFFF;
          y_start = y_mov = touch_data & 0xFFF;
        }

        if (touch_data & TOUCH_MOVE) {
          x_mov = (touch_data >> 12) & 0xFFF;
          y_mov = touch_data & 0xFFF;
        }

        if ((abs(x_start - x_mov) > 100) || (abs(y_start - y_mov) > 100)) {
          touched = true;
        }
      }

      hal_delay(1);
    }
    ble_usart_irq_disable();
    display_bar(160, 352, 160, 4, COLOR_BLACK);
  }

  if (mode == BOARD_MODE) {
    if (!factory_mode) {
      f_chmod("/res/", AM_RDO | AM_SYS | AM_HID, AM_RDO | AM_SYS | AM_HID);
    }
  } else {
    f_chmod("/res/", 0, AM_RDO | AM_SYS | AM_HID);
  }

  if (mode == BOARD_MODE) {
    display_printf(BOARD_VERSION);
    display_printf("USB Mass Storage Mode\n");
    display_printf("======================\n\n");
    usb_msc_init();
    while (1) {
      if (system_reset == 1) {
        hal_delay(5);
        restart();
      }
    }
  }

  if (mode == BOOT_MODE) {
    *STAY_IN_FLAG_ADDR = STAY_IN_BOOTLOADER_FLAG;
    SCB_CleanDCache();
  }

  image_header hdr_inner, hdr_sd;

  const uint8_t sectors[] = {
      FLASH_SECTOR_BOOTLOADER_1,
      FLASH_SECTOR_BOOTLOADER_2,
  };
  secbool boot_hdr = secfalse, boot_present = secfalse;

  boot_hdr = load_image_header((const uint8_t *)BOOTLOADER_START,
                               BOOTLOADER_IMAGE_MAGIC, BOOTLOADER_IMAGE_MAXSIZE,
                               BOARDLOADER_KEY_M, BOARDLOADER_KEY_N,
                               BOARDLOADER_KEYS, &hdr_inner);

  if (sectrue == boot_hdr) {
    boot_present = check_image_contents(&hdr_inner, IMAGE_HEADER_SIZE, sectors,
                                        sizeof(sectors));
  }

  if (sectrue == check_sdcard(&hdr_sd)) {
    if (sectrue == boot_hdr) {
      if (memcmp(&hdr_sd.version, &hdr_inner.version, 4) >= 0) {
        return copy_sdcard(hdr_sd.codelen) == sectrue ? 0 : 3;
      }
    } else {
      return copy_sdcard(hdr_sd.codelen) == sectrue ? 0 : 3;
    }
  }

  if (boot_present == secfalse) {
    display_printf(BOARD_VERSION);
    display_printf("USB Mass Storage Mode\n");
    display_printf("======================\n\n");
    usb_msc_init();
    while (1) {
      if (system_reset == 1) {
        hal_delay(5);
        restart();
      }
    }
  }

  jump_to(BOOTLOADER_START + IMAGE_HEADER_SIZE);

  return 0;
}

#else

// we use SRAM as SD card read buffer (because DMA can't access the CCMRAM)
extern uint32_t sram_start[];
#define sdcard_buf sram_start

#if defined TREZOR_MODEL_T
static uint32_t check_sdcard(void) {
  if (sectrue != sdcard_power_on()) {
    return 0;
  }

  uint64_t cap = sdcard_get_capacity_in_bytes();
  if (cap < 1024 * 1024) {
    sdcard_power_off();
    return 0;
  }

  memzero(sdcard_buf, IMAGE_HEADER_SIZE);

  const secbool read_status =
      sdcard_read_blocks(sdcard_buf, 0, IMAGE_HEADER_SIZE / SDCARD_BLOCK_SIZE);

  sdcard_power_off();

  image_header hdr;

  if ((sectrue == read_status) &&
      (sectrue ==
       load_image_header((const uint8_t *)sdcard_buf, BOOTLOADER_IMAGE_MAGIC,
                         BOOTLOADER_IMAGE_MAXSIZE, BOARDLOADER_KEY_M,
                         BOARDLOADER_KEY_N, BOARDLOADER_KEYS, &hdr))) {
    return hdr.codelen;
  } else {
    return 0;
  }
}

static void progress_callback(int pos, int len) { display_printf("."); }

static secbool copy_sdcard(void) {
  display_backlight(255);

  display_printf("Trezor Boardloader\n");
  display_printf("==================\n\n");

  display_printf("bootloader found on the SD card\n\n");
  display_printf("applying bootloader in 10 seconds\n\n");
  display_printf("unplug now if you want to abort\n\n");

  uint32_t codelen;

  for (int i = 10; i >= 0; i--) {
    display_printf("%d ", i);
    hal_delay(1000);
    codelen = check_sdcard();
    if (0 == codelen) {
      display_printf("\n\nno SD card, aborting\n");
      return secfalse;
    }
  }

  display_printf("\n\nerasing flash:\n\n");

  // erase all flash (except boardloader)
  static const uint8_t sectors[] = {
      FLASH_SECTOR_STORAGE_1,
      FLASH_SECTOR_STORAGE_2,
      3,
      FLASH_SECTOR_BOOTLOADER,
      FLASH_SECTOR_FIRMWARE_START,
      7,
      8,
      9,
      10,
      FLASH_SECTOR_FIRMWARE_END,
      FLASH_SECTOR_UNUSED_START,
      13,
      14,
      FLASH_SECTOR_UNUSED_END,
      FLASH_SECTOR_FIRMWARE_EXTRA_START,
      18,
      19,
      20,
      21,
      22,
      FLASH_SECTOR_FIRMWARE_EXTRA_END,
  };
  if (sectrue !=
      flash_erase_sectors(sectors, sizeof(sectors), progress_callback)) {
    display_printf(" failed\n");
    return secfalse;
  }
  display_printf(" done\n\n");

  ensure(flash_unlock_write(), NULL);

  // copy bootloader from SD card to Flash
  display_printf("copying new bootloader from SD card\n\n");

  ensure(sdcard_power_on(), NULL);

  memzero(sdcard_buf, SDCARD_BLOCK_SIZE);

  for (int i = 0; i < (IMAGE_HEADER_SIZE + codelen) / SDCARD_BLOCK_SIZE; i++) {
    ensure(sdcard_read_blocks(sdcard_buf, i, 1), NULL);
    for (int j = 0; j < SDCARD_BLOCK_SIZE / sizeof(uint32_t); j++) {
      ensure(flash_write_word(FLASH_SECTOR_BOOTLOADER,
                              i * SDCARD_BLOCK_SIZE + j * sizeof(uint32_t),
                              sdcard_buf[j]),
             NULL);
    }
  }

  sdcard_power_off();
  ensure(flash_lock_write(), NULL);

  display_printf("\ndone\n\n");
  display_printf("Unplug the device and remove the SD card\n");

  return sectrue;
}
#endif

int main(void) {
  reset_flags_reset();

  // need the systick timer running before many HAL operations.
  // want the PVD enabled before flash operations too.
  periph_init();

  if (sectrue != flash_configure_option_bytes()) {
    // display is not initialized so don't call ensure
    const secbool r =
        flash_erase_sectors(STORAGE_SECTORS, STORAGE_SECTORS_COUNT, NULL);
    (void)r;
    return 2;
  }

  clear_otg_hs_memory();

  display_init();
  display_clear();

#if defined TREZOR_MODEL_T
  sdcard_init();

  if (check_sdcard()) {
    return copy_sdcard() == sectrue ? 0 : 3;
  }
#endif

  image_header hdr;

  ensure(load_image_header((const uint8_t *)BOOTLOADER_START,
                           BOOTLOADER_IMAGE_MAGIC, BOOTLOADER_IMAGE_MAXSIZE,
                           BOARDLOADER_KEY_M, BOARDLOADER_KEY_N,
                           BOARDLOADER_KEYS, &hdr),
         "invalid bootloader header");

  const uint8_t sectors[] = {
      FLASH_SECTOR_BOOTLOADER,
  };
  ensure(check_image_contents(&hdr, IMAGE_HEADER_SIZE, sectors, 1),
         "invalid bootloader hash");

  jump_to(BOOTLOADER_START + IMAGE_HEADER_SIZE);

  return 0;
}

#endif
