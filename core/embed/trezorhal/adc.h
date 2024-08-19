#ifndef TREZORHAL_ADC_H
#define TREZORHAL_ADC_H

#include <stdbool.h>
#include <stdint.h>

bool adc_init();
bool adc_deinit();
bool adc_read_device_hw_ver(uint16_t *value);

// helper functions

// defines

#endif