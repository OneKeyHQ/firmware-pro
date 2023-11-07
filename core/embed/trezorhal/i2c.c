#include "i2c.h"

// handles
I2C_HandleTypeDef i2c_handles[I2C_MASTER_TOTAL];

// init status
bool i2c_status[I2C_MASTER_TOTAL];

// init function and arrays
bool I2C_1_INIT()
{
    if ( i2c_status[I2C_1] )
        return true;

    // I2C1 GPIO Configuration
    // PB6     ------> I2C1_SCL
    // PB7     ------> I2C1_SDA
    GPIO_InitTypeDef gpio_config = {
        .Pin = GPIO_PIN_6 | GPIO_PIN_7,
        .Alternate = GPIO_AF4_I2C1,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL, // GPIO_PULLUP?
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &gpio_config);

    // I2C1 Peripherals Configuration
    i2c_handles[I2C_1].Instance = I2C1;
    i2c_handles[I2C_1].Init.Timing = 0x70B03140;
    i2c_handles[I2C_1].Init.OwnAddress1 = 0; // master
    i2c_handles[I2C_1].Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handles[I2C_1].Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handles[I2C_1].Init.OwnAddress2 = 0;
    i2c_handles[I2C_1].Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c_handles[I2C_1].Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handles[I2C_1].Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();

    ExecuteCheck_HAL_OK(HAL_I2C_Init(&i2c_handles[I2C_1]));
    ExecuteCheck_HAL_OK(HAL_I2CEx_ConfigAnalogFilter(&i2c_handles[I2C_1], I2C_ANALOGFILTER_ENABLE));
    ExecuteCheck_HAL_OK(HAL_I2CEx_ConfigDigitalFilter(&i2c_handles[I2C_1], 0));

    i2c_status[I2C_1] = true;
    return true;
}

bool I2C_1_DEINIT()
{
    if ( i2c_handles[I2C_1].Instance != NULL )
    {
        ExecuteCheck_HAL_OK(HAL_I2C_DeInit(&i2c_handles[I2C_1]));
        i2c_handles[I2C_1].Instance = NULL;
        i2c_status[I2C_1] = false;
    }
    return true;
}

bool I2C_4_INIT()
{
    if ( i2c_status[I2C_4] )
        return true;
    // I2C4 GPIO Configuration
    // PD12     ------> I2C4_SCL
    // PD13     ------> I2C4_SDA
    GPIO_InitTypeDef gpio_config = {
        .Pin = GPIO_PIN_12 | GPIO_PIN_13,
        .Alternate = GPIO_AF4_I2C4,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
    };
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_Init(GPIOD, &gpio_config);

    // I2C4 Peripherals Configuration
    i2c_handles[I2C_4].Instance = I2C4;
    i2c_handles[I2C_4].Init.Timing = 0x03401242;
    i2c_handles[I2C_4].Init.OwnAddress1 = 0; // master
    i2c_handles[I2C_4].Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    i2c_handles[I2C_4].Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    i2c_handles[I2C_4].Init.OwnAddress2 = 0;
    i2c_handles[I2C_4].Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    i2c_handles[I2C_4].Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    i2c_handles[I2C_4].Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    __HAL_RCC_I2C4_CLK_ENABLE();
    __HAL_RCC_I2C4_FORCE_RESET();
    __HAL_RCC_I2C4_RELEASE_RESET();

    ExecuteCheck_HAL_OK(HAL_I2C_Init(&i2c_handles[I2C_4]));
    ExecuteCheck_HAL_OK(HAL_I2CEx_ConfigAnalogFilter(&i2c_handles[I2C_4], I2C_ANALOGFILTER_ENABLE));
    ExecuteCheck_HAL_OK(HAL_I2CEx_ConfigDigitalFilter(&i2c_handles[I2C_4], 0));

    i2c_status[I2C_4] = true;
    return true;
}

bool I2C_4_DEINIT()
{
    if ( i2c_handles[I2C_4].Instance != NULL )
    {
        ExecuteCheck_HAL_OK(HAL_I2C_DeInit(&i2c_handles[I2C_4]));
        i2c_handles[I2C_4].Instance = NULL;
        i2c_status[I2C_4] = false;
    }
    return true;
}

i2c_init_function_t i2c_init_function[I2C_MASTER_TOTAL] = {
    &I2C_1_INIT,
    &I2C_4_INIT,
};

i2c_deinit_function_t i2c_deinit_function[I2C_MASTER_TOTAL] = {
    &I2C_1_DEINIT,
    &I2C_4_DEINIT,
};

// helper functions

i2c_master i2c_find_master_by_slave(i2c_slave slave)
{
    switch ( slave )
    {
    case I2C_TOUCHPANEL:
        return I2C_1;

    case I2C_SE:
    case I2C_CAMERA:
        return I2C_4;

    default:
        return I2C_UNKNOW;
    }
}

bool is_i2c_initialized_by_device(i2c_slave slave)
{
    i2c_master master = i2c_find_master_by_slave(slave);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_status[master];
}

bool i2c_init_by_device(i2c_slave slave)
{
    i2c_master master = i2c_find_master_by_slave(slave);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_init_function[master]();
}

bool i2c_deinit_by_device(i2c_slave slave)
{
    i2c_master master = i2c_find_master_by_slave(slave);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_deinit_function[master]();
}