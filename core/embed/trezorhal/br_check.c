/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (C) 2018 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>

#include "blake2s.h"
#include "br_check.h"
#include "flash.h"
#include "sha2.h"

static char boardloader_version[32] = {0};
#define FW_CHUNK_SIZE 65536

#if PRODUCTION

static int onekey_known_boardloader(const uint8_t *hash) {
  if (0 ==
      memcmp(hash,
             "\x5b\xd1\xcd\x80\xa3\xbd\xa4\xa2\xf0\x84\x89\xec\x09\x9a\xad\xbb"
             "\xa6\x52\xd9\xb3\x1c\x1c\xfc\x90\xb1\x93\x43\x66\x1e\x8b\x8d\x6c",
             32)) {
    memcpy(boardloader_version, "1.0.0", strlen("1.0.0"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x01\x0f\x28\xc2\x08\x2d\x90\xce\x18\xc0\x35\x2d\x0d\xb9\x0d\xa7"
             "\x6c\x61\x00\xf9\xfb\x05\x5b\xad\xef\x37\x53\x8a\xea\x44\x50\x78",
             32)) {
    memcpy(boardloader_version, "1.1.0", strlen("1.1.0"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x3f\x25\xa2\x93\x48\xf8\x8b\x25\x34\xc0\x2d\x6c\x30\xc2\xc4\xb5"
             "\x61\xf8\xeb\xd7\x2d\xc7\xb0\xab\x76\xd4\xc9\x42\xaa\x2c\x57\x4b",
             32)) {
    memcpy(boardloader_version, "1.2.0", strlen("1.2.0"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x1b\x1a\x80\xdc\x3e\xbd\x83\x6d\x16\x56\x4f\x80\xa3\x47\x61\xdf"
             "\x41\xdf\xf7\x16\x68\xf4\x38\x1d\x8b\x66\xf2\x87\x47\xfa\xde\x51",
             32)) {
    memcpy(boardloader_version, "1.3.1", strlen("1.3.1"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\xff\xf7\x0b\x68\xa0\x1c\x79\xf0\x59\xfc\x82\x0f\x4f\x73\xac\x4e"
             "\xe7\x76\x56\x5a\x14\x1b\x0b\x1d\x24\xfc\xac\x83\x76\x6e\x8b\xd2",
             32)) {
    memcpy(boardloader_version, "1.4.0", strlen("1.4.0"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x40\xd6\xd1\x77\x07\x93\x84\x75\x3a\x9a\x93\xfa\x7d\xe9\x96\x6d"
             "\xd8\x65\x23\xef\x6f\xbc\x03\x00\x10\x43\x94\xd8\x5c\x90\xbf\x5e",
             32)) {
    memcpy(boardloader_version, "1.5.0", strlen("1.5.0"));
    return 1;
  }
  memcpy(boardloader_version, "unknown boardloader",
         strlen("unknown boardloader"));
  return 1;
}

#else

static int onekey_known_boardloader(const uint8_t *hash) {
  if (0 ==
      memcmp(hash,
             "\xc5\x05\x0c\x19\x42\xe4\x41\x60\x2b\xe3\xa2\x31\xd0\x28\x95\xcf"
             "\x24\xeb\x38\xcd\xe5\xdf\xb1\x37\x94\x63\x47\xea\x00\x88\x6f\x69",
             32)) {
    memcpy(boardloader_version, "1.5.0", strlen("1.5.0"));
    return 1;
  }
  memcpy(boardloader_version, "unknown boardloader",
         strlen("unknown boardloader"));
  return 1;
}

#endif

char *get_boardloader_version(void) {
  uint8_t hash[32] = {0};

  if (strlen(boardloader_version) == 0) {
    sha256_Raw((uint8_t *)BOARDLOADER_START,
               BOOTLOADER_START - BOARDLOADER_START, hash);
    sha256_Raw(hash, 32, hash);

    onekey_known_boardloader(hash);
  }

  return boardloader_version;
}

uint8_t *get_boardloader_hash(void) {
  static uint8_t boardloader_hash[32] = {0};

  sha256_Raw((uint8_t *)BOARDLOADER_START, BOOTLOADER_START - BOARDLOADER_START,
             boardloader_hash);
  sha256_Raw(boardloader_hash, 32, boardloader_hash);

  return boardloader_hash;
}

uint8_t *get_bootloader_hash(void) {
  static uint8_t bootloader_hash[32] = {0};

  sha256_Raw((uint8_t *)BOOTLOADER_START, FIRMWARE_START - BOOTLOADER_START,
             bootloader_hash);
  sha256_Raw(bootloader_hash, 32, bootloader_hash);

  return bootloader_hash;
}

uint8_t *get_firmware_hash(const image_header *hdr) {
  static uint8_t onekey_firmware_hash[32] = {0};
  static bool onekey_firmware_hash_cached = false;
  if (!onekey_firmware_hash_cached) {
    onekey_firmware_hash_cached = true;

    BLAKE2S_CTX ctx;
    blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);
    for (int i = 0; i < FIRMWARE_SECTORS_COUNT; i++) {
      uint8_t sector = FIRMWARE_SECTORS[i];
      uint32_t size = flash_sector_size(sector);
      const void *data = flash_get_address(sector, 0, size);
      if (data == NULL) {
        return NULL;
      }
      blake2s_Update(&ctx, data, size);
    }

    if (blake2s_Final(&ctx, onekey_firmware_hash, BLAKE2S_DIGEST_LENGTH) != 0) {
      return NULL;
    }
  }

  return onekey_firmware_hash;
}
