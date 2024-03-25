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

typedef struct {
  char version[16];
  char build_id[16];
} board_info_t;

#if PRODUCTION

static int onekey_known_boardloader(const uint8_t *hash) {
  if (0 ==
      memcmp(hash,
             "\x22\x9a\xcd\xc3\x29\x8b\x61\x05\x0c\x0c\x3b\x0b\x3d\x93\x8d\xbc"
             "\x9b\xa4\xb0\x1e\x44\x8e\x59\xbe\x31\x1b\x83\x92\xb6\x5e\x25\xf5",
             32)) {
    memcpy(boardloader_version, "1.6.1", strlen("1.6.1"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x85\xe5\x28\x2c\xb2\x18\x69\x91\x3e\x46\xa5\xbf\x98\x6f\xdb\x7d"
             "\xc7\x5d\x55\x32\x08\xa1\x4b\x7b\x9e\xdd\xdf\xc5\x33\x15\x9a\x5e",
             32)) {
    memcpy(boardloader_version, "1.6.2", strlen("1.6.2"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\xc0\xbb\x85\xe5\xb2\xfc\xd6\x76\x59\x6d\xb2\x53\xa6\x68\x50\xbe"
             "\x4d\x04\x1d\xa9\x99\x49\xd9\x95\x29\xe2\x0f\xb9\x18\x3a\x17\xb4",
             32)) {
    memcpy(boardloader_version, "1.6.3", strlen("1.6.3"));
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
             "\x96\x86\x3d\x8d\x63\x35\x28\x29\x30\x5e\x95\xc5\x6c\x5f\x0d\x17"
             "\xeb\x8f\x4b\xe7\x2c\x57\xeb\xe4\xe4\x4c\x37\xba\xe7\xb3\xcf\x34",
             32)) {
    memcpy(boardloader_version, "1.6.1", strlen("1.6.1"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\xf3\x80\x4e\x1b\x58\xed\x0d\x39\x66\x76\xe0\xf0\x64\x94\x5d\xfd"
             "\xe0\x26\x79\xc8\x55\x51\x3e\xee\x56\xa0\xcc\x0a\x02\x10\x05\xa1",
             32)) {
    memcpy(boardloader_version, "1.6.2", strlen("1.6.2"));
    return 1;
  }
  if (0 ==
      memcmp(hash,
             "\x19\xb9\x98\xef\xe8\xb2\x3c\x80\x14\x8b\xa0\x2d\xfe\x4a\xb7\x65"
             "\xba\xc2\x1e\x22\xf2\x0f\x23\xcd\x60\xc5\xd2\xbe\x80\xe3\x4c\x5c",
             32)) {
    memcpy(boardloader_version, "1.6.3", strlen("1.6.3"));
    return 1;
  }
  memcpy(boardloader_version, "unknown boardloader",
         strlen("unknown boardloader"));
  return 1;
}

#endif

char *get_boardloader_version(void) {
  uint8_t hash[32] = {0};
  SHA256_CTX context = {0};

  if (strlen(boardloader_version) == 0) {
    sha256_Init(&context);
    sha256_Update(&context, (uint8_t *)BOARDLOADER_START,
                  BOARDLOADER_SIZE - 32);
    sha256_Update(
        &context,
        (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00",
        32);
    sha256_Final(&context, hash);
    sha256_Raw(hash, 32, hash);

    onekey_known_boardloader(hash);
  }

  return boardloader_version;
}

char *get_boardloader_build_id(void) {
  static char boardloader_build_id[16] = {0};
  uint8_t build_id_len = 0;

  board_info_t *board_info =
      (board_info_t *)(BOARDLOADER_START + BOARDLOADER_SIZE -
                       sizeof(board_info_t));
  build_id_len =
      strnlen(board_info->build_id, sizeof(board_info->build_id) - 1);
  if (build_id_len > 0 && board_info->build_id[0] != 0xFF) {
    memcpy(boardloader_build_id, board_info->build_id, build_id_len);
  } else {
    memcpy(boardloader_build_id, "unknown", strlen("unknown"));
  }

  return boardloader_build_id;
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

  uint8_t *p_code_len = (uint8_t *)(BOOTLOADER_START + 12);
  int len = p_code_len[0] + p_code_len[1] * 256 + p_code_len[2] * 256 * 256;
  sha256_Raw((uint8_t *)(BOOTLOADER_START + 1024), len, bootloader_hash);
  sha256_Raw(bootloader_hash, 32, bootloader_hash);

  return bootloader_hash;
}

char *get_bootloader_build_id(void) {
#define BOOTLOADER_BUILD_ID_OFFSET 943
  static char bootloader_build[16] = {0};
  uint8_t build_id_len = 0;

  char *p_build_id = (char *)(BOOTLOADER_START + BOOTLOADER_BUILD_ID_OFFSET);
  build_id_len = strnlen(p_build_id, sizeof(bootloader_build) - 1);
  if (build_id_len > 0) {
    memcpy(bootloader_build, p_build_id, build_id_len);
  } else {
    memcpy(bootloader_build, "unknown", strlen("unknown"));
  }

  return bootloader_build;
}

uint8_t *get_firmware_hash(void) {
  static uint8_t onekey_firmware_hash[32] = {0};
  static bool onekey_firmware_hash_cached = false;
  if (!onekey_firmware_hash_cached) {
    SHA256_CTX context = {0};
    sha256_Init(&context);

    vendor_header *vhdr = (vendor_header *)FIRMWARE_START;
    image_header *hdr = (image_header *)(FIRMWARE_START + vhdr->hdrlen);
    uint32_t innner_firmware_len = 0, outer_firmware_len = 0;

    if (vhdr->magic != 0x56544B4F || hdr->magic != FIRMWARE_IMAGE_MAGIC)
      return onekey_firmware_hash;

    innner_firmware_len =
        hdr->codelen >
                FLASH_FIRMWARE_SECTOR_SIZE * FIRMWARE_INNER_SECTORS_COUNT -
                    vhdr->hdrlen - IMAGE_HEADER_SIZE
            ? FLASH_FIRMWARE_SECTOR_SIZE * FIRMWARE_INNER_SECTORS_COUNT -
                  vhdr->hdrlen - IMAGE_HEADER_SIZE
            : hdr->codelen;
    outer_firmware_len = hdr->codelen - innner_firmware_len;
    sha256_Update(&context,
                  (uint8_t *)FIRMWARE_START + vhdr->hdrlen + IMAGE_HEADER_SIZE,
                  innner_firmware_len);
    sha256_Update(&context,
                  flash_get_address(FLASH_SECTOR_FIRMWARE_EXTRA_START, 0, 0),
                  outer_firmware_len);
    sha256_Final(&context, onekey_firmware_hash);

    onekey_firmware_hash_cached = true;
  }

  return onekey_firmware_hash;
}
