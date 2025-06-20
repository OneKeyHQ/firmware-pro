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

#define TEMPLATE_LENGTH         (12 * 1024)
#define TEMPLATE_DATA_CRC_LEN   4
#define TEMPLATE_TOTAL_LENGTH   (TEMPLATE_LENGTH + TEMPLATE_DATA_CRC_LEN)

#define MAX_FINGERPRINT_COUNT   6
#define FP_TEMPLATE_GROUP_COUNT 3

#define FINGER_ID_LIST_OFFSET   (0x100)
#define TEMPLATE_ADDR_OFFSET    (0x1000)

#define FINGER_ID_LIST_START    (FINGER_ADDR_START + FINGER_ID_LIST_OFFSET)
#define TEMPLATE_ADDR_START     (FINGER_ADDR_START + TEMPLATE_ADDR_OFFSET)

#define FINGER_DATA_HEADER_SIZE (0x1000)
#define FINGER_DATA_SIZE        (MAX_FINGERPRINT_COUNT * TEMPLATE_LENGTH)
#define FINGER_DATA_TOTAL_SIZE  (FINGER_DATA_HEADER_SIZE + FINGER_DATA_SIZE)

#define FINGER_DATA_VERSION_OLD 0x00000000
#define FINGER_DATA_VERSION_NEW 0x46505631 // FPV1

typedef struct
{
    union
    {
        struct
        {
            uint32_t version;
            uint8_t id_group1[4];
            uint8_t id_group2[4];
            uint32_t prompt_flag;
        } ver;
        uint8_t unknown[FINGER_ID_LIST_OFFSET];
    } header;
    uint8_t id_list[TEMPLATE_ADDR_OFFSET - FINGER_ID_LIST_OFFSET];
    uint8_t template[MAX_FINGERPRINT_COUNT * TEMPLATE_TOTAL_LENGTH];
} finger_storage_data_t;

uint8_t fpsensor_gpio_init(void);
uint8_t fpsensor_hard_reset(void); // hard reset

uint8_t fpsensor_spi_init(void);
uint8_t fpsensor_spi_transceive(uint8_t* buffer, int32_t length);
uint8_t fpsensor_spi_set_cs(uint8_t level);

void fpsensor_delay_ms(uint32_t num);

void fpsensor_irq_enable(void);
void fpsensor_irq_disable(void);
int fpsensor_detect(void);
void fpsensor_state_set(bool state);

bool fpsensor_data_init(void);
bool fpsensor_data_init_start(void);
bool fpsensor_data_init_read(void);
bool fpsensor_data_init_read_remaining(void);
bool fpsensor_data_inited(void);
bool fpsensor_data_save(uint8_t index);
bool fpsensor_data_delete(bool all, uint8_t id);
bool fpsensor_data_delete_group(uint8_t group_id[4]);
void fpsensor_data_cache_clear(void);
void fpsensor_data_get_group(uint8_t group[8]);
void fpsensor_template_cache_clear(bool clear_data);
int fpsensor_get_max_template_count(void);
bool fpsensor_data_version_is_new(void);
void fpsensor_data_upgrade_prompted(void);
bool fpsensor_data_upgrade_is_prompted(void);
uint8_t SF_Init(uint32_t startAddr, uint32_t ucMmSize);
uint8_t SF_ReadData(uint8_t* buffer, uint32_t offset, uint32_t length);
uint8_t SF_WriteData(uint8_t* buffer, uint32_t offset, uint32_t length);

#endif
