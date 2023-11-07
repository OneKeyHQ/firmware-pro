#ifndef __I2C_H__
#define __I2C_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include STM32_HAL_H

#define I2C_MASTER_TOTAL 2

#define ExecuteCheck_ADV_I2C(func_call, expected_result, on_false) \
  {                                                                \
    if ((func_call) != (expected_result)) {                        \
      on_false                                                     \
    }                                                              \
  }

#define ExecuteCheck_HAL_OK(func_call) \
  ExecuteCheck_ADV_I2C(func_call, HAL_OK, { return false; })

typedef enum I2C_SLAVE {
  I2C_TOUCHPANEL,
  I2C_SE,
  I2C_CAMERA,
} i2c_slave;

typedef enum I2C_MASTER {
  I2C_1,  // TOUCHPANEL
  I2C_4,  // SE and CAMERA
  I2C_UNKNOW = -1,
} i2c_master;

// handles
extern I2C_HandleTypeDef i2c_handles[I2C_MASTER_TOTAL];

// init status
extern bool i2c_status[I2C_MASTER_TOTAL];

// init function and arrays
bool I2C_1_INIT();
bool I2C_1_DEINIT();
bool I2C_4_INIT();
bool I2C_4_DEINIT();
typedef bool (*i2c_init_function_t)(void);
extern i2c_init_function_t i2c_init_function[I2C_MASTER_TOTAL];
typedef bool (*i2c_deinit_function_t)(void);
extern i2c_deinit_function_t i2c_deinit_function[I2C_MASTER_TOTAL];

// helper functions

bool i2c_deinit_by_bus(i2c_master master);

i2c_master i2c_find_master_by_slave(i2c_slave slave);
bool is_i2c_initialized_by_device(i2c_slave slave);
bool i2c_init_by_device(i2c_slave slave);
bool i2c_deinit_by_device(i2c_slave slave); // make sure you understand what you doing!

#endif  // __I2C_H__