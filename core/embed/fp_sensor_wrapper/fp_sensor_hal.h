#ifndef _FP_SENSOR_HAL_H_
#define _FP_SENSOR_HAL_H_

#include STM32_HAL_H

#include <stdint.h>
#include "sdram.h"
#include "emmc_debug_utils.h"

// #define SPI_FP_USE_HW_CS

// FP_FPC      B2B SOCKET  MCU  AF
// SPI_MISO    SPI3_MISO   PB4  AF6
// SPI_MOSI    SPI3_MOSI   PD6  AF5
// SPI_CLK     SPI3_SCK    PB3  AF6
// SPI_CSn     SPI3_NSS    PA15 AF6
// SPI_INT     FP_IRQ      PB15
// RSTn        FP_RST      PB14

#define ExecuteCheck_ADV_FP(func_call, expected_result, on_false) \
    {                                                             \
        uint8_t FP_ret = (func_call);                             \
        if ( FP_ret != (expected_result) )                        \
        {                                                         \
            on_false                                              \
        }                                                         \
    }

#define ExecuteCheck_FPSENSOR_OK(func_call) \
    ExecuteCheck_ADV_FP(func_call, 0, { return false; }) // 0 = FPSENSOR_OK

#endif //_FP_SENSOR_HAL_H_