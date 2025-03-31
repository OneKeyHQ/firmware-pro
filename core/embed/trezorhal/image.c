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

#include "blake2s.h"
#include "ed25519-donna/ed25519.h"

#include "common.h"
#include "emmc_fs.h"
#include "flash.h"
#include "fw_keys.h"
#include "hardware_version.h"
#include "image.h"
#include "qspi_flash.h"
#include "sdram.h"
#include "util_macros.h"

static secbool compute_pubkey(uint8_t sig_m, uint8_t sig_n,
                              const uint8_t* const* pub, uint8_t sigmask,
                              ed25519_public_key res) {
  if (0 == sig_m || 0 == sig_n) return secfalse;
  if (sig_m > sig_n) return secfalse;

  // discard bits higher than sig_n
  sigmask &= ((1 << sig_n) - 1);

  // remove if number of set bits in sigmask is not equal to sig_m
  if (__builtin_popcount(sigmask) != sig_m) return secfalse;

  ed25519_public_key keys[sig_m];
  int j = 0;
  for (int i = 0; i < sig_n; i++) {
    if ((1 << i) & sigmask) {
      memcpy(keys[j], pub[i], 32);
      j++;
    }
  }

  return sectrue * (0 == ed25519_cosi_combine_publickeys(res, keys, sig_m));
}

secbool load_image_header(const uint8_t* const data, const uint32_t magic,
                          const uint32_t maxsize, uint8_t key_m, uint8_t key_n,
                          const uint8_t* const* keys, image_header* const hdr) {
  memcpy(&hdr->magic, data, 4);
  if (hdr->magic != magic) return secfalse;

  memcpy(&hdr->hdrlen, data + 4, 4);
  if (hdr->hdrlen != IMAGE_HEADER_SIZE) return secfalse;

  memcpy(&hdr->expiry, data + 8, 4);
  // TODO: expiry mechanism needs to be ironed out before production or those
  // devices won't accept expiring bootloaders (due to boardloader write
  // protection).
  if (hdr->expiry != 0) return secfalse;

  memcpy(&hdr->codelen, data + 12, 4);
  if (hdr->codelen > (maxsize - hdr->hdrlen)) return secfalse;
  if ((hdr->hdrlen + hdr->codelen) < 4 * 1024) return secfalse;
  if ((hdr->hdrlen + hdr->codelen) % 512 != 0) return secfalse;

  memcpy(&hdr->version, data + 16, 4);
  memcpy(&hdr->fix_version, data + 20, 4);
  memcpy(&hdr->onekey_version, data + 24, 4);

  memcpy(hdr->hashes, data + 32, 512);

  memcpy(&hdr->sigmask, data + IMAGE_HEADER_SIZE - IMAGE_SIG_SIZE, 1);

  memcpy(hdr->sig, data + IMAGE_HEADER_SIZE - IMAGE_SIG_SIZE + 1,
         IMAGE_SIG_SIZE - 1);

  // check header signature

  BLAKE2S_CTX ctx;
  blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);
  blake2s_Update(&ctx, data, IMAGE_HEADER_SIZE - IMAGE_SIG_SIZE);
  for (int i = 0; i < IMAGE_SIG_SIZE; i++) {
    blake2s_Update(&ctx, (const uint8_t*)"\x00", 1);
  }
  blake2s_Final(&ctx, hdr->fingerprint, BLAKE2S_DIGEST_LENGTH);

  ed25519_public_key pub;
  if (sectrue != compute_pubkey(key_m, key_n, keys, hdr->sigmask, pub))
    return secfalse;

  return sectrue *
         (0 == ed25519_sign_open(hdr->fingerprint, BLAKE2S_DIGEST_LENGTH, pub,
                                 *(const ed25519_signature*)hdr->sig));
}

secbool load_ble_image_header(const uint8_t* const data, const uint32_t magic,
                              const uint32_t maxsize, image_header* const hdr) {
  memcpy(&hdr->magic, data, 4);
  if (hdr->magic != magic) return secfalse;

  memcpy(&hdr->hdrlen, data + 4, 4);
  // if (hdr->hdrlen != IMAGE_HEADER_SIZE) return secfalse;

  memcpy(&hdr->expiry, data + 8, 4);
  // TODO: expiry mechanism needs to be ironed out before production or those
  // devices won't accept expiring bootloaders (due to boardloader write
  // protection).
  if (hdr->expiry != 0) return secfalse;

  memcpy(&hdr->codelen, data + 12, 4);
  // if (hdr->codelen > (maxsize - hdr->hdrlen)) return secfalse;
  // if ((hdr->hdrlen + hdr->codelen) < 4 * 1024) return secfalse;
  // if ((hdr->hdrlen + hdr->codelen) % 512 != 0) return secfalse;

  memcpy(&hdr->version, data + 16, 4);
  memcpy(&hdr->fix_version, data + 20, 4);
  memcpy(&hdr->onekey_version, data + 24, 4);

  memcpy(hdr->hashes, data + 32, 512);

  memcpy(&hdr->sigmask, data + IMAGE_HEADER_SIZE - IMAGE_SIG_SIZE, 1);

  memcpy(hdr->sig, data + IMAGE_HEADER_SIZE - IMAGE_SIG_SIZE + 1,
         IMAGE_SIG_SIZE - 1);

  return sectrue;
}

secbool load_thd89_image_header(const uint8_t* const data, const uint32_t magic,
                                const uint32_t maxsize,
                                image_header_th89* const hdr) {
  memcpy(&hdr->magic, data, 4);
  if (hdr->magic != magic) return secfalse;

  memcpy(&hdr->hdrlen, data + 4, 4);
  // if (hdr->hdrlen != IMAGE_HEADER_SIZE) return secfalse;

  memcpy(&hdr->expiry, data + 8, 4);
  memcpy(&hdr->version, data + 16, 4);
  // TODO: expiry mechanism needs to be ironed out before production or those
  // devices won't accept expiring bootloaders (due to boardloader write
  // protection).
  // if (hdr->expiry != 0) return secfalse;

  memcpy(&hdr->i2c_address, data + 28, 1);

  memcpy(&hdr->codelen, data + 12, 4);

  memcpy(hdr->hashes, data + 32, 512);

  memcpy(hdr->sig1, data + 32 + 512, 64);

  return sectrue;
}

secbool read_vendor_header(const uint8_t* const data,
                           vendor_header* const vhdr) {
  memcpy(&vhdr->magic, data, 4);
  if (vhdr->magic != 0x56544B4F) return secfalse;  // OKTV

  memcpy(&vhdr->hdrlen, data + 4, 4);
  if (vhdr->hdrlen > 64 * 1024) return secfalse;

  memcpy(&vhdr->expiry, data + 8, 4);
  if (vhdr->expiry != 0) return secfalse;

  memcpy(&vhdr->version, data + 12, 2);

  memcpy(&vhdr->vsig_m, data + 14, 1);
  memcpy(&vhdr->vsig_n, data + 15, 1);
  memcpy(&vhdr->vtrust, data + 16, 2);

  if (vhdr->vsig_n > MAX_VENDOR_PUBLIC_KEYS) {
    return secfalse;
  }

  for (int i = 0; i < vhdr->vsig_n; i++) {
    vhdr->vpub[i] = data + 32 + i * 32;
  }
  for (int i = vhdr->vsig_n; i < MAX_VENDOR_PUBLIC_KEYS; i++) {
    vhdr->vpub[i] = 0;
  }

  memcpy(&vhdr->vstr_len, data + 32 + vhdr->vsig_n * 32, 1);

  vhdr->vstr = (const char*)(data + 32 + vhdr->vsig_n * 32 + 1);

  vhdr->vimg = data + 32 + vhdr->vsig_n * 32 + 1 + vhdr->vstr_len;
  // align to 4 bytes
  vhdr->vimg += (-(uintptr_t)vhdr->vimg) & 3;

  memcpy(&vhdr->sigmask, data + vhdr->hdrlen - IMAGE_SIG_SIZE, 1);

  memcpy(vhdr->sig, data + vhdr->hdrlen - IMAGE_SIG_SIZE + 1,
         IMAGE_SIG_SIZE - 1);

  return sectrue;
}

secbool load_vendor_header(const uint8_t* const data, uint8_t key_m,
                           uint8_t key_n, const uint8_t* const* keys,
                           vendor_header* const vhdr) {
  if (sectrue != read_vendor_header(data, vhdr)) {
    return secfalse;
  }

  // check header signature

  uint8_t hash[BLAKE2S_DIGEST_LENGTH];
  BLAKE2S_CTX ctx;
  blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);
  blake2s_Update(&ctx, data, vhdr->hdrlen - IMAGE_SIG_SIZE);
  for (int i = 0; i < IMAGE_SIG_SIZE; i++) {
    blake2s_Update(&ctx, (const uint8_t*)"\x00", 1);
  }
  blake2s_Final(&ctx, hash, BLAKE2S_DIGEST_LENGTH);

  ed25519_public_key pub;
  if (sectrue != compute_pubkey(key_m, key_n, keys, vhdr->sigmask, pub))
    return secfalse;

  return sectrue *
         (0 == ed25519_sign_open(hash, BLAKE2S_DIGEST_LENGTH, pub,
                                 *(const ed25519_signature*)vhdr->sig));
}

void vendor_header_hash(const vendor_header* const vhdr, uint8_t* hash) {
  BLAKE2S_CTX ctx;
  blake2s_Init(&ctx, BLAKE2S_DIGEST_LENGTH);
  blake2s_Update(&ctx, vhdr->vstr, vhdr->vstr_len);
  blake2s_Update(&ctx, "OneKey Vendor Header", 20);
  blake2s_Final(&ctx, hash, BLAKE2S_DIGEST_LENGTH);
}

secbool check_single_hash(const uint8_t* const hash, const uint8_t* const data,
                          int len) {
  uint8_t h[BLAKE2S_DIGEST_LENGTH];
  blake2s(data, len, h, BLAKE2S_DIGEST_LENGTH);
  return sectrue * (0 == memcmp(h, hash, BLAKE2S_DIGEST_LENGTH));
}

secbool check_image_contents(const image_header* const hdr, uint32_t firstskip,
                             const uint8_t* sectors, int blocks) {
  if (0 == sectors || blocks < 1) {
    return secfalse;
  }

  const void* data =
      flash_get_address(sectors[0], firstskip, IMAGE_CHUNK_SIZE - firstskip);
  if (!data) {
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
    if (block >= blocks) {
      return secfalse;
    }
    data = flash_get_address(sectors[block], 0, IMAGE_CHUNK_SIZE);
    if (!data) {
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
    remaining -= IMAGE_CHUNK_SIZE;
  }
  return sectrue;
}

secbool check_image_contents_ADV(const vendor_header* const vhdr,
                                 const image_header* const hdr,
                                 const uint8_t* const code_buffer,
                                 const size_t code_len_skipped,
                                 const size_t code_len_check) {
  // sanity check
  if (
      // vhdr == NULL || // this is allowed since bootloader image have no vhdr
      hdr == NULL ||       // hdr pointer must valid (loose check)
      code_buffer == NULL  // buffer pointer must valid (loose check)
  ) {
    return secfalse;
  }

  // const vars
  const size_t hash_chunk_size = FLASH_FIRMWARE_SECTOR_SIZE * 2;
  const size_t first_chunk_skip =
      ((vhdr != NULL) ? vhdr->hdrlen : 0) + hdr->hdrlen;

  // vars
  secbool result = secfalse;
  size_t processed_size = 0;
  size_t process_size = 0;
  size_t block = (code_len_skipped + first_chunk_skip) / hash_chunk_size;

  // checking process
  while (true) {
    if (processed_size == 0 && block == 0)
      process_size = MIN((code_len_check - processed_size),
                         (hash_chunk_size - first_chunk_skip));
    else
      process_size = MIN((code_len_check - processed_size), (hash_chunk_size));

    if (sectrue == check_single_hash(hdr->hashes + block * 32,
                                     code_buffer + processed_size,
                                     process_size)) {
      block++;
      processed_size += process_size;
    } else {
      // error exit, hash mismatch
      break;
    }

    if (processed_size >= code_len_check) {
      // make sure we actually checked something
      if (processed_size > 0)
        // flag set to valid
        result = sectrue;

      // normal exit, no error found
      break;
    }
  }

  return result;
}

secbool install_bootloader(const uint8_t* const buffer, const size_t size,
                           char* error_msg, size_t error_msg_len,
                           size_t* const processed,
                           void (*const progress_callback)(int)) {
  // sanity check
  if (buffer == NULL ||          // pointer invalid
      size <= 0 ||               // cannot be zero size
      size % 4 != 0 ||           // not 32 bit aligned
      error_msg == NULL ||       // must provide error reporting msg buffer
      progress_callback == NULL  // must provide progress reporting function
  )
    return secfalse;

  // vars
  size_t processed_size = 0;

  // if error reporting needed, must provide a large enough buffer
  if (error_msg != NULL && error_msg_len < IMAGE_UTIL_ERROR_MSG_BUFFER_SIZE_MIN)
    return secfalse;

  // prepare
  progress_callback(0);

  // install
  while (processed_size < size) {
    // erase
    EXEC_RETRY(
        10, sectrue, {},
        {
          return flash_erase(BOOTLOADER_SECTORS[processed_size /
                                                FLASH_BOOTLOADER_SECTOR_SIZE]);
        },
        {},
        {
          strncpy(error_msg, "Flash bootloader area erase failed!",
                  error_msg_len);
          return secfalse;
        });

    // unlock
    EXEC_RETRY(
        10, sectrue, {}, { return flash_unlock_write(); }, {},
        {
          strncpy(error_msg, "Flash unlock failed!", error_msg_len);
          return secfalse;
        });

    // write
    for (size_t sector_offset = 0; sector_offset < FLASH_BOOTLOADER_SECTOR_SIZE;
         sector_offset += 32) {
      // write with retry, max 10 retry allowed
      EXEC_RETRY(
          10, sectrue, {},
          {
            return flash_write_words(
                BOOTLOADER_SECTORS[processed_size /
                                   FLASH_BOOTLOADER_SECTOR_SIZE],
                sector_offset, (uint32_t*)(buffer + processed_size));
          },
          {},
          {
            strncpy(error_msg, "Flash write failed!", error_msg_len);
            return secfalse;
          });

      processed_size += ((size - processed_size) > 32)
                            ? 32  // since we could only write 32 byte a time
                            : (size - processed_size);

      // update progress
      progress_callback((100 * processed_size / size));
      if (processed != NULL) *processed = processed_size;
    }

    // lock
    EXEC_RETRY(
        10, sectrue, {}, { return flash_lock_write(); }, {},
        {
          strncpy(error_msg, "Flash unlock failed!", error_msg_len);
          return secfalse;
        });
  }

  return sectrue;
}

/*
  Note:
  why load and output headers and code from here instead of individual
  functions, is to ensure what we output is what we checked

  All arguments are OPTIONAL, put NULL if not used

  vhdr -> pointer to vendor header buffer, output vhdr data
  hdr -> pointer to image header buffer, output hdr data
  xxx_valid -> output validate results
  error_msg, error_msg_len -> output error message, error_msg_len should be at
  least 64 bytes

  return value -> total check result
 */
secbool verify_bootloader(image_header* const hdr, secbool* const hdr_valid,
                          secbool* const code_valid, char* error_msg,
                          size_t error_msg_len) {
  // internal vars
  image_header _hdr = {0};
  secbool _hdr_valid = secfalse;
  secbool _code_valid = secfalse;

  // optional arguments wipe default
  if (hdr != NULL) memset(hdr, 0xff, sizeof(image_header));
  if (hdr_valid != NULL) *hdr_valid = secfalse;
  if (code_valid != NULL) *code_valid = secfalse;
  if (error_msg != NULL) memset(error_msg, '\0', error_msg_len);

  // if error reporting needed, must provide a large enough buffer
  if (error_msg != NULL && error_msg_len < IMAGE_UTIL_ERROR_MSG_BUFFER_SIZE_MIN)
    return secfalse;

  // verify hdr
  ExecuteCheck_ADV(
      load_image_header((const uint8_t*)BOOTLOADER_START,
                        BOOTLOADER_IMAGE_MAGIC, BOOTLOADER_IMAGE_MAXSIZE,
                        FW_KEY_M, FW_KEY_N, FW_KEYS, &_hdr),
      sectrue, {
        if (error_msg != NULL)
          strncpy(error_msg, "Bootloader image header invalid!", error_msg_len);
        return secfalse;
      });
  _hdr_valid = sectrue;
  if (hdr != NULL) memcpy(hdr, &_hdr, sizeof(image_header));
  if (hdr_valid != NULL) *hdr_valid = _hdr_valid;

  // verify code
  ExecuteCheck_ADV(
      check_image_contents_ADV(NULL, &_hdr,
                               (const uint8_t*)BOOTLOADER_START + _hdr.hdrlen,
                               0, _hdr.codelen),
      sectrue, {
        if (error_msg != NULL)
          strncpy(error_msg, "Bootloader code invalid!", error_msg_len);
        return secfalse;
      });
  _code_valid = sectrue;
  if (code_valid != NULL) *code_valid = _code_valid;

  return (((_hdr_valid == sectrue) && (_code_valid == sectrue))) ? sectrue
                                                                 : secfalse;
}

secbool install_firmware(const uint8_t* const buffer, const size_t size,
                         char* error_msg, size_t error_msg_len,
                         size_t* const processed, uint8_t percent_start,
                         uint8_t weights,
                         void (*const progress_callback)(int)) {
  // sanity check
  if (buffer == NULL ||          // pointer invalid
      size <= 0 ||               // cannot be zero size
      size % 4 != 0 ||           // not 32 bit aligned
      error_msg == NULL ||       // must provide error reporting msg buffer
      progress_callback == NULL  // must provide progress reporting function
  )
    return secfalse;

  // const vars
  const size_t fw_internal_size =
      FLASH_FIRMWARE_SECTOR_SIZE * FIRMWARE_INNER_SECTORS_COUNT;

  // vars
  size_t processed_size = 0;

  // if error reporting needed, must provide a large enough buffer
  if (error_msg != NULL && error_msg_len < IMAGE_UTIL_ERROR_MSG_BUFFER_SIZE_MIN)
    return secfalse;

  // prepare
  if (get_hw_ver() >= HW_VER_3P0A) {
    // enusure dir exists and remove p2 file
    if (!emmc_fs_dir_make("0:/data") ||
        !emmc_fs_file_delete("0:/data/fw_p2.bin")) {
      strncpy(error_msg, "Flash firmware prepare failed! (EMMC)",
              error_msg_len);
      return secfalse;
    }
  } else {
    // not worth it, take too long (8M for 25s)
    // if ( HAL_OK != qspi_flash_erase_chip() )
    // {
    //     strncpy(error_msg, "Flash firmware prepare failed! (QSPI)",
    //     error_msg_len); return secfalse;
    // }
  }

  // install
  while (processed_size < size) {
    if (processed_size < fw_internal_size) {
      // install p1

      // erase
      EXEC_RETRY(
          10, sectrue, {},
          {
            return flash_erase(
                FIRMWARE_SECTORS[processed_size / FLASH_FIRMWARE_SECTOR_SIZE]);
          },
          {},
          {
            strncpy(error_msg, "Flash firmware area erase failed!",
                    error_msg_len);
            return secfalse;
          });

      // unlock
      EXEC_RETRY(
          10, sectrue, {}, { return flash_unlock_write(); }, {},
          {
            strncpy(error_msg, "Flash unlock failed!", error_msg_len);
            return secfalse;
          });

      // write
      for (size_t sector_offset = 0; sector_offset < FLASH_FIRMWARE_SECTOR_SIZE;
           sector_offset += 32) {
        // write with retry, max 10 retry allowed
        EXEC_RETRY(
            10, sectrue, {},
            {
              return flash_write_words(
                  FIRMWARE_SECTORS[processed_size / FLASH_FIRMWARE_SECTOR_SIZE],
                  sector_offset, (uint32_t*)(buffer + processed_size));
            },
            {},
            {
              strncpy(error_msg, "Flash write failed!", error_msg_len);
              return secfalse;
            });

        processed_size += ((size - processed_size) > 32)
                              ? 32  // since we could only write 32 byte a time
                              : (size - processed_size);
      }

      // lock
      EXEC_RETRY(
          10, sectrue, {}, { return flash_lock_write(); }, {},
          {
            strncpy(error_msg, "Flash unlock failed!", error_msg_len);
            return secfalse;
          });
    } else {
      // install p2

      if (get_hw_ver() >= HW_VER_3P0A) {
        // EMMC
        uint32_t emmc_fs_processed = 0;
        EXEC_RETRY(
            10, true, {},
            {
              return emmc_fs_file_write(
                  "0:/data/fw_p2.bin", (processed_size - fw_internal_size),
                  (uint8_t*)(buffer + processed_size),
                  MIN((size - processed_size), FLASH_FIRMWARE_SECTOR_SIZE),
                  &emmc_fs_processed, false, true);
            },
            {
              processed_size += emmc_fs_processed;
              hal_delay(100);  // delay for visual
            },
            {
              strncpy(error_msg, "Flash write failed! (EMMC)", error_msg_len);
              return secfalse;
            });
      } else {
        // QSPI

        // erase
        EXEC_RETRY(
            10, sectrue, {},
            {
              return flash_erase(FIRMWARE_SECTORS[processed_size /
                                                  FLASH_FIRMWARE_SECTOR_SIZE]);
            },
            {},
            {
              strncpy(error_msg, "Flash firmware area erase failed! (QSPI)",
                      error_msg_len);
              return secfalse;
            });

        // unlock
        EXEC_RETRY(
            10, sectrue, {}, { return flash_unlock_write(); }, {},
            {
              strncpy(error_msg, "Flash unlock failed! (QSPI)", error_msg_len);
              return secfalse;
            });

        // write
        for (size_t sector_offset = 0;
             sector_offset < FLASH_FIRMWARE_SECTOR_SIZE; sector_offset += 32) {
          // write with retry, max 10 retry allowed
          EXEC_RETRY(
              10, sectrue, {},
              {
                return flash_write_words(
                    FIRMWARE_SECTORS[processed_size /
                                     FLASH_FIRMWARE_SECTOR_SIZE],
                    sector_offset, (uint32_t*)(buffer + processed_size));
              },
              {},
              {
                strncpy(error_msg, "Flash write failed! (QSPI)", error_msg_len);
                return secfalse;
              });

          processed_size +=
              ((size - processed_size) > 32)
                  ? 32  // since we could only write 32 byte a time
                  : (size - processed_size);
        }

        // lock
        EXEC_RETRY(
            10, sectrue, {}, { return flash_lock_write(); }, {},
            {
              strncpy(error_msg, "Flash unlock failed! (QSPI)", error_msg_len);
              return secfalse;
            });
      }
    }

    // update progress
    progress_callback(percent_start + weights * processed_size / size);
    if (processed != NULL) *processed = processed_size;
  }

  if (get_hw_ver() < HW_VER_3P0A) {
    // wipe unused sectors (P1 and P2 QSPI only)
    size_t used_sector_count =
        (processed_size / FLASH_FIRMWARE_SECTOR_SIZE) +
        ((processed_size % FLASH_FIRMWARE_SECTOR_SIZE) != 0 ? 1 : 0);
    while (used_sector_count < FIRMWARE_SECTORS_COUNT) {
      flash_erase(FIRMWARE_SECTORS[used_sector_count]);
      used_sector_count++;
    }
  }

  return sectrue;
}

/*
  Note:
  why load and output headers and code from here instead of individual
  functions, is to ensure what we output is what we checked

  All arguments are OPTIONAL, put NULL if not used

  vhdr -> pointer to vendor header buffer, output vhdr data
  hdr -> pointer to image header buffer, output hdr data
  xxx_valid -> output validate results
  error_msg, error_msg_len -> output error message, error_msg_len should be at
  least 64 bytes

  return value -> total check result
 */
secbool verify_firmware(vendor_header* const vhdr, image_header* const hdr,
                        secbool* const vhdr_valid, secbool* const hdr_valid,
                        secbool* const code_valid, char* error_msg,
                        size_t error_msg_len) {
  // internal vars
  vendor_header _vhdr = {0};
  image_header _hdr = {0};
  secbool _vhdr_valid = secfalse;
  secbool _hdr_valid = secfalse;
  secbool _code_valid = secfalse;

  // optional arguments wipe default
  if (vhdr != NULL) memset(vhdr, 0xff, sizeof(vendor_header));
  if (hdr != NULL) memset(hdr, 0xff, sizeof(image_header));
  if (vhdr_valid != NULL) *vhdr_valid = secfalse;
  if (hdr_valid != NULL) *hdr_valid = secfalse;
  if (code_valid != NULL) *code_valid = secfalse;
  if (error_msg != NULL) memset(error_msg, '\0', error_msg_len);

  // if error reporting needed, must provide a large enough buffer
  if (error_msg != NULL && error_msg_len < IMAGE_UTIL_ERROR_MSG_BUFFER_SIZE_MIN)
    return secfalse;

  // const vars
  const size_t fw_internal_size =
      FLASH_FIRMWARE_SECTOR_SIZE * FIRMWARE_INNER_SECTORS_COUNT;
  const size_t fw_external_size =
      FLASH_FIRMWARE_SECTOR_SIZE *
      (FIRMWARE_SECTORS_COUNT - FIRMWARE_INNER_SECTORS_COUNT);

  // verify vhdr
  ExecuteCheck_ADV(load_vendor_header((const uint8_t*)FIRMWARE_START, FW_KEY_M,
                                      FW_KEY_N, FW_KEYS, &_vhdr),
                   sectrue, {
                     if (error_msg != NULL)
                       strncpy(error_msg, "Firmware vendor header invalid!",
                               error_msg_len);
                     return secfalse;
                   });
  _vhdr_valid = sectrue;
  if (vhdr != NULL) memcpy(vhdr, &_vhdr, sizeof(vendor_header));
  if (vhdr_valid != NULL) *vhdr_valid = _vhdr_valid;

  // verify hdr
  ExecuteCheck_ADV(
      load_image_header((const uint8_t*)(FIRMWARE_START + _vhdr.hdrlen),
                        FIRMWARE_IMAGE_MAGIC, FIRMWARE_IMAGE_MAXSIZE,
                        _vhdr.vsig_m, _vhdr.vsig_n, _vhdr.vpub, &_hdr),
      sectrue, {
        if (error_msg != NULL)
          strncpy(error_msg, "Firmware image header invalid!", error_msg_len);
        return secfalse;
      });
  _hdr_valid = sectrue;
  if (hdr != NULL) memcpy(hdr, &_hdr, sizeof(image_header));
  if (hdr_valid != NULL) *hdr_valid = _hdr_valid;

  // verify p1
  ExecuteCheck_ADV(
      check_image_contents_ADV(
          &_vhdr, &_hdr,
          (const uint8_t*)FIRMWARE_START + _vhdr.hdrlen + _hdr.hdrlen, 0,
          fw_internal_size - (_vhdr.hdrlen + _hdr.hdrlen)),
      sectrue, {
        if (error_msg != NULL)
          strncpy(error_msg, "Firmware code invalid! (P1)", error_msg_len);
        return secfalse;
      });

  // verify p2
  if (get_hw_ver() >= HW_VER_3P0A) {
    EMMC_PATH_INFO path_info = {0};
    uint32_t processed_len = 0;

    ExecuteCheck_ADV(emmc_fs_path_info("0:data/fw_p2.bin", &path_info), true, {
      if (error_msg != NULL)
        strncpy(error_msg, "Firmware code invalid! (P2_EMMC_1)", error_msg_len);
      return secfalse;
    });

    ExecuteCheck_ADV(
        emmc_fs_file_read(
            "0:data/fw_p2.bin", 0, (uint32_t*)FMC_SDRAM_FIRMWARE_P2_ADDRESS,
            MAX(path_info.size, fw_external_size), &processed_len),
        true, {
          if (error_msg != NULL)
            strncpy(error_msg, "Firmware code invalid! (P2_EMMC_2)",
                    error_msg_len);
          return secfalse;
        });
  } else {
    memcpy((uint8_t*)FMC_SDRAM_FIRMWARE_P2_ADDRESS, (const uint8_t*)0x90000000,
           _hdr.codelen - (fw_internal_size - (_vhdr.hdrlen + _hdr.hdrlen)));
  }

  ExecuteCheck_ADV(
      check_image_contents_ADV(
          &_vhdr, &_hdr, (const uint8_t*)FMC_SDRAM_FIRMWARE_P2_ADDRESS,
          (fw_internal_size - (_vhdr.hdrlen + _hdr.hdrlen)),
          _hdr.codelen - (fw_internal_size - (_vhdr.hdrlen + _hdr.hdrlen))),
      sectrue, {
        memset((uint8_t*)FMC_SDRAM_FIRMWARE_P2_ADDRESS, 0x00,
               (2 * 1024 * 1024));  // wipe the buffer if fail
        if (error_msg != NULL)
          strncpy(error_msg, "Firmware code invalid! (P2)", error_msg_len);
        return secfalse;
      });
  _code_valid = sectrue;
  if (code_valid != NULL) *code_valid = _code_valid;

  return (((_vhdr_valid == sectrue) && (_hdr_valid == sectrue) &&
           (_code_valid == sectrue)))
             ? sectrue
             : secfalse;
}
