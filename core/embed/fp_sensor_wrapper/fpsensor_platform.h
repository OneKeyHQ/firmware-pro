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
#include "fpalgorithm_interface.h"

#define FINGER_ADDR_START       0 //(FMC_SDRAM_CAMERA_BUFFER_ADDRESS) // 存储地址用户根据使用情况分配
#define FINGER_ID_LIST_START    (FINGER_ADDR_START + 0x100)
#define TEMPLATE_ADDR_START     (FINGER_ADDR_START + 0x1000)

#define FINGER_ID_LIST_OFFSET   (0x100)
#define TEMPLATE_ADDR_OFFSET    (0x1000)

#define FINGER_DATA_HEADER_SIZE (0x1000)
#define FINGER_DATA_SIZE        (MAX_FINGERPRINT_COUNT * TEMPLATE_LENGTH)
#define FINGER_DATA_TOTAL_SIZE  (FINGER_DATA_HEADER_SIZE + FINGER_DATA_SIZE)

uint8_t fpsensor_gpio_init(void);
uint8_t fpsensor_hard_reset(void); // hard reset

uint8_t fpsensor_spi_init(void);
uint8_t fpsensor_spi_transceive(uint8_t* buffer, int32_t length);
uint8_t fpsensor_spi_set_cs(uint8_t level);

void fpsensor_delay_ms(uint32_t num);

void fpsensor_irq_enable(void);
void fpsensor_irq_disable(void);
int fpsensor_detect(void);

bool fpsensor_data_init(void);
bool fpsensor_data_save(void);

uint8_t SF_Init(uint32_t startAddr, uint32_t ucMmSize);
uint8_t SF_ReadData(uint8_t* buffer, uint32_t offset, uint32_t length);
uint8_t SF_WriteData(uint8_t* buffer, uint32_t offset, uint32_t length);

#endif
