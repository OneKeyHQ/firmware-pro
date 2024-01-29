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
             "\x08\xe5\xd4\x4e\xd5\xa3\x9b\x15\x07\x7d\xfa\x96\xbe\x53\xca\x64"
             "\x36\xf4\x8a\xfb\xac\xe6\xcf\x9d\x8c\xd3\x30\xb8\x43\x5c\x59\xd4",
             32)) {
    memcpy(boardloader_version, "1.6.1", strlen("1.6.1"));
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
             "\x6d\xa9\x60\x54\xdb\xc3\x27\x32\x29\x50\x27\x37\x80\xc6\x78\xec"
             "\xc3\xe6\x64\x55\x37\xe9\x68\x67\x9f\x6d\xcf\x0c\x43\x01\x84\x38",
             32)) {
    memcpy(boardloader_version, "1.6.1", strlen("1.6.1"));
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
