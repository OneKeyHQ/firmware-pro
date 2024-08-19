#ifndef _HARDWARE_VERSION_H_
#define _HARDWARE_VERSION_H_

#include <stdint.h>

// ADC PA7 for hardware version
// V1.3 and lower -> R82=10K,R83=100K ADCVAL=372;
// V1.3a -> R82=47K,R83=100K ADCVAL=1309;
// V2.0 -> R82=68K,R83=100K ADCVAL=1657; (NOT RELEASED)
// V3.0 -> R82=82K,R83=100K ADCVAL=1845;
// V3.0a -> R82=100K,R83=68K

typedef enum {
  // force compiler use at least 16bit number type
  HW_VER_INVALID = 65535,
  // values
  HW_VER_UNKNOWN = 0,
  HW_VER_LEGACY = 300,
  HW_VER_1P3A = 1055,
  HW_VER_3P0 = 1487,
  HW_VER_3P0A = 1964,
} HW_VER_t;

extern const char* const HW_VER_str[];

// max adc voltage 3.3v = 3300mv
#define HW_VER_ADC_VOLTAGE_MAX 3300
#define HW_VER_ADC_RANGE_MAX 0xffff
// +/- 5% of mv value
#define HW_VER_PRECISION_PERCENT 15

uint16_t get_hw_ver_adc_raw();
uint16_t get_hw_ver_adc_mv();
HW_VER_t get_hw_ver();

#endif  //_HARDWARE_VERSION_H_
