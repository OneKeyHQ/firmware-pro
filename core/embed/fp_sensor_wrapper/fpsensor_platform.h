/*
 * Copyright (c) 2018, Beijing Chipone System Technology Co.,Ltd.
 * All rights reserved.
 *
 * File Name: filename.h/.c
 * Description: ICN7153 驱动平台支持函数接口
 *
 */
#ifndef __FPSENSOR_PLATFORM_H
#define __FPSENSOR_PLATFORM_H
#include <stdint.h>

#include "fp_sensor_hal.h"

#define FINGER_ADDR_START    (FMC_SDRAM_BOOLOADER_BUFFER_ADDRESS) // 存储地址用户根据使用情况分配
#define FINGER_ID_LIST_START (FINGER_ADDR_START + 0x100)
#define TEMPLATE_ADDR_START  (FINGER_ADDR_START + 0x1000)

uint8_t fpsensor_gpio_init(void);
uint8_t fpsensor_hard_reset(void); // hard reset

uint8_t fpsensor_spi_init(void);
uint8_t fpsensor_spi_transceive(uint8_t* buffer, int32_t length);
uint8_t fpsensor_spi_set_cs(uint8_t level);

void fpsensor_delay_ms(uint32_t num);

uint8_t SF_Init(uint32_t startAddr, uint32_t ucMmSize);
uint8_t SF_ReadData(uint8_t* buffer, uint32_t addr, uint32_t length);
uint8_t SF_WriteData(uint8_t* buffer, uint32_t addr, uint32_t length);

#endif
