#include "i2c.h"
#include "systick.h"

#define ExecuteCheck_ADV_I2C(func_call, expected_result, on_false) \
  {                                                                \
    if ( (func_call) != (expected_result) )                        \
    {                                                              \
      on_false                                                     \
    }                                                              \
  }

#define ExecuteCheck_HAL_OK(func_call) ExecuteCheck_ADV_I2C(func_call, HAL_OK, { return false; })

// handles
I2C_HandleTypeDef i2c_handles[I2C_CHANNEL_TOTAL];

// init status
bool i2c_status[I2C_CHANNEL_TOTAL];

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
    // i2c_handles[I2C_1].Init.Timing = 0x70B03140; // ?
    i2c_handles[I2C_1].Init.Timing = 0x10C0ECFF; // pclk 100M I2C 100K
    i2c_handles[I2C_1].Init.OwnAddress1 = 0;     // master
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
    i2c_handles[I2C_4].Init.Timing = 0x10C0ECFF; // 100k
    // i2c_handles[I2C_4].Init.Timing = 0x009034B6; // 400k
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

i2c_init_function_t i2c_init_function[I2C_CHANNEL_TOTAL] = {
    &I2C_1_INIT,
    &I2C_4_INIT,
};

i2c_deinit_function_t i2c_deinit_function[I2C_CHANNEL_TOTAL] = {
    &I2C_1_DEINIT,
    &I2C_4_DEINIT,
};

// helper functions

i2c_channel i2c_find_channel_by_device(i2c_device device)
{
    switch ( device )
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

bool is_i2c_initialized_by_device(i2c_device device)
{
    i2c_channel master = i2c_find_channel_by_device(device);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_status[master];
}

bool i2c_init_by_device(i2c_device device)
{
    i2c_channel master = i2c_find_channel_by_device(device);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_init_function[master]();
}

bool i2c_deinit_by_device(i2c_device device)
{
    i2c_channel master = i2c_find_channel_by_device(device);
    if ( master == I2C_UNKNOW )
        return false;

    return i2c_deinit_function[master]();
}

#define I2C_TIMEOUT_BUSY (25U) /*!< 25 ms */
#define MAX_NBYTE_SIZE   255U

static HAL_StatusTypeDef I2C_WaitOnFlagUntilTimeout(
    I2C_HandleTypeDef* hi2c, uint32_t Flag, FlagStatus Status, uint32_t Timeout, uint32_t Tickstart
)
{
    dwt_reset();
    while ( __HAL_I2C_GET_FLAG(hi2c, Flag) == Status )
    {
        /* Check for the Timeout */
        if ( Timeout != HAL_MAX_DELAY )
        {
            if ( dwt_is_timeout(Timeout) || (Timeout == 0U) )
            {
                hi2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
                hi2c->State = HAL_I2C_STATE_READY;
                hi2c->Mode = HAL_I2C_MODE_NONE;

                /* Process Unlocked */
                __HAL_UNLOCK(hi2c);
                return HAL_ERROR;
            }
        }
    }
    return HAL_OK;
}

static HAL_StatusTypeDef
I2C_IsAcknowledgeFailed(I2C_HandleTypeDef* hi2c, uint32_t Timeout, uint32_t Tickstart)
{
    if ( __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_AF) == SET )
    {
        /* Wait until STOP Flag is reset */
        /* AutoEnd should be initiate after AF */
        dwt_reset();
        while ( __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == RESET )
        {
            /* Check for the Timeout */
            if ( Timeout != HAL_MAX_DELAY )
            {
                if ( dwt_is_timeout(Timeout) || (Timeout == 0U) )
                {
                    hi2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
                    hi2c->State = HAL_I2C_STATE_READY;
                    hi2c->Mode = HAL_I2C_MODE_NONE;

                    /* Process Unlocked */
                    __HAL_UNLOCK(hi2c);

                    return HAL_ERROR;
                }
            }
        }

        /* Clear NACKF Flag */
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_AF);

        /* Clear STOP Flag */
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);

        /* Clear Configuration Register 2 */
        I2C_RESET_CR2(hi2c);

        hi2c->ErrorCode |= HAL_I2C_ERROR_AF;
        hi2c->State = HAL_I2C_STATE_READY;
        hi2c->Mode = HAL_I2C_MODE_NONE;

        /* Process Unlocked */
        __HAL_UNLOCK(hi2c);

        return HAL_ERROR;
    }
    return HAL_OK;
}

// static HAL_StatusTypeDef I2C_WaitOnRXNEFlagUntilTimeout(I2C_HandleTypeDef *hi2c,
//                                                         uint32_t Timeout,
//                                                         uint32_t Tickstart) {
//   uint32_t timeout_counter = Timeout * 1000;
//   while (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == RESET) {
//     /* Check if a NACK is detected */
//     if (I2C_IsAcknowledgeFailed(hi2c, Timeout, Tickstart) != HAL_OK) {
//       return HAL_ERROR;
//     }

//     /* Check if a STOPF is detected */
//     if (__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == SET) {
//       /* Check if an RXNE is pending */
//       /* Store Last receive data if any */
//       if ((__HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_RXNE) == SET) &&
//           (hi2c->XferSize > 0U)) {
//         /* Return HAL_OK */
//         /* The Reading of data from RXDR will be done in caller function */
//         return HAL_OK;
//       } else {
//         /* Clear STOP Flag */
//         __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);

//         /* Clear Configuration Register 2 */
//         I2C_RESET_CR2(hi2c);

//         hi2c->ErrorCode = HAL_I2C_ERROR_NONE;
//         hi2c->State = HAL_I2C_STATE_READY;
//         hi2c->Mode = HAL_I2C_MODE_NONE;

//         /* Process Unlocked */
//         __HAL_UNLOCK(hi2c);

//         return HAL_ERROR;
//       }
//     }

//     /* Check for the Timeout */
//     if ((timeout_counter == 0) || (Timeout == 0U)) {
//       hi2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
//       hi2c->State = HAL_I2C_STATE_READY;

//       /* Process Unlocked */
//       __HAL_UNLOCK(hi2c);

//       return HAL_ERROR;
//     }

//     timeout_counter--;
//   }
//   return HAL_OK;
// }

static HAL_StatusTypeDef
I2C_WaitOnSTOPFlagUntilTimeout(I2C_HandleTypeDef* hi2c, uint32_t Timeout, uint32_t Tickstart)
{
    dwt_reset();
    while ( __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_STOPF) == RESET )
    {
        /* Check if a NACK is detected */
        if ( I2C_IsAcknowledgeFailed(hi2c, Timeout, Tickstart) != HAL_OK )
        {
            return HAL_ERROR;
        }
        /* Check for the Timeout */
        if ( dwt_is_timeout(Timeout) || (Timeout == 0U) )
        {
            hi2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
            hi2c->State = HAL_I2C_STATE_READY;
            hi2c->Mode = HAL_I2C_MODE_NONE;

            /* Process Unlocked */
            __HAL_UNLOCK(hi2c);

            return HAL_ERROR;
        }
    }
    return HAL_OK;
}

static HAL_StatusTypeDef
I2C_WaitOnTXISFlagUntilTimeout(I2C_HandleTypeDef* hi2c, uint32_t Timeout, uint32_t Tickstart)
{
    dwt_reset();
    while ( __HAL_I2C_GET_FLAG(hi2c, I2C_FLAG_TXIS) == RESET )
    {
        /* Check if a NACK is detected */
        if ( I2C_IsAcknowledgeFailed(hi2c, Timeout, Tickstart) != HAL_OK )
        {
            return HAL_ERROR;
        }
        /* Check for the Timeout */
        if ( Timeout != HAL_MAX_DELAY )
        {
            if ( dwt_is_timeout(Timeout) || (Timeout == 0U) )
            {
                hi2c->ErrorCode |= HAL_I2C_ERROR_TIMEOUT;
                hi2c->State = HAL_I2C_STATE_READY;
                hi2c->Mode = HAL_I2C_MODE_NONE;

                /* Process Unlocked */
                __HAL_UNLOCK(hi2c);

                return HAL_ERROR;
            }
        }
    }
    return HAL_OK;
}

static void I2C_TransferConfig(
    I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t Size, uint32_t Mode, uint32_t Request
)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_INSTANCE(hi2c->Instance));
    assert_param(IS_TRANSFER_MODE(Mode));
    assert_param(IS_TRANSFER_REQUEST(Request));

    /* update CR2 register */
    MODIFY_REG(
        hi2c->Instance->CR2,
        ((I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND |
          (I2C_CR2_RD_WRN & (uint32_t)(Request >> (31U - I2C_CR2_RD_WRN_Pos))) | I2C_CR2_START | I2C_CR2_STOP)
        ),
        (uint32_t
        )(((uint32_t)DevAddress & I2C_CR2_SADD) | (((uint32_t)Size << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
          (uint32_t)Mode | (uint32_t)Request)
    );
}

HAL_StatusTypeDef
i2c_send_data(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickstart;

    if ( hi2c->State == HAL_I2C_STATE_READY )
    {
        /* Process Locked */
        __HAL_LOCK(hi2c);

        /* Init tickstart for timeout management*/
        // tickstart = HAL_GetTick();

        if ( I2C_WaitOnFlagUntilTimeout(hi2c, I2C_FLAG_BUSY, SET, I2C_TIMEOUT_BUSY, tickstart) != HAL_OK )
        {
            return HAL_ERROR;
        }

        hi2c->State = HAL_I2C_STATE_BUSY_TX;
        hi2c->Mode = HAL_I2C_MODE_MASTER;
        hi2c->ErrorCode = HAL_I2C_ERROR_NONE;

        /* Prepare transfer parameters */
        hi2c->pBuffPtr = pData;
        hi2c->XferCount = Size;
        hi2c->XferISR = NULL;

        /* Send Slave Address */
        /* Set NBYTES to write and reload if hi2c->XferCount > MAX_NBYTE_SIZE and generate RESTART */
        if ( hi2c->XferCount > MAX_NBYTE_SIZE )
        {
            hi2c->XferSize = MAX_NBYTE_SIZE;
            I2C_TransferConfig(
                hi2c, DevAddress, (uint8_t)hi2c->XferSize, I2C_RELOAD_MODE, I2C_GENERATE_START_WRITE
            );
        }
        else
        {
            hi2c->XferSize = hi2c->XferCount;
            I2C_TransferConfig(
                hi2c, DevAddress, (uint8_t)hi2c->XferSize, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE
            );
        }

        while ( hi2c->XferCount > 0U )
        {
            /* Wait until TXIS flag is set */
            if ( I2C_WaitOnTXISFlagUntilTimeout(hi2c, Timeout, tickstart) != HAL_OK )
            {
                return HAL_ERROR;
            }
            /* Write data to TXDR */
            hi2c->Instance->TXDR = *hi2c->pBuffPtr;

            /* Increment Buffer pointer */
            hi2c->pBuffPtr++;

            hi2c->XferCount--;
            hi2c->XferSize--;

            if ( (hi2c->XferCount != 0U) && (hi2c->XferSize == 0U) )
            {
                /* Wait until TCR flag is set */
                if ( I2C_WaitOnFlagUntilTimeout(hi2c, I2C_FLAG_TCR, RESET, Timeout, tickstart) != HAL_OK )
                {
                    return HAL_ERROR;
                }

                if ( hi2c->XferCount > MAX_NBYTE_SIZE )
                {
                    hi2c->XferSize = MAX_NBYTE_SIZE;
                    I2C_TransferConfig(
                        hi2c, DevAddress, (uint8_t)hi2c->XferSize, I2C_RELOAD_MODE, I2C_NO_STARTSTOP
                    );
                }
                else
                {
                    hi2c->XferSize = hi2c->XferCount;
                    I2C_TransferConfig(
                        hi2c, DevAddress, (uint8_t)hi2c->XferSize, I2C_AUTOEND_MODE, I2C_NO_STARTSTOP
                    );
                }
            }
        }

        /* No need to Check TC flag, with AUTOEND mode the stop is automatically generated */
        /* Wait until STOPF flag is set */
        if ( I2C_WaitOnSTOPFlagUntilTimeout(hi2c, Timeout, tickstart) != HAL_OK )
        {
            return HAL_ERROR;
        }

        /* Clear STOP Flag */
        __HAL_I2C_CLEAR_FLAG(hi2c, I2C_FLAG_STOPF);

        /* Clear Configuration Register 2 */
        I2C_RESET_CR2(hi2c);

        hi2c->State = HAL_I2C_STATE_READY;
        hi2c->Mode = HAL_I2C_MODE_NONE;

        /* Process Unlocked */
        __HAL_UNLOCK(hi2c);

        return HAL_OK;
    }
    else
    {
        return HAL_BUSY;
    }
}
