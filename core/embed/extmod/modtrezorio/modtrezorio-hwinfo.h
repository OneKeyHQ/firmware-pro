
#include "embed/extmod/trezorobj.h"

#include "hardware_version.h"

/// package: trezorio.hwinfo

/// def ver() -> str:
///     """
///     Get hardware version string.
///     """
STATIC mp_obj_t mod_trezorio_hwinfo_ver(void) {
  HW_VER_t hwver = get_hw_ver();  // cache this to make sure not allocate buffer
                                  // for ver x but copy string from ver y
  return mp_obj_new_str(hw_ver_to_str(hwver),
                        MIN(strlen(hw_ver_to_str(hwver)), 32));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_hwinfo_ver_obj,
                                 mod_trezorio_hwinfo_ver);

/// def ver_adc() -> int:
///     """
///     Get hardware version adc (debug purpose only).
///     """
STATIC mp_obj_t mod_trezorio_hwinfo_ver_adc(void) {
  return mp_obj_new_int_from_uint(get_hw_ver_adc_raw());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_trezorio_hwinfo_ver_adc_obj,
                                 mod_trezorio_hwinfo_ver_adc);

STATIC const mp_rom_map_elem_t mod_trezorio_hwinfo_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_hwinfo)},
    {MP_ROM_QSTR(MP_QSTR_ver), MP_ROM_PTR(&mod_trezorio_hwinfo_ver_obj)},
    {MP_ROM_QSTR(MP_QSTR_ver_adc),
     MP_ROM_PTR(&mod_trezorio_hwinfo_ver_adc_obj)},
};

STATIC MP_DEFINE_CONST_DICT(mod_trezorio_hwinfo_globals,
                            mod_trezorio_hwinfo_globals_table);

STATIC const mp_obj_module_t mod_trezorio_hwinfo_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mod_trezorio_hwinfo_globals,
};
