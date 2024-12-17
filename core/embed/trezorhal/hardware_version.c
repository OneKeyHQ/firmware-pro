#include "hardware_version.h"

#include <stdio.h>

#include "adc.h"
#include "util_macros.h"

// since the HW_VER_t enum was designed to represent ADC values directly for
// easy usage we cannot use regular "array of strings" method to offer name
// reflection because the array will have gaps between items, which wasting
// memory

char const* hw_ver_to_str(HW_VER_t hw_ver) {
  static char hw_ver_str[32];  // self contained static buffer

  switch (hw_ver) {
    case HW_VER_UNKNOWN:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_UNKNOWN");
      break;
    case HW_VER_LEGACY:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_LEGACY");
      break;
    case HW_VER_1P3A:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_1P3A");
      break;
    case HW_VER_3P0:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_3P0");
      break;
    case HW_VER_3P0A:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_3P0A");
      break;
    case HW_VER_INVALID:
    default:
      snprintf(hw_ver_str, sizeof(hw_ver_str) - 1, "HW_VER_INVALID");
      break;
  }

  hw_ver_str[sizeof(hw_ver_str) - 1] = '\0';

  return hw_ver_str;
}

static bool check_mv_in_range(uint16_t value, uint16_t target,
                              uint16_t accuracy_percent) {
  uint16_t mv_min = target * (100 - accuracy_percent) / 100;
  uint16_t mv_max = target * (100 + accuracy_percent) / 100;

  if ((mv_min < value) && (value < mv_max))
    return true;
  else
    return false;
}

uint16_t get_hw_ver_adc_raw() {
  uint16_t adc_val;
  if (!adc_read_device_hw_ver(&adc_val)) return 0;
  return adc_val;
}

uint16_t get_hw_ver_adc_mv() {
  return (get_hw_ver_adc_raw() & HW_VER_ADC_RANGE_MAX) *
         HW_VER_ADC_VOLTAGE_MAX / HW_VER_ADC_RANGE_MAX;
}

HW_VER_t get_hw_ver() {
  uint16_t adc_mv = get_hw_ver_adc_mv();

  if (check_mv_in_range(adc_mv, HW_VER_LEGACY, HW_VER_PRECISION_PERCENT))
    return HW_VER_LEGACY;
  if (check_mv_in_range(adc_mv, HW_VER_1P3A, HW_VER_PRECISION_PERCENT))
    return HW_VER_1P3A;
  if (check_mv_in_range(adc_mv, HW_VER_3P0, HW_VER_PRECISION_PERCENT))
    return HW_VER_3P0;
  if (check_mv_in_range(adc_mv, HW_VER_3P0A, HW_VER_PRECISION_PERCENT))
    return HW_VER_3P0A;

  return HW_VER_UNKNOWN;
}