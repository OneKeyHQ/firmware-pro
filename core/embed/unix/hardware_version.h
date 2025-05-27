#ifndef _HARDWARE_VERSION_H_
#define _HARDWARE_VERSION_H_

typedef enum {
  // force compiler use at least 16bit number type
  HW_VER_INVALID = 65535,
  HW_VER_UNKNOWN = 0,
  HW_VER_LEGACY = 300,
  HW_VER_1P3A = 1055,
  HW_VER_3P0 = 1487,
  HW_VER_3P0A = 1964,
} HW_VER_t;

#define get_hw_ver() 0
#define hw_ver_to_str(...) "emu_v0.0.1"
#define get_hw_ver_adc_raw() 0

#endif  //_HARDWARE_VERSION_H_
