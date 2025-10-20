/*!
    \file    gsl61xx.h
    \brief   gsl61xx

    \version 2023-06-30, V1.0.0, demo for GD32_xD_W515_EVAL
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef GSL61XX_H
#define GSL61XX_H

#include "stdint.h"
#include "stdbool.h"

// *************************************************

#define IMG_WIDTH       36  // image size - width
#define IMG_HEIGHT      160 // image size - height

#define IMG_BUFFER_SIZE ((IMG_WIDTH + 4) * IMG_HEIGHT)

#define USE_CALI
#define DYNMIC_DC 0

// *************************************************

// typedef uint8_t BYTE;
// typedef uint8_t UINT8;
// typedef uint16_t UINT16;
// typedef uint32_t UINT32;

#define SPI_DELAY           1
#define FINGER_UP           1
#define FINGER_DOWN         0
#define MIN_COVERAGE        80
#define DAC_PREV            1
#define DAC_TUNE_OFFSET_MAX 0x10
#define DAC_TUNE_OFFSET_MIN 0x04
#define DAC_TUNE_STEP_MAX   0x04
#define DAC_TUNE_CNT_MAX    3

typedef struct Chip_Config
{
    uint32_t Addr;
    uint32_t Data;
} Chip_Config_T;

typedef struct GSL_FP_Config
{
    Chip_Config_T* Config_NormalMode;
    uint32_t Config_NormalMode_Len;
    Chip_Config_T* Config_Interrupt;
    uint32_t Config_Interrupt_Len;

    Chip_Config_T* Config_upInterrupt;
    uint32_t Config_upInterrupt_Len;
} GSL_FP_Config_T;

typedef struct GSL_FP_Register
{
    uint32_t RegGain12;
    uint32_t RegGain34;
    uint32_t RegDac;
} GSL_FP_Register_T;

typedef struct GSL_FP_Sensor
{
    uint32_t ImageW;
    uint32_t ImageH;
    uint32_t ImageSize;
    uint8_t Gain12Ratio[8];
    uint8_t Gain34Ratio[8];
} GSL_FP_Sensor_T;

typedef struct GSL_FP_Tune
{
    uint8_t DacShift;
    uint8_t DacMax;
    uint8_t DacMin;
} GSL_FP_Tune_T;

typedef struct GSL_FP_Detect
{
    uint32_t DownThreshold;
    uint32_t UpThreshold;
    uint32_t DetGain12;
    uint32_t DetGain34;
    uint32_t DetDac;
} GSL_FP_Detect_T;

typedef struct GSL_FP_Capture
{
    uint32_t CapGain12;
    uint32_t CapGain34;
    uint32_t CapDac;
} GSL_FP_Capture_T;

typedef struct GSL_FP_Data
{
    GSL_FP_Config_T Config;
    GSL_FP_Register_T Reg;
    GSL_FP_Sensor_T Sensor;
    GSL_FP_Tune_T Tune;
    GSL_FP_Detect_T Detect;
    GSL_FP_Capture_T Capture;
} GSL_FP_Data_T;

uint8_t GSL61xx_init(void);

uint32_t DATA_FLASH_SectorSize();
void DATA_FLASH_Read(uint32_t addr, uint8_t* buffer, uint32_t len);
void DATA_FLASH_Write(uint32_t addr, uint8_t* buffer, uint32_t len);
void DATA_FLASH_Clear(uint32_t addr, uint32_t len);
void DATA_FLASH_Wipe(void);

// uint8_t FP_Spi_WriteRead(uint8_t Data);
void FP_Spi_RegRead(uint8_t addr, uint32_t* data);
void FP_Spi_RegRead_2(uint8_t addr, uint32_t* data);
void FP_Spi_RegWrite(uint8_t addr, uint32_t data);
void FP_Spi_RegRead32(uint32_t addr, uint32_t* data);
void FP_Spi_RegWrite32(uint32_t addr, uint32_t data);
void FP_Spi_RegWrite_master(uint32_t len);

void sensorEnterNormalMode(void);

void sensorEnterDownInterruptMode(void);

void sensorEnterUpInterruptMode(void);

void GSL_FP_GetSetupData(void);

void GSL_FP_ReadImageData(uint32_t page, uint8_t reg, uint8_t* buf, uint32_t len);

void GSL_FP_CaptureStart(void);

uint32_t GSL_FP_GetCaptureStatus(void);

void GSL_FP_WaitCaptureEnd(uint32_t DelayMS, uint32_t TimeOut);

void GSL_FP_ReadChipID(uint32_t* ChipID);

uint32_t GSL_FP_SensorSetup(void);

uint8_t
GSL_FP_GetDacShift(uint32_t Gain12, uint32_t Gain34, uint8_t* Gain12R, uint8_t* Gain34R, uint32_t Ref);

void GSL_FP_SensorTune(void);

uint8_t GSL_FP_DacTune(uint8_t* Image, uint32_t Width, uint32_t Height, uint32_t* TuneDac, uint32_t Mean);

uint8_t coverage61(uint8_t* buf, int w, int h, uint32_t* Mean);

uint8_t GSL_FP_CaptureImage_dynamicDC(uint8_t* pImageBmp);

uint8_t CaptureGSL61xx(uint8_t* pImageBmp);

void GSL_FP_LoadConfig(Chip_Config_T* ChipConfig, uint32_t ConfigLen);

#endif
