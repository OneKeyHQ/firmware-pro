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

#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

#include STM32_HAL_H

#include "flash.h"
#include "image.h"
#include "sdram.h"
#include "secbool.h"

// Note: as Arm V7-M MPU requires region base addr aligns to region size
// some parts we have to set multiple regions to cover it
// MPU REGION LAYERS
// --- base
#define MPU_REGION_BASE MPU_REGION_NUMBER0
#define MPU_REGION_SRAMs MPU_REGION_NUMBER1
#define MPU_REGION_SDRAM MPU_REGION_NUMBER2
#define MPU_REGION_FLASH MPU_REGION_NUMBER3
#define MPU_REGION_FLASH_OTP MPU_REGION_NUMBER4

// stages
#define MPU_REGION_FLASH_BOARD MPU_REGION_NUMBER5
#define MPU_REGION_FLASH_BOOT MPU_REGION_NUMBER6
#define MPU_REGION_FLASH_FWBK1 MPU_REGION_NUMBER7
#define MPU_REGION_FLASH_FWBK2 MPU_REGION_NUMBER8
// --- misc
#define MPU_REGION_SRAM3_DMA MPU_REGION_NUMBER9
#define MPU_REGION_QSPI_FLASH MPU_REGION_NUMBER10

// --- flash subregion for stages
#define MPU_SUBREGION_MASK_BK1_BOARD (uint8_t)(~0b00000001U)
#define MPU_SUBREGION_MASK_BK1_BOOT (uint8_t)(~0b00000110U)
#define MPU_SUBREGION_MASK_BK1_FW (uint8_t)(~0b11111000U)
#define MPU_SUBREGION_MASK_BK2_FW (uint8_t)(~0b01111111U)
#define MPU_SUBREGION_MASK_BK2_OTP (uint8_t)(~0b10000000U)
#define MPU_SUBREGION_MASK_NONE 0xffU

void mpu_ctrl(secbool mpu_enable) {
  if (mpu_enable != secfalse)  // doing in this way as secfase is just zero
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  else
    HAL_MPU_Disable();
}

void mpu_config_base() {
  MPU_Region_InitTypeDef mpu_init_struct = {0};

  // BASE
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_BASE;
    mpu_init_struct.BaseAddress = 0x00000000;
    mpu_init_struct.Size = MPU_REGION_SIZE_4GB;
    mpu_init_struct.SubRegionDisable =
        0x87;  // check here
               // https://community.st.com/t5/stm32-mcus-embedded-software/information-about-mpu-settings-for-the-stm32h743/m-p/83831/highlight/true#M2917
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission = MPU_REGION_NO_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // DTCM-RAM, AXI-SRAM, SRAM1, SRAM2, SRAM3, SRAM4, SRAM5, BAK-SRAM
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_SRAMs;
    mpu_init_struct.BaseAddress = 0x20000000;
    mpu_init_struct.Size = MPU_REGION_SIZE_512MB;
    mpu_init_struct.SubRegionDisable = 0x00;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL1;
    mpu_init_struct.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // SDRAM
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_SDRAM;
    mpu_init_struct.BaseAddress = 0xD0000000;
    mpu_init_struct.Size = MPU_REGION_SIZE_32MB;
    mpu_init_struct.SubRegionDisable = 0x00;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // FLASH, write through cache,keep default config
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH;
    mpu_init_struct.BaseAddress = FLASH_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_2MB;
    mpu_init_struct.SubRegionDisable = 0x00;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission = MPU_REGION_NO_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // FLASH OTP
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH;
    mpu_init_struct.BaseAddress = FLASH_BANK2_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_1MB;
    mpu_init_struct.SubRegionDisable = MPU_SUBREGION_MASK_BK2_OTP;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission = MPU_REGION_PRIV_RW_URO;
    // mpu_init_struct.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }
}

void mpu_config_boardloader(secbool access, secbool exec) {
  MPU_Region_InitTypeDef mpu_init_struct = {0};

  // FLASH
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH_BOARD;
    mpu_init_struct.BaseAddress = FLASH_BANK1_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_1MB;
    mpu_init_struct.SubRegionDisable = MPU_SUBREGION_MASK_BK1_BOARD;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission =
        ((access == sectrue) ? MPU_REGION_FULL_ACCESS : MPU_REGION_NO_ACCESS);
    mpu_init_struct.DisableExec =
        ((exec == sectrue) ? MPU_INSTRUCTION_ACCESS_ENABLE
                           : MPU_INSTRUCTION_ACCESS_DISABLE);
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }
}

void mpu_config_bootloader(secbool access, secbool exec) {
  MPU_Region_InitTypeDef mpu_init_struct = {0};

  // FLASH
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH_BOOT;
    mpu_init_struct.BaseAddress = FLASH_BANK1_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_1MB;
    mpu_init_struct.SubRegionDisable = MPU_SUBREGION_MASK_BK1_BOOT;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission =
        ((access == sectrue) ? MPU_REGION_FULL_ACCESS : MPU_REGION_NO_ACCESS);
    mpu_init_struct.DisableExec =
        ((exec == sectrue) ? MPU_INSTRUCTION_ACCESS_ENABLE
                           : MPU_INSTRUCTION_ACCESS_DISABLE);
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // SRAM3 SPI/UART DMA
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_SRAM3_DMA;
    mpu_init_struct.BaseAddress = 0x30040000;
    mpu_init_struct.Size = MPU_REGION_SIZE_32KB;
    mpu_init_struct.SubRegionDisable = 0x00;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL1;
    mpu_init_struct.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // SPI FLASH
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_QSPI_FLASH;
    mpu_init_struct.BaseAddress = 0x90000000;
    mpu_init_struct.Size = MPU_REGION_SIZE_8MB;
    mpu_init_struct.SubRegionDisable = 0x00;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission = MPU_REGION_PRIV_RW;
    mpu_init_struct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
    mpu_init_struct.IsShareable = MPU_ACCESS_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }
}

secbool mpu_config_firmware(secbool access, secbool exec) {
  MPU_Region_InitTypeDef mpu_init_struct = {0};

  // FLASH
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH_FWBK1;
    mpu_init_struct.BaseAddress = FLASH_BANK1_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_1MB;
    mpu_init_struct.SubRegionDisable = MPU_SUBREGION_MASK_BK1_FW;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission =
        ((access == sectrue) ? MPU_REGION_FULL_ACCESS : MPU_REGION_NO_ACCESS);
    mpu_init_struct.DisableExec =
        ((exec == sectrue) ? MPU_INSTRUCTION_ACCESS_ENABLE
                           : MPU_INSTRUCTION_ACCESS_DISABLE);
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }

  // FLASH
  {
    mpu_init_struct.Enable = MPU_REGION_ENABLE;
    mpu_init_struct.Number = MPU_REGION_FLASH_FWBK2;
    mpu_init_struct.BaseAddress = FLASH_BANK2_BASE;
    mpu_init_struct.Size = MPU_REGION_SIZE_1MB;
    mpu_init_struct.SubRegionDisable = MPU_SUBREGION_MASK_BK2_FW;
    mpu_init_struct.TypeExtField = MPU_TEX_LEVEL0;
    mpu_init_struct.AccessPermission =
        ((access == sectrue) ? MPU_REGION_FULL_ACCESS : MPU_REGION_NO_ACCESS);
    mpu_init_struct.DisableExec =
        ((exec == sectrue) ? MPU_INSTRUCTION_ACCESS_ENABLE
                           : MPU_INSTRUCTION_ACCESS_DISABLE);
    mpu_init_struct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    mpu_init_struct.IsCacheable = MPU_ACCESS_CACHEABLE;
    mpu_init_struct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&mpu_init_struct);
  }
  __asm__ volatile("dsb");
  __asm__ volatile("isb");

  return sectrue;
}
