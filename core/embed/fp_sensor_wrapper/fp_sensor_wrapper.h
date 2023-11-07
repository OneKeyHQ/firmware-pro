#ifndef _FP_SENSOR_WRAPPER_H_
#define _FP_SENSOR_WRAPPER_H_

#include "fpalgorithm_interface.h"
#include "fpsensor_cac.h"
#include "fpsensor_common.h"
#include "fpsensor_driver.h"
#include "fpsensor_platform.h"

#if 0
uint8_t Fp_AlgorithmInit_old(uint32_t addr)
{
  int v2; // r4
  int v3; // r4
  int v4; // r4
  int v5; // r4
  int v6; // r4
  int v7; // r4
  char v8; // r4
  bool v9; // zf
  char v10; // r4
  uint8_t v12[16]; // [sp+0h] [bp-10h] BYREF

  v3 = v2 | fpsensor_gpio_init();
  v2 = fpsensor_spi_init();
  v4 = v3 | fpsensor_hard_reset();
  v5 = v4 | fpsensor_adc_init(0xCu, 0xCu, 0x10u, 3u);
  v6 = v5 | fpsensor_set_config_param(0xC0u, 8u);
  v7 = v6 | fpsensor_init();
  v8 = v7 | fpsensor_get_HWID(v12);
  v9 = v12[0] == 113;
  if ( v12[0] == 113 )
    v9 = v12[1] == 83;
  if ( !v9 )
    v8 = 1;
  v10 = v8 | SF_Init(addr);
  FLASH_FIRST_RUNNING_ADDRESS = addr;
  return Finger_DeleteAll() | v10;
}

int __fastcall Fp_AlgorithmInit(int address)
{
  int result; // result
  uint8_t spi_result; // [sp+Fh] [bp+Fh]
  uint8_t buffer[4]; // [sp+10h] [bp+10h] BYREF

  spi_result = fpsensor_get_HWID(buffer);
  if ( buffer[0] == 0x71 && buffer[1] == 0x52 )
    result = (spi_result | SF_Init(address, 204800));
  else
    result = 1;
  return result;
}
#endif

uint8_t fp_init(uint32_t addr)
{
    uint8_t result;
    uint8_t hwid[2];

    result = fpsensor_get_HWID(hwid);
    if ( result != FPSENSOR_OK )
        return result;

    if ( !(hwid[0] == 0x71 && hwid[1] == 0x53) )
    {
        result = FPSENSOR_SPI_ERROR;
        return result;
    }

    result = SF_Init(addr, 204800);
    if ( result != FPSENSOR_OK )
        return result;

    return 0;
}

#endif //_FP_SENSOR_WRAPPER_H_