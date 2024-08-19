#include "hardware_version.h"

#include "adc.h"
#include "util_macros.h"

const char* const HW_VER_str[] = {
    ENUM_NAME_ARRAY_ITEM(HW_VER_INVALID), ENUM_NAME_ARRAY_ITEM(HW_VER_UNKNOWN),
    ENUM_NAME_ARRAY_ITEM(HW_VER_LEGACY),  ENUM_NAME_ARRAY_ITEM(HW_VER_1P3A),
    ENUM_NAME_ARRAY_ITEM(HW_VER_3P0),     ENUM_NAME_ARRAY_ITEM(HW_VER_3P0A),
};

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

  return HW_VER_UNKNOWN;
}