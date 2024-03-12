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

#ifndef __BR_CHECK_H__
#define __BR_CHECK_H__

#include <stdbool.h>
#include <stdio.h>
#include "image.h"

#define FLASH_PTR(x) (const uint8_t *)(x)

char *get_boardloader_version(void);
uint8_t *get_boardloader_hash(void);
char *get_boardloader_build_id(void);
char *get_bootloader_build_id(void);
uint8_t *get_bootloader_hash(void);
uint8_t *get_firmware_hash(void);

#endif
