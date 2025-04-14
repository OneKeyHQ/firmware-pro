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

#include "embed/extmod/trezorobj.h"

#include "camera.h"
#include "camera_qrcode.h"

/// package: trezorio.camera

/// def scan_qrcode(x: int, y: int) -> bytes:
///     """
///     Returns qr data.
///     """
STATIC mp_obj_t mod_trezorio_camera_scan_qrcode(mp_obj_t x, mp_obj_t y) {
  uint32_t pos_x = trezor_obj_get_uint(x);
  uint32_t pos_y = trezor_obj_get_uint(y);

  uint8_t qr_code[1024] = {0};

  uint32_t qr_len;

  qr_len = camera_qr_decode(pos_x, pos_y, (uint8_t *)qr_code, sizeof(qr_code));

  if (qr_len == 0) {
    return mp_const_none;
  }
  if (qr_len > sizeof(qr_code)) {
    mp_raise_ValueError("QR code buffer too small");
  }

  return mp_obj_new_bytes(qr_code, qr_len);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_trezorio_camera_scan_qrcode_obj,
                                 mod_trezorio_camera_scan_qrcode);

/// def stop() -> None:
///     """
///     Stop camera.
///     """
STATIC mp_obj_t mod_trezorio_camera_stop(void) {
  camera_stop();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_camera_stop_obj,
                                 mod_trezorio_camera_stop);

STATIC const mp_rom_map_elem_t mod_trezorio_camera_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_camera)},

    {MP_ROM_QSTR(MP_QSTR_scan_qrcode),
     MP_ROM_PTR(&mod_trezorio_camera_scan_qrcode_obj)},
    {MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&mod_trezorio_camera_stop_obj)},
};
STATIC MP_DEFINE_CONST_DICT(mod_trezorio_camera_globals,
                            mod_trezorio_camera_globals_table);

STATIC const mp_obj_module_t mod_trezorio_camera_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mod_trezorio_camera_globals,
};
