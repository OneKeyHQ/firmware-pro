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
#include "motor.h"

/// package: trezorio.__init__

/// class MOTOR:
///     """
///     """
typedef struct _mp_obj_MOTOR_t {
  mp_obj_base_t base;
} mp_obj_MOTOR_t;

/// def __init__(
///     self,
/// ) -> None:
///     """
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_make_new(const mp_obj_type_t* type,
                                            size_t n_args, size_t n_kw,
                                            const mp_obj_t* args) {
  mp_arg_check_num(n_args, n_kw, 0, 0, false);

  mp_obj_MOTOR_t* o = m_new_obj(mp_obj_MOTOR_t);
  o->base.type = type;

  return MP_OBJ_FROM_PTR(o);
}

/// def reset(self) -> None:
///     """
///     Reset motor and stop any on going vibrate
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_reset(mp_obj_t self) {
  motor_reset();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_reset_obj,
                                 mod_trezorio_MOTOR_reset);

/// def play_whisper(self) -> None:
///     """
///     Play builtin whisper pattern
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_whisper(mp_obj_t self) {
  motor_play_whisper();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_whisper_obj,
                                 mod_trezorio_MOTOR_play_whisper);

/// def play_light(self) -> None:
///     """
///     Play builtin light pattern
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_light(mp_obj_t self) {
  motor_play_light();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_light_obj,
                                 mod_trezorio_MOTOR_play_light);

/// def play_medium(self) -> None:
///     """
///     Play builtin medium pattern
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_medium(mp_obj_t self) {
  motor_play_medium();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_medium_obj,
                                 mod_trezorio_MOTOR_play_medium);

/// def play_heavy(self) -> None:
///     """
///     Play builtin heavy pattern
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_heavy(mp_obj_t self) {
  motor_play_heavy();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_heavy_obj,
                                 mod_trezorio_MOTOR_play_heavy);

/// def play_success(self) -> None:
///     """
///     Play builtin success sequence
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_success(mp_obj_t self) {
  motor_play_success();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_success_obj,
                                 mod_trezorio_MOTOR_play_success);

/// def play_warning(self) -> None:
///     """
///     Play builtin warning sequence
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_warning(mp_obj_t self) {
  motor_play_warning();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_warning_obj,
                                 mod_trezorio_MOTOR_play_warning);

/// def play_error(self) -> None:
///     """
///     Play builtin error sequence
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_play_error(mp_obj_t self) {
  motor_play_error();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_error_obj,
                                 mod_trezorio_MOTOR_play_error);

// /// def play_slide(self) -> None:
// ///     """
// ///     Play builtin slide sequence
// ///     """
// STATIC mp_obj_t mod_trezorio_MOTOR_play_slide(mp_obj_t self) {
// motor_play_slide();
//     return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_play_slide_obj,
//                                  mod_trezorio_MOTOR_play_slide);

STATIC const mp_rom_map_elem_t mod_trezorio_MOTOR_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_MOTOR)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&mod_trezorio_MOTOR_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_whisper),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_whisper_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_light),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_light_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_medium),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_medium_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_heavy),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_heavy_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_success),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_success_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_warning),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_warning_obj)},
    {MP_ROM_QSTR(MP_QSTR_play_error),
     MP_ROM_PTR(&mod_trezorio_MOTOR_play_error_obj)},
    // {MP_ROM_QSTR(MP_QSTR_play_slide),
    // MP_ROM_PTR(&mod_trezorio_MOTOR_play_slide_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_MOTOR_locals_dict,
                            mod_trezorio_MOTOR_locals_dict_table);

STATIC const mp_obj_type_t mod_trezorio_MOTOR_module = {
    {&mp_type_type},
    .name = MP_QSTR_MOTOR,
    .make_new = mod_trezorio_MOTOR_make_new,
    .locals_dict = (void*)&mod_trezorio_MOTOR_locals_dict,
};
