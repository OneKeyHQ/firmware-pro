#ifndef __I2C_H__
#define __I2C_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include STM32_HAL_H

#define I2C_CHANNEL_TOTAL 2

typedef enum I2C_DEVICE
{
    I2C_TOUCHPANEL,
    I2C_SE,
    I2C_CAMERA,
} i2c_device;

typedef enum I2C_CHANNEL
{
    I2C_1, // TOUCHPANEL
    I2C_4, // SE and CAMERA
    I2C_UNKNOW = -1,
} i2c_channel;

// handles
extern I2C_HandleTypeDef i2c_handles[I2C_CHANNEL_TOTAL];

// init status
extern bool i2c_status[I2C_CHANNEL_TOTAL];

// init function and arrays
bool I2C_1_INIT();
bool I2C_1_DEINIT();
bool I2C_4_INIT();
bool I2C_4_DEINIT();
typedef bool (*i2c_init_function_t)(void);
extern i2c_init_function_t i2c_init_function[I2C_CHANNEL_TOTAL];
typedef bool (*i2c_deinit_function_t)(void);
extern i2c_deinit_function_t i2c_deinit_function[I2C_CHANNEL_TOTAL];

// helper functions

// bool i2c_deinit_by_channel(i2c_channel channel);

i2c_channel i2c_find_channel_by_device(i2c_device device);
bool is_i2c_initialized_by_device(i2c_device device);
bool i2c_init_by_device(i2c_device device);
bool i2c_deinit_by_device(i2c_device device); // make sure you understand what you doing!
HAL_StatusTypeDef
i2c_send_data(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);

#endif // __I2C_H__