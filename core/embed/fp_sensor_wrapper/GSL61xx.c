

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

#include <string.h>

// #include "fp_algo_interface.h"
#include "fp_sensor_hal.h"
#include "GSL61xx.h"

#define MIN(a, b)     \
  ({                  \
  typeof(a) _a = (a); \
  typeof(b) _b = (b); \
  _a < _b ? _a : _b;  \
  })

// private data

// #ifdef GSL_6150_P1
Chip_Config_T Config_6157_NormalMode[] = {
    {0x00000098, 0x20044710}, //
    {0x000000E4, 0x00000005}, //
    {0x000000E8, 0x00010043}, //
    {0xFF010008, 0x00000039}, //
    {0xFF000038, 0x00000020}, //
    {0xFF000048, 0x00000223}, //
    {0xFF00004C, 0x00000081}, //
    {0xFF000050, 0x0001A823}, //
    {0xFF000034, 0x00000070}, //
    {0xFF08084C, 0x00000001}, //
    {0xFF080880, 0x20050000}, //
    {0xFF080870, 0x0081005D}, //
    {0xFF08002C, 0x00000080}, //
    {0xFF080054, 0x000005A0}, //
    {0xFF080150, 0x009F0008}, //
    {0xFF080154, 0x00000000}, //
    {0xFF08000C, 0x00006868}, //
    {0xFF000020, 0x00224444}, //
    {0xFF080030, 0x003D0003}, //
    {0xFF08007C, 0x008000C0}, //
    {0xFF080000, 0x20226040}, //
    {0xFF0800AC, 0x0011005B}, //
    {0xFF080884, 0x00590026}, //
    {0xFF080874, 0x005A0034}, //
    {0xFF080878, 0x00570047}, //
    {0xFF080868, 0x00290015}, //
    {0xFF08085C, 0x002D0017}, //
    {0xFF080860, 0x00210017}, //
    {0xFF080864, 0x002D0023}, //
    {0xFF08086C, 0x0056002E}, //
    {0xFF080870, 0x005A0031}, //
    {0xFF08001C, 0x00808001}, //
    {0xff080054, 0x000005a0}, //
    {0xff080150, 0x009f0008}, //
    {0xff080154, 0x00000000}, //
    {0xff080188, 0x0002f000}, //
    {0xff080110, 0x9f010009}, //
    {0xff080164, 0xffffffff}, //
    {0xff080168, 0xffffffff}, //
    {0xff08016c, 0xffffffff}, //
    {0xff080130, 0x00000101}, //
    {0xff080000, 0x20227040}, //
};
// Chip_Config_T Config_6157_NormalMode[] = {
// {0x000000E4,0x00000005},
// {0x000000E8,0x00010043},
// {0xFF010008,0x00000039},
// {0xFF000038,0x00000020},
// {0xFF000048,0x00000223},
// {0xFF00004C,0x00000081},
// {0xFF000050,0x0001A823},
// {0xFF000034,0x00000070},
// {0xFF08084C,0x00000001},
// {0xFF080880,0x20050000},
// {0xFF080870,0x0081005D},
// {0xFF08002C,0x00000080},
// {0xFF080054,0x000005A0},
// {0xFF080150,0x009F0008},
// {0xFF080154,0x00000000},
// {0xFF08000C,0x00006868},
// {0xFF000020,0x00225555},
// {0xFF080030,0x003D0003},
// {0xFF08007C,0x008000C0},
// {0xFF080000,0x20226040},
// {0xFF0800AC,0x0011005B},
// {0xFF080884,0x00590026},
// {0xFF080874,0x005A0034},
// {0xFF080878,0x00570047},
// {0xFF080868,0x00290015},
// {0xFF08085C,0x002D0017},
// {0xFF080860,0x00210017},
// {0xFF080864,0x002D0023},
// {0xFF08086C,0x0056002E},
// {0xFF080870,0x005A0031},
// {0xFF08001C,0x00808001},//600K
// };
// Chip_Config_T Config_6157_NormalMode[] = {
//     // {0x000000bf, 0x00000000}, //
//     {0x00000098, 0x20044710}, //
//     {0x000000e4, 0x00000005}, //
//     {0x000000e8, 0x00040043}, //
//     {0xff010008, 0x00000039}, //
//     {0xff000038, 0x00000020}, //
//     {0xff000048, 0x00000223}, //
//     {0xff00004c, 0x00000081}, //
//     {0xff000050, 0x0001a823}, //
//     {0xff000034, 0x00000070}, //
//     {0xff08084c, 0x00000001}, //
//     {0xff080880, 0x20050000}, //
//     {0xff080870, 0x0081005d}, //
//     {0xff08002c, 0x00000080}, //
//     {0xff080054, 0x000005a0}, //
//     {0xff080150, 0x009f0008}, //
//     {0xff080154, 0x00000000}, //
//     {0xff08000c, 0x00006868}, //
//     {0xff000020, 0x00225555}, //
//     {0xff080030, 0x003d0003}, //
//     {0xff08007c, 0x008000c0}, //
//     {0xff080000, 0x20226040}, //
//     // {0xff080188, 0x0001f000}, //
// };

Chip_Config_T Config_6157_Interrupt[] = {
    {0x000000bf, 0x00000000}, {0xff08084c, 0x00000000}, {0xff080034, 0x10001000}, {0xff080038, 0x0f000020},
    {0xff080124, 0x00000018}, {0xff080128, 0x80000000}, {0xff08012c, 0x80000000}, {0xff080160, 0xa0010801},
    {0xff010004, 0x00000900}, {0xff000080, 0x00000701}, {0x00000090, 0x0000800b}, {0xff000020, 0x00229999},
    {0xff08000c, 0x00007575}, {0xff080850, 0x9e003c00}, {0xFF080030, 0x011D0001},
};

Chip_Config_T Config_6157_upInterrupt[] = {
    {0x000000BF, 0x00000000}, {0xFF08084C, 0x00000000}, {0xFF080034, 0x12001000}, {0xFF080038, 0x0F000020},
    {0xFF080124, 0x00000018}, {0xFF080128, 0x80000000}, {0xFF08012C, 0x80000000}, {0xFF080160, 0xA0010801},
    {0xFF010004, 0x00000900}, {0xFF000080, 0x00000701}, {0x00000090, 0x0000800B}, {0xFF000020, 0x00229999},
    {0xFF08000C, 0x00007575}, {0xFF080030, 0x011D0001},
};

GSL_FP_Data_T GSL_FP_6157GS1P = {
    {
        Config_6157_NormalMode,
        sizeof(Config_6157_NormalMode) / sizeof(Chip_Config_T),
        Config_6157_Interrupt,
        sizeof(Config_6157_Interrupt) / sizeof(Chip_Config_T),
        Config_6157_upInterrupt,
        sizeof(Config_6157_upInterrupt) / sizeof(Chip_Config_T),
    },
    {0xFF000020, 0xFF00002c, 0xFF08000c},
    {IMG_WIDTH,
     IMG_HEIGHT,
     IMG_WIDTH* IMG_HEIGHT,
     {10, 20, 40, 56, 70, 80, 112, 140},
     {10, 20, 30, 40, 50, 60, 70, 80}},
    {0x0F, 0xF0, 0x02},
    {120, 220, 0, 0, 0},
    {0, 0, 0x00006868}};

// globals

GSL_FP_Data_T* GSL_FP;
uint32_t g_DAC_value = 0;
uint32_t g_chip_id;
uint8_t gsl_spi_send_array[128];
uint32_t g_addr_whole;
uint32_t g_reg_flush_tx_count;
uint32_t TuneDac;
uint8_t Coverge = 0;

fp_sensor_ll_interface_t* fpSensorLLIF = NULL;

uint8_t GSL61xx_init(void)
{
    fpSensorLLIF = fp_senrsor_get_LLIF(5, 1);
    fpSensorLLIF->bus_ctrl(true);
    fpSensorLLIF->gpio_ctrl(true);
    fpSensorLLIF->data_init(false);
    fpSensorLLIF->seq_chip_reset();
    GSL_FP_SensorSetup();
    GSL_FP_SensorTune();

    fpSensorLLIF->seq_chip_reset();
    GSL_FP_LoadConfig(GSL_FP->Config.Config_NormalMode, GSL_FP->Config.Config_NormalMode_Len);

    return 0;
}

uint32_t DATA_FLASH_SectorSize()
{
    return fpSensorLLIF->data_sector_size;
}

void DATA_FLASH_Read(uint32_t addr, uint8_t* buffer, uint32_t len)
{
    fpSensorLLIF->data_read(addr, buffer, len);
}

void DATA_FLASH_Write(uint32_t addr, uint8_t* buffer, uint32_t len)
{
    fpSensorLLIF->data_write(addr, buffer, len);
}

void DATA_FLASH_Clear(uint32_t addr, uint32_t len)
{
    if ( addr == 0x1000 && len == 0x60000 )
    {
        // total clear
        fpSensorLLIF->data_init(true);
    }
    else
    {
        // area wipe
        uint32_t block_size = MIN(64 * 1024, len);
        uint8_t FFs[block_size];
        memset(FFs, 0xff, block_size);
        for ( uint32_t i = 0; i < len; i += block_size )
        {
            fpSensorLLIF->data_write(addr + i, FFs, sizeof(uint32_t));
        }
    }
}
void DATA_FLASH_Wipe(void)
{
    fpSensorLLIF->data_init(true);
}

void FP_Spi_RegRead(uint8_t addr, uint32_t* data)
{
    // fpSensorLLIF->delay_clk(SPI_DELAY);

    uint8_t buf_w[] = {addr, 0x00};
    uint8_t buf_r[4];

    fpSensorLLIF->bus_rw(buf_w, sizeof(buf_w), buf_r, sizeof(buf_r));

    *data = *((uint32_t*)(buf_r));
}

void FP_Spi_RegRead_2(uint8_t addr, uint32_t* data)
{
    // fpSensorLLIF->delay_clk(SPI_DELAY);

    uint8_t buf_w[] = {addr, 0x00, 0x00};
    uint8_t buf_r[4];

    fpSensorLLIF->bus_rw(buf_w, sizeof(buf_w), buf_r, sizeof(buf_r));

    *data = *((uint32_t*)(buf_r));
}

void FP_Spi_RegWrite(uint8_t addr, uint32_t data)
{
    // fpSensorLLIF->delay_clk(SPI_DELAY);

    uint8_t buf_w[2 + sizeof(uint32_t)] = {
        addr,
        0xFF,
    };
    memcpy(buf_w + 2, &data, sizeof(uint32_t));

    fpSensorLLIF->bus_write(buf_w, sizeof(buf_w));
}

void FP_Spi_RegRead32(uint32_t addr, uint32_t* data)
{
    if ( addr < 0x100 )
    {
        FP_Spi_RegRead(addr, data);
    }
    else
    {
        // send
        // fpSensorLLIF->delay_clk(SPI_DELAY);

        uint32_t page = (addr / 0x80);
        uint8_t buf_w[2 + sizeof(uint32_t)] = {
            0xF0,
            0xFF,
        };
        memcpy(buf_w + 2, &page, sizeof(uint32_t));

        fpSensorLLIF->bus_write(buf_w, sizeof(buf_w));

        // read
        // fpSensorLLIF->delay_clk(SPI_DELAY);

        uint8_t reg = (uint8_t)(addr % 0x80);

        FP_Spi_RegRead_2(reg, data);
    }
}

void FP_Spi_RegWrite32(uint32_t addr, uint32_t data)
{
    uint32_t page = addr / 0x80;
    uint32_t reg = addr % 0x80;

    FP_Spi_RegWrite(0xF0, page);
    FP_Spi_RegWrite((uint8_t)reg, data);
}

void FP_Spi_RegWrite_master(uint32_t len)
{
    // fpSensorLLIF->delay_clk(SPI_DELAY);

    fpSensorLLIF->bus_write(gsl_spi_send_array, (uint16_t)len);
}

void sensorEnterNormalMode(void)
{
    fpSensorLLIF->seq_chip_reset();
    GSL_FP_LoadConfig(GSL_FP->Config.Config_NormalMode, GSL_FP->Config.Config_NormalMode_Len);
}

void sensorEnterDownInterruptMode(void)
{
    GSL_FP_LoadConfig(GSL_FP->Config.Config_Interrupt, GSL_FP->Config.Config_Interrupt_Len);
}

void sensorEnterUpInterruptMode(void)
{
    GSL_FP_LoadConfig(GSL_FP->Config.Config_upInterrupt, GSL_FP->Config.Config_upInterrupt_Len);
}

void GSL_FP_GetSetupData(void)
{
    GSL_FP = &GSL_FP_6157GS1P;
}

void GSL_FP_ReadImageData(uint32_t page, uint8_t reg, uint8_t* buf, uint32_t len)
{
    FP_Spi_RegWrite(0xF0, page);

    uint8_t buf_w[] = {0x00, 0x00, 0x00};
    uint8_t buf_r[len];

    fpSensorLLIF->bus_rw(buf_w, sizeof(buf_w), buf_r, sizeof(buf_r));
    memcpy(buf, buf_r, len);

    // fpSensorLLIF->bus_read(buf, (uint16_t)len+3);
}

void GSL_FP_CaptureStart(void)
{
    FP_Spi_RegWrite(0xBF, 0x00);
    FP_Spi_RegWrite32(0xFF080024, 0x00000000);
}

uint32_t GSL_FP_GetCaptureStatus(void)
{
    uint32_t Status = 0;

    FP_Spi_RegRead(0xbf, &Status);
    return Status;
}

void GSL_FP_WaitCaptureEnd(uint32_t DelayMS, uint32_t TimeOut)
{
    uint32_t TryTime = HAL_GetTick();

    while ( GSL_FP_GetCaptureStatus() != 2 )
    {
        if ( HAL_GetTick() > TryTime + TimeOut )
        {
            break;
        }
    }
}

void GSL_FP_ReadChipID(uint32_t* ChipID)
{
    FP_Spi_RegRead(0xFC, ChipID);
}

uint32_t GSL_FP_SensorSetup(void)
{
    int i;
    uint32_t ChipID;
    Chip_Config_T* Config;
    uint32_t ConfigLen;

    GSL_FP_ReadChipID(&ChipID);
    GSL_FP_GetSetupData();
    Config = GSL_FP->Config.Config_NormalMode;
    ConfigLen = GSL_FP->Config.Config_NormalMode_Len;

    for ( i = 0; i < ConfigLen; i++, Config++ )
    {
        if ( Config->Addr == GSL_FP->Reg.RegGain12 )
        {
            GSL_FP->Detect.DetGain12 = Config->Data;
            GSL_FP->Capture.CapGain12 = Config->Data;
        }
        else if ( Config->Addr == GSL_FP->Reg.RegGain34 )
        {
            GSL_FP->Detect.DetGain34 = Config->Data;
            GSL_FP->Capture.CapGain34 = Config->Data;
        }
        else if ( Config->Addr == GSL_FP->Reg.RegDac )
        {
            GSL_FP->Detect.DetDac = Config->Data;
            GSL_FP->Capture.CapDac = Config->Data;
        }
    }
    return ChipID;
}

uint8_t GSL_FP_GetDacShift(uint32_t Gain12, uint32_t Gain34, uint8_t* Gain12R, uint8_t* Gain34R, uint32_t Ref)
{
    uint8_t Gain2 = (uint8_t)((Gain12 >> 16) & 0x7);
    uint8_t Gain3 = (uint8_t)(Gain34 & 0x7);
    uint8_t Gain4 = (uint8_t)((Gain34 >> 8) & 0x7);
    uint8_t Ref2 = (uint8_t)((Ref >> 4) & 0x7);
    uint8_t Ref3 = (uint8_t)((Ref >> 8) & 0x7);
    uint8_t Ref4 = (uint8_t)((Ref >> 12) & 0x7);
    uint32_t DacShift;

    DacShift = 374 * (Gain12R[Gain2] * Gain34R[Gain3] * Gain34R[Gain4]) /
               (Gain12R[Ref2] * Gain34R[Ref3] * Gain34R[Ref4]) / 100;

    if ( DacShift < 1 )
    {
        DacShift = 1;
    }
    return (uint8_t)DacShift;
}

void GSL_FP_SensorTune(void)
{
    uint32_t DacBase;

#if 0
    GSL_FP->Tune.DacShift = GSL_FP_GetDacShift(GSL_FP->Capture.CapGain12,
                            GSL_FP->Capture.CapGain34,
                            GSL_FP->Sensor.Gain12Ratio,
                            GSL_FP->Sensor.Gain34Ratio,
                            0x1227);
#endif

    DacBase = (GSL_FP->Capture.CapDac << 24) >> 24;

    if ( DacBase > DAC_TUNE_OFFSET_MIN )
    {
        GSL_FP->Tune.DacMin = DacBase - DAC_TUNE_OFFSET_MIN;
    }
    else
    {
        GSL_FP->Tune.DacMin = 0x00;
    }

    if ( DacBase < 0xFF - DAC_TUNE_OFFSET_MAX )
    {
        GSL_FP->Tune.DacMax = DacBase + DAC_TUNE_OFFSET_MAX;
    }
    else
    {
        GSL_FP->Tune.DacMax = 0xFF;
    }
}

uint8_t GSL_FP_DacTune(uint8_t* Image, uint32_t Width, uint32_t Height, uint32_t* TuneDac, uint32_t Mean)
{
    int i, j, n;
    uint8_t Ret = 1;
    uint8_t Dac = (uint8_t)((*TuneDac) & 0xFF);
    uint8_t DacShift = GSL_FP->Tune.DacShift;
    uint8_t DacMax = GSL_FP->Tune.DacMax;
    uint8_t DacMin = GSL_FP->Tune.DacMin;
    uint8_t DacOffset;
    uint32_t StartX, StartY;
    uint32_t Temp = 0;
    uint32_t Max = 0;
    uint8_t MEAN_MIN = 120;
    uint8_t MEAN_MAX = 160;
    uint8_t MEAN_MIDDLE = 0;
    uint32_t average = 0;
    uint32_t Min = 255, Min2 = 255;
    {
        MEAN_MIN = 120;
        MEAN_MAX = 160;
    }

    MEAN_MIDDLE = (MEAN_MIN + MEAN_MAX) / 2;
    if ( Mean < MEAN_MIN )
    {
        DacOffset = (MEAN_MIDDLE - Mean) / DacShift;
        if ( DacOffset > DAC_TUNE_STEP_MAX )
        {
            DacOffset = DAC_TUNE_STEP_MAX;
        }
        else if ( DacOffset < 1 )
        {
            DacOffset = 1;
        }

        if ( Dac > DacOffset )
        {
            Dac -= DacOffset;
        }
        else
        {
            Dac = 0x68;
        }
    }
    else if ( Mean > MEAN_MAX )
    {
        DacOffset = (Mean - MEAN_MIDDLE) / DacShift;
        if ( DacOffset > DAC_TUNE_STEP_MAX )
        {
            DacOffset = DAC_TUNE_STEP_MAX;
        }
        else if ( DacOffset < 1 )
        {
            DacOffset = 1;
        }

        if ( Dac < 0xFF - DacOffset )
        {
            Dac += DacOffset;
        }
        else
        {
            Dac = 0x68;
        }
    }
    else
    {
        Ret = 0;
    }
    (*TuneDac) = Dac;
    (*TuneDac) += (*TuneDac) << 8;

    return Ret;
}

uint8_t coverage61(uint8_t* buf, int w, int h, uint32_t* Mean)
{
    unsigned short cov = 0;
    uint8_t c = 0;
    if ( buf == NULL || w < 1 || h < 1 )
    {
        return 0;
    }
    for ( int i = 0; i < h; i++ )
    {
        for ( int j = 0; j < w; j++ )
        {
            if ( buf[i * w + j] < 210 )
            {
                cov++;
            }
            *Mean += buf[i * 36 + j];
        }
    }
    *Mean /= h * w;

    c = (uint32_t)(cov * 100) / (uint32_t)(w * h);

    if ( c >= 100 )
    {
        return 100;
    }
    return c;
}

uint8_t GSL_FP_CaptureImage_dynamicDC(uint8_t* pImageBmp)
{
    int8_t Ret = 1;
    uint8_t CaptureCnt = 0;
    uint32_t Mean = 0;
    TuneDac = GSL_FP->Capture.CapDac;

    for ( CaptureCnt = 0; CaptureCnt < DAC_TUNE_CNT_MAX; CaptureCnt++ )
    {
        FP_Spi_RegWrite32(GSL_FP->Reg.RegDac, TuneDac);
        g_DAC_value = TuneDac;
        GSL_FP_CaptureStart();
        GSL_FP_WaitCaptureEnd(5, 30);                                        // 10ms
        GSL_FP_ReadImageData(0, 0, pImageBmp, GSL_FP->Sensor.ImageSize + 3); // 60ms
        // icache_enable();
        Coverge = coverage61(pImageBmp, IMG_WIDTH, IMG_HEIGHT, &Mean);
        // icache_disable();
        if ( Coverge < MIN_COVERAGE )
        {
            Ret = -1;
            break;
        }

        Ret = GSL_FP_DacTune(pImageBmp, GSL_FP->Sensor.ImageW, GSL_FP->Sensor.ImageH, &TuneDac, Mean); // 2ms
        if ( 0 == Ret )
        {
            GSL_FP->Capture.CapDac = TuneDac;
            break;
        }
    }
    if ( Ret == -1 )
    {
        GSL_FP->Capture.CapDac = TuneDac;
    }

    GSL_FP->Capture.CapDac = TuneDac;

    return Ret;
}

uint8_t CaptureGSL61xx(uint8_t* pImageBmp)
{
    uint8_t i = 0, j = 0;
    int8_t ret = 0;
    uint16_t len = IMG_WIDTH * IMG_HEIGHT;

    fpSensorLLIF->seq_chip_reset();
    GSL_FP_LoadConfig(GSL_FP->Config.Config_NormalMode, GSL_FP->Config.Config_NormalMode_Len);

    ret = GSL_FP_CaptureImage_dynamicDC(pImageBmp);

    if ( ret == -1 )
    {
        return FINGER_UP;
    }
    return FINGER_DOWN;
}

void GSL_FP_LoadConfig(Chip_Config_T* ChipConfig, uint32_t ConfigLen)
{
    // for ( uint32_t i = 0; i < ConfigLen; i++ )
    // {
    //     FP_Spi_RegRead32(ChipConfig[i].Addr, &(ChipConfig[i].Data));
    // }
    // return;

    uint32_t i;
    uint32_t Addr;
    uint32_t Data;
    uint32_t uPageLast;
    uint32_t sPage;
    uint8_t sReg;
    Chip_Config_T* Config = ChipConfig;

    for ( i = 0; i < ConfigLen; i++, Config++ )
    {
        Addr = Config->Addr;
        Data = Config->Data;
        if ( 0 == (Addr & 0xff000000) )
        {
            if ( g_reg_flush_tx_count != 0 )
            {
                FP_Spi_RegWrite_master(g_reg_flush_tx_count); // send all cache data.
            }
            FP_Spi_RegWrite((uint8_t)Addr, Data);
        }
        else
        {
            sPage = (Addr >> 7);
            sReg = (uint8_t)(Addr & 0x7F);
            uPageLast = (g_addr_whole >> 7);
            uPageLast += (((g_addr_whole & 0x7F) == 0x7C) ? 1 : 0);
            if ( sPage != uPageLast )
            {
                if ( g_reg_flush_tx_count != 0 )
                {
                    FP_Spi_RegWrite_master(g_reg_flush_tx_count);
                }
                FP_Spi_RegWrite(0xF0, sPage);
                gsl_spi_send_array[0] = sReg;
                gsl_spi_send_array[1] = 0xFF;
                gsl_spi_send_array[2] = (Data >> 0) & 0xff;
                gsl_spi_send_array[3] = (Data >> 8) & 0xff;
                gsl_spi_send_array[4] = (Data >> 16) & 0xff;
                gsl_spi_send_array[5] = (Data >> 24) & 0xff;
                g_reg_flush_tx_count = 6;
            }
            else
            {
                if ( (Addr - g_addr_whole) == 4 )
                {
                    gsl_spi_send_array[g_reg_flush_tx_count++] = (Data >> 0) & 0xff;
                    gsl_spi_send_array[g_reg_flush_tx_count++] = (Data >> 8) & 0xff;
                    gsl_spi_send_array[g_reg_flush_tx_count++] = (Data >> 16) & 0xff;
                    gsl_spi_send_array[g_reg_flush_tx_count++] = (Data >> 24) & 0xff;
                }
                else
                {
                    if ( g_reg_flush_tx_count != 0 )
                    {
                        FP_Spi_RegWrite_master(g_reg_flush_tx_count); // send all cache data.
                    }
                    FP_Spi_RegWrite(0xF0, sPage);
                    gsl_spi_send_array[0] = sReg;
                    gsl_spi_send_array[1] = 0xFF;
                    gsl_spi_send_array[2] = (Data >> 0) & 0xff;
                    gsl_spi_send_array[3] = (Data >> 8) & 0xff;
                    gsl_spi_send_array[4] = (Data >> 16) & 0xff;
                    gsl_spi_send_array[5] = (Data >> 24) & 0xff;
                    g_reg_flush_tx_count = 6;
                }
            }
        }
        g_addr_whole = Addr;
    }
    if ( g_reg_flush_tx_count != 0 )
    {
        FP_Spi_RegWrite_master(g_reg_flush_tx_count);
    }
    g_addr_whole = 0;
    g_reg_flush_tx_count = 0;
}
