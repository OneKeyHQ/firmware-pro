#include "spi.h"

#define ExecuteCheck_ADV_SPI(func_call, expected_result, on_false) \
  {                                                                \
    if ( (func_call) != (expected_result) )                        \
    {                                                              \
      on_false                                                     \
    }                                                              \
  }

#define ExecuteCheck_HAL_OK(func_call) ExecuteCheck_ADV_SPI(func_call, HAL_OK, { return false; })

// handles
SPI_HandleTypeDef spi_handles[SPI_CHANNEL_TOTAL];

// init status
bool spi_status[SPI_CHANNEL_TOTAL];

// init function and arrays
bool SPI_2_INIT()
{
    if ( spi_status[SPI_2] )
        return true;

    // SPI2 GPIO Configuration
    // PC2     ------> SPI2_MISO
    // PC3     ------> SPI2_MOSI
    // PA12    ------> SPI2_SCK
    // PA11    ------> SPI2_NSS

    GPIO_InitTypeDef gpio_A_config = {
        .Pin = GPIO_PIN_12 | GPIO_PIN_11,
        .Alternate = GPIO_AF5_SPI2,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
    };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &gpio_A_config);

    GPIO_InitTypeDef gpio_C_config = {
        .Pin = GPIO_PIN_2 | GPIO_PIN_3,
        .Alternate = GPIO_AF5_SPI2,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
    };
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_Init(GPIOC, &gpio_C_config);

    // SPI2 Peripherals Configuration
    spi_handles[SPI_2].Instance = SPI2;
    spi_handles[SPI_2].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    spi_handles[SPI_2].Init.Direction = SPI_DIRECTION_2LINES;
    spi_handles[SPI_2].Init.CLKPhase = SPI_PHASE_1EDGE;
    spi_handles[SPI_2].Init.CLKPolarity = SPI_POLARITY_LOW;
    spi_handles[SPI_2].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    spi_handles[SPI_2].Init.CRCPolynomial = 7;
    spi_handles[SPI_2].Init.DataSize = SPI_DATASIZE_8BIT;
    spi_handles[SPI_2].Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi_handles[SPI_2].Init.NSS = SPI_NSS_HARD_INPUT;
    spi_handles[SPI_2].Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handles[SPI_2].Init.Mode = SPI_MODE_SLAVE;
    spi_handles[SPI_2].Init.FifoThreshold = SPI_FIFO_THRESHOLD_16DATA;

    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_SPI2_FORCE_RESET();
    __HAL_RCC_SPI2_RELEASE_RESET();

    ExecuteCheck_HAL_OK(HAL_SPI_Init(&spi_handles[SPI_2]));

    spi_status[SPI_2] = true;
    return true;
}

bool SPI_2_DEINIT()
{
    if ( spi_handles[SPI_2].Instance != NULL )
    {
        ExecuteCheck_HAL_OK(HAL_SPI_DeInit(&spi_handles[SPI_2]));
        spi_handles[SPI_2].Instance = NULL;
        spi_status[SPI_2] = false;
    }
    return true;
}

bool SPI_3_INIT()
{
    if ( spi_status[SPI_3] )
        return true;
    // SPI3 GPIO Configuration
    // PB4     ------> SPI3_MISO
    // PD6     ------> SPI3_MOSI
    // PB3     ------> SPI3_SCK
    // PA15    ------> SPI3_NSS

    GPIO_InitTypeDef gpio_A_config = {
        .Pin = GPIO_PIN_15,

    // TODO: chose one then clean up!
#ifndef FP_USE_SOFTWARE_CS
        .Alternate = GPIO_AF6_SPI3,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
#else
        .Alternate = 0,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
#endif

    };
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &gpio_A_config);

    GPIO_InitTypeDef gpio_B_config = {
        .Pin = GPIO_PIN_3 | GPIO_PIN_4,
        .Alternate = GPIO_AF6_SPI3,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &gpio_B_config);

    GPIO_InitTypeDef gpio_D_config = {
        .Pin = GPIO_PIN_6,
        .Alternate = GPIO_AF5_SPI3,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_Init(GPIOD, &gpio_D_config);

    // SPI3 Peripherals Configuration
    spi_handles[SPI_3].Instance = SPI3;
    spi_handles[SPI_3].Init.Mode = SPI_MODE_MASTER;
    spi_handles[SPI_3].Init.Direction = SPI_DIRECTION_2LINES;
    spi_handles[SPI_3].Init.DataSize = SPI_DATASIZE_8BIT;
    spi_handles[SPI_3].Init.CLKPolarity = SPI_POLARITY_LOW;
    spi_handles[SPI_3].Init.CLKPhase = SPI_PHASE_1EDGE;
#ifndef FP_USE_SOFTWARE_CS
    // TODO: chose one then clean up!
    spi_handles[SPI_3].Init.NSS = SPI_NSS_HARD_OUTPUT;
    spi_handles[SPI_3].Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    spi_handles[SPI_3].Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
#else
    spi_handles[SPI_3].Init.NSS = SPI_NSS_SOFT;
#endif
    spi_handles[SPI_3].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    spi_handles[SPI_3].Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi_handles[SPI_3].Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handles[SPI_3].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();

    ExecuteCheck_HAL_OK(HAL_SPI_Init(&spi_handles[SPI_3]));

    spi_status[SPI_3] = true;
    return true;
}

bool SPI_3_DEINIT()
{
    if ( spi_handles[SPI_3].Instance != NULL )
    {
        ExecuteCheck_HAL_OK(HAL_SPI_DeInit(&spi_handles[SPI_3]));
        spi_handles[SPI_3].Instance = NULL;
        spi_status[SPI_3] = false;
    }
    return true;
}

bool SPI_6_INIT()
{
    if ( spi_status[SPI_6] )
        return true;
    // SPI6 GPIO Configuration
    // PG12     ------> SPI6_MISO
    // PG14     ------> SPI6_MOSI
    // PG13     ------> SPI6_SCK
    // PJ4      ------> SPI6_NSS (SOFTWARE ONLY)

    GPIO_InitTypeDef gpio_G_config = {
        .Pin = GPIO_PIN_12 | GPIO_PIN_14 | GPIO_PIN_13,
        .Alternate = GPIO_AF5_SPI6,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    __HAL_RCC_GPIOG_CLK_ENABLE();
    HAL_GPIO_Init(GPIOG, &gpio_G_config);

    GPIO_InitTypeDef gpio_J_config = {
        .Pin = GPIO_PIN_4,
        .Alternate = 0,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
    };
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    HAL_GPIO_Init(GPIOJ, &gpio_J_config);

    // SPI6 Peripherals Configuration
    spi_handles[SPI_6].Instance = SPI6;
    spi_handles[SPI_6].Init.Mode = SPI_MODE_MASTER;
    spi_handles[SPI_6].Init.Direction = SPI_DIRECTION_2LINES;
    spi_handles[SPI_6].Init.DataSize = SPI_DATASIZE_8BIT;
    spi_handles[SPI_6].Init.CLKPolarity = SPI_POLARITY_LOW;
    spi_handles[SPI_6].Init.CLKPhase = SPI_PHASE_1EDGE;
    spi_handles[SPI_6].Init.NSS = SPI_NSS_SOFT;
    spi_handles[SPI_6].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    spi_handles[SPI_6].Init.FirstBit = SPI_FIRSTBIT_LSB;
    spi_handles[SPI_6].Init.TIMode = SPI_TIMODE_DISABLE;
    spi_handles[SPI_6].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    __HAL_RCC_SPI6_CLK_ENABLE();
    __HAL_RCC_SPI6_FORCE_RESET();
    __HAL_RCC_SPI6_RELEASE_RESET();

    ExecuteCheck_HAL_OK(HAL_SPI_Init(&spi_handles[SPI_6]));

    spi_status[SPI_6] = true;
    return true;
}

bool SPI_6_DEINIT()
{
    if ( spi_handles[SPI_6].Instance != NULL )
    {
        ExecuteCheck_HAL_OK(HAL_SPI_DeInit(&spi_handles[SPI_6]));
        spi_handles[SPI_6].Instance = NULL;
        spi_status[SPI_6] = false;
    }
    return true;
}

spi_init_function_t spi_init_function[SPI_CHANNEL_TOTAL] = {
    &SPI_2_INIT,
    &SPI_3_INIT,
    &SPI_6_INIT,
};

spi_deinit_function_t spi_deinit_function[SPI_CHANNEL_TOTAL] = {
    &SPI_2_DEINIT,
    &SPI_3_DEINIT,
    &SPI_6_DEINIT,
};

// helper functions

spi_channel spi_find_channel_by_device(spi_device device)
{
    switch ( device )
    {
    case SPI_BLUETOOTH:
        return SPI_2;

    case SPI_FINGERPRINT:
        return SPI_3;

    case SPI_NFC:
        return SPI_6;

    default:
        return SPI_UNKNOW;
    }
}

bool is_spi_initialized_by_device(spi_device device)
{
    spi_channel master = spi_find_channel_by_device(device);
    if ( master == SPI_UNKNOW )
        return false;

    return spi_status[master];
}

bool spi_init_by_device(spi_device device)
{
    spi_channel master = spi_find_channel_by_device(device);
    if ( master == SPI_UNKNOW )
        return false;

    return spi_init_function[master]();
}

bool spi_deinit_by_device(spi_device device)
{
    spi_channel master = spi_find_channel_by_device(device);
    if ( master == SPI_UNKNOW )
        return false;

    return spi_deinit_function[master]();
}
