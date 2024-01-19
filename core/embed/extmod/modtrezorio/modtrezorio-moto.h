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

/// def tick(self) -> None:
///     """
///     Strong vibrate
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_tick(mp_obj_t self) {
  motor_tick();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_tick_obj,
                                 mod_trezorio_MOTOR_tick);

/// def tock(self) -> None:
///     """
///     Weak vibrate
///     """
STATIC mp_obj_t mod_trezorio_MOTOR_tock(mp_obj_t self) {
  motor_tick();
  return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_MOTOR_tock_obj,
                                 mod_trezorio_MOTOR_tock);

// /// def play(self, pattern) -> None:
// ///     """
// ///     Play custom pattern
// ///
// ///     Pattern is expacted to be a List that contains multiple Pair of
// ///     MOTOR_STATE and durnation
// ///     """
// STATIC mp_obj_t mod_trezorio_MOTOR_play(mp_obj_t self, mp_obj_t pattern)
// {
//     // WIP, DO NOT USE!

//     // get pattern
//     mp_obj_list_t* pattern_list_p = MP_OBJ_TO_PTR(pattern);

//     // translate pattern
//     MOTOR_ACTION MAL_pattern[pattern_list_p->len];
//     for ( size_t index = 0; index < pattern_list_p->len; index++ )
//     {
//         mp_obj_t* item = pattern_list_p->items[index];
//     }

//     motor_timer_play(MAL_pattern, len);

//     return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_2(mod_trezorio_MOTOR_play_obj,
// mod_trezorio_MOTOR_play);

STATIC const mp_rom_map_elem_t mod_trezorio_MOTOR_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_tick), MP_ROM_PTR(&mod_trezorio_MOTOR_tick_obj)},
    {MP_ROM_QSTR(MP_QSTR_tock), MP_ROM_PTR(&mod_trezorio_MOTOR_tock_obj)},
    // {MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&mod_trezorio_MOTOR_play_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_MOTOR_locals_dict,
                            mod_trezorio_MOTOR_locals_dict_table);

STATIC const mp_obj_type_t mod_trezorio_MOTOR_type = {
    {&mp_type_type},
    .name = MP_QSTR_MOTOR,
    .make_new = mod_trezorio_MOTOR_make_new,
    .locals_dict = (void*)&mod_trezorio_MOTOR_locals_dict,
};
