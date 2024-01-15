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

#include "fingerprint.h"
/// package: trezorio.fingerprint

/// class FpError(OSError):
///     pass
MP_DEFINE_EXCEPTION(FpError, OSError)

/// class EnrollDuplicate(FpError):
///     pass

MP_DEFINE_EXCEPTION(EnrollDuplicate, FpError)

/// class NoFp(FpError):
///     pass
MP_DEFINE_EXCEPTION(NoFp, FpError)

/// class GetImageFail(FpError):
///     pass
MP_DEFINE_EXCEPTION(GetImageFail, FpError)

/// class ExtractFeatureFail(FpError):
///     pass
MP_DEFINE_EXCEPTION(ExtractFeatureFail, FpError)

/// class NotMatch(FpError):
///     pass
MP_DEFINE_EXCEPTION(NotMatch, FpError)

#define FP_RAISE(exc_type, num)                                      \
  {                                                                  \
    nlr_raise(mp_obj_new_exception_arg1(&mp_type_##exc_type,         \
                                        MP_OBJ_NEW_SMALL_INT(num))); \
  }

/// mock:global

/// def detect() -> bool:
///     """
///     Detect fingerprint.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_detect(void) {
  return mp_obj_new_bool(fingerprint_detect());
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_fingerprint_detect_obj,
                                 mod_trezorio_fingerprint_detect);

/// def enroll(seq: int) -> bool:
///     """
///     Enroll fingerprint.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_enroll(mp_obj_t seq) {
  uint8_t seq_ = trezor_obj_get_uint8(seq);
  FP_RESULT res = fingerprint_enroll(seq_);
  switch (res) {
    case FP_OK:
      return mp_const_true;
    case FP_DUPLICATE: {
      FP_RAISE(EnrollDuplicate, res);
      break;
    }
    case FP_NO_FP: {
      FP_RAISE(NoFp, res);
      break;
    }
    case FP_GET_IMAGE_FAIL: {
      FP_RAISE(GetImageFail, res);
      break;
    }
    case FP_EXTRACT_FEATURE_FAIL: {
      FP_RAISE(ExtractFeatureFail, res);
      break;
    }
    default: {
      FP_RAISE(FpError, res);
    }
  }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_fingerprint_enroll_obj,
                                 mod_trezorio_fingerprint_enroll);

/// def save(id: int) -> bool:
///     """
///     Save fingerprint.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_save(mp_obj_t id) {
  uint8_t id_ = trezor_obj_get_uint8(id);
  if (id_ > 2) {
    mp_raise_ValueError("Invalid fingerprint id");
  }
  int8_t res = fingerprint_save(id_);

  if (res == 0) {
    return mp_const_true;
  } else {
    return mp_const_false;
  }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_fingerprint_save_obj,
                                 mod_trezorio_fingerprint_save);
/// def match() -> int:
///     """
///     Verify fingerprint.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_match(void) {
  uint8_t count;
  uint8_t res = fingerprint_get_count(&count);
  if (res != 0) {
    mp_raise_msg(&mp_type_RuntimeError, "Fingerprint sensor error");
  }
  if (count == 0) {
    mp_raise_msg(&mp_type_RuntimeError, "No fingerprints enrolled");
  }
  uint8_t match_id;
  FP_RESULT mres = fingerprint_match(&match_id);
  switch (mres) {
    case FP_OK:
      return mp_obj_new_int_from_uint(match_id);
    case FP_NO_FP: {
      FP_RAISE(NoFp, mres);
      break;
    }
    case FP_NOT_MATCH: {
      FP_RAISE(NotMatch, mres);
      break;
    }
    case FP_GET_IMAGE_FAIL: {
      FP_RAISE(GetImageFail, mres);
      break;
    }
    case FP_EXTRACT_FEATURE_FAIL: {
      FP_RAISE(ExtractFeatureFail, mres);
      break;
    }
    default: {
      FP_RAISE(FpError, mres);
    }
  }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_fingerprint_match_obj,
                                 mod_trezorio_fingerprint_match);

/// def remove(id: int) -> bool:
///     """
///     Remove fingerprint.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_remove(mp_obj_t id) {
  uint8_t id_ = trezor_obj_get_uint8(id);
  uint8_t res = fingerprint_delete(id_);
  if (res == 0) {
    return mp_const_true;
  } else {
    return mp_const_false;
  }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_trezorio_fingerprint_remove_obj,
                                 mod_trezorio_fingerprint_remove);

/// def clear() -> bool:
///     """
///     Remove all fingerprints.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_clear(void) {
  uint8_t res = fingerprint_delete_all();
  if (res == 0) {
    return mp_const_true;
  } else {
    return mp_const_false;
  }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_fingerprint_clear_obj,
                                 mod_trezorio_fingerprint_clear);

/// def get_template_count() -> int:
///     """
///     Get number of stored fingerprints.
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_get_template_count(void) {
  uint8_t count;
  uint8_t res = fingerprint_get_count(&count);
  if (res != 0) {
    mp_raise_msg(&mp_type_RuntimeError, "Fingerprint sensor error");
  }
  return mp_obj_new_int_from_uint(count);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(
    mod_trezorio_fingerprint_get_template_count_obj,
    mod_trezorio_fingerprint_get_template_count);

/// def list_template() -> tuple[int | None] | None:
///     """
///     List fingerprints.
///     returns: tuple of fingerprint ids
///     """

STATIC mp_obj_t mod_trezorio_fingerprint_list_template(void) {
  uint8_t count;
  fingerprint_get_count(&count);
  if (count == 0) {
    return mp_const_none;
  }

  uint8_t bitmap[1] = {0};
  uint8_t res = fingerprint_get_list(bitmap, 1);
  if (res != 0) {
    mp_raise_msg(&mp_type_RuntimeError, "Fingerprint sensor error");
  }
  mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));
  for (int i = 0; i < 3; i++) {
    if (*bitmap & (1 << i)) {
      tuple->items[i] = mp_obj_new_int_from_uint(i);
    } else {
      tuple->items[i] = mp_const_none;
    }
  }

  return MP_OBJ_FROM_PTR(tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_fingerprint_list_template_obj,
                                 mod_trezorio_fingerprint_list_template);

///  def sleep() -> None:
///      """
///      make fingerprint sensor to sleep mode.
///      """

STATIC mp_obj_t mod_trezorio_fingerprint_sleep(void) {
  fingerprint_enter_sleep();
  return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_fingerprint_sleep_obj,
                                 mod_trezorio_fingerprint_sleep);

STATIC const mp_rom_map_elem_t mod_trezorio_fingerprint_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_fingerprint)},

    {MP_ROM_QSTR(MP_QSTR_detect),
     MP_ROM_PTR(&mod_trezorio_fingerprint_detect_obj)},
    {MP_ROM_QSTR(MP_QSTR_enroll),
     MP_ROM_PTR(&mod_trezorio_fingerprint_enroll_obj)},
    {MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&mod_trezorio_fingerprint_save_obj)},
    {MP_ROM_QSTR(MP_QSTR_match),
     MP_ROM_PTR(&mod_trezorio_fingerprint_match_obj)},
    {MP_ROM_QSTR(MP_QSTR_remove),
     MP_ROM_PTR(&mod_trezorio_fingerprint_remove_obj)},
    {MP_ROM_QSTR(MP_QSTR_clear),
     MP_ROM_PTR(&mod_trezorio_fingerprint_clear_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_template_count),
     MP_ROM_PTR(&mod_trezorio_fingerprint_get_template_count_obj)},
    {MP_ROM_QSTR(MP_QSTR_list_template),
     MP_ROM_PTR(&mod_trezorio_fingerprint_list_template_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep),
     MP_ROM_PTR(&mod_trezorio_fingerprint_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_FpError), MP_ROM_PTR(&mp_type_FpError)},
    {MP_ROM_QSTR(MP_QSTR_EnrollDuplicate),
     MP_ROM_PTR(&mp_type_EnrollDuplicate)},
    {MP_ROM_QSTR(MP_QSTR_NoFp), MP_ROM_PTR(&mp_type_NoFp)},
    {MP_ROM_QSTR(MP_QSTR_GetImageFail), MP_ROM_PTR(&mp_type_GetImageFail)},
    {MP_ROM_QSTR(MP_QSTR_ExtractFeatureFail),
     MP_ROM_PTR(&mp_type_ExtractFeatureFail)},
    {MP_ROM_QSTR(MP_QSTR_NotMatch), MP_ROM_PTR(&mp_type_NotMatch)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_fingerprint_globals,
                            mod_trezorio_fingerprint_globals_table);

STATIC const mp_obj_module_t mod_trezorio_fingerprint_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mod_trezorio_fingerprint_globals,
};
