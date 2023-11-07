#include "fpsensor_platform.h"
#include "fpsensor_driver.h"

SPI_HandleTypeDef spi_fp;

/*
* Function：    fpsensor_gpio_init
* Description： MCU初始化SENSOR的RST引脚。
                要求：
                上拉推挽输出
                初始化完成后设置该PIN为高。
* Input：       无。
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
uint8_t fpsensor_gpio_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // MISC

    // SPI_INT     FP_IRQ      PB15
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // RSTn        FP_RST      PB14
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    return FPSENSOR_OK;
}
/*
* Function：    fpsensor_spi_init
* Description： 检测手指并获取指纹图像数据。
                要求：
                MCU针对于SENSOR的SPI初始化。
                SENSOR作为从设备
                SPI初始化要求
                速率:5MHz
                4线SPI
                MSBFirst
                数据大小:8Bit
                SPI_POLARITY_LOW
                SPI_PHASE_1EDGE
                软件片选
* Input：       无。
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
uint8_t fpsensor_spi_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    // SPI

    // SPI_MISO    SPI3_MISO   PB4  AF6
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SPI_MOSI    SPI3_MOSI   PD6  AF5
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    // SPI_CLK     SPI3_SCK    PB3  AF6
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SPI_CSn     SPI3_NSS    PA15 AF6
    GPIO_InitStruct.Pin = GPIO_PIN_15;
#ifdef SPI_FP_USE_HW_CS
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
#else
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
#endif
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_SPI3_CLK_ENABLE();
    __HAL_RCC_SPI3_FORCE_RESET();
    __HAL_RCC_SPI3_RELEASE_RESET();

    spi_fp.Instance = SPI3;
    spi_fp.Init.Mode = SPI_MODE_MASTER;
    spi_fp.Init.Direction = SPI_DIRECTION_2LINES;
    spi_fp.Init.DataSize = SPI_DATASIZE_8BIT;
    spi_fp.Init.CLKPolarity = SPI_POLARITY_LOW;
    spi_fp.Init.CLKPhase = SPI_PHASE_1EDGE;
#ifdef SPI_FP_USE_HW_CS
    spi_fp.Init.NSS = SPI_NSS_HARD_OUTPUT;
    spi_fp.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    spi_fp.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
#else
    spi_fp.Init.NSS = SPI_NSS_SOFT;
#endif
    spi_fp.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    spi_fp.Init.FirstBit = SPI_FIRSTBIT_MSB;
    spi_fp.Init.TIMode = SPI_TIMODE_DISABLE;
    spi_fp.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

    if ( HAL_OK != HAL_SPI_Init(&spi_fp) )
    {
        return FPSENSOR_SPI_ERROR;
    }

    return FPSENSOR_OK;
}

/*
* Function：    fpsensor_spi_transceive
* Description： MCU针对于SENSOR的SPI传递数据。
* Input：       uint8_t *buffer:
buffer承载SPI通信的数据，包含MCU发送至SENSOR的CMD(txLenght)和
                SENSOR发送至MCU输出的数据(rxLenght)

                int32_t length:  length = txLenght + rxLenght
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
uint8_t fpsensor_spi_transceive(uint8_t* buffer, int32_t length)
{
    HAL_StatusTypeDef result = HAL_ERROR;
    fpsensor_spi_set_cs(0);
    result = HAL_SPI_TransmitReceive(&spi_fp, buffer, buffer, length, 1000U);
    fpsensor_spi_set_cs(1);

    if ( HAL_OK != result )
    {
        return FPSENSOR_SPI_ERROR;
    }

    return FPSENSOR_OK;
}
/*
 * Function：    fpsensor_spi_set_cs
 * Description： MCU对CS操作。
 * Input：       uint8_t level: level = 1,拉高CS，level = 0,拉低CS
 * Output：      无。
 * Return:       0-成功。
 * Others:       无。
 */
uint8_t fpsensor_spi_set_cs(uint8_t level)
{
#ifdef SPI_FP_USE_HW_CS
    // we use hardware management, do nothing here
#else
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, (level == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
    return FPSENSOR_OK;
}
/*
* Function：    fpsensor_hard_reset
* Description： MCU产生对SENSOR的硬复位信号。
                要求：
                拉低RST引脚，延时2ms,拉高RST引脚，延时2ms。
* Input：       无。
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
uint8_t fpsensor_hard_reset()
{
    // RSTn        FP_RST      PB14
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(5);

    return FPSENSOR_OK;
}
/*
* Function：    fpsensor_delay_ms
* Description： 延时函数。
                延时ms。
* Input：       uint32_t Timeout：延时的时间大小，单位ms。
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
void fpsensor_delay_ms(uint32_t Timeout)
{
    HAL_Delay(Timeout);
}

/*
* Function：    SF_Init
* Description： 申请起始地址为startAddr，大小为ucMmSize字节大小。
* Input：       uint32_t startAddr：起始地址 直接使用头文件的 FINGER_ADDR_START
即可， uint32_t ucMmSize：连续存储空间大小,目前要求200*1024大小，至少是4k大小
* Output：      无。
* Return:       0-成功。
* Others:       无。
*/
uint8_t SF_Init(uint32_t startAddr, uint32_t ucMmSize)
{
    memset((void*)startAddr, 0x00, ucMmSize);
    return FPSENSOR_OK;
}
/*
 * Function：    SF_WriteData
 * Description： 将大小为length字节数的buffer的数据写入addr处。
 * Input：       uint8_t *buffer, uint32_t addr, uint32_t length，
 * Output：      无。
 * Return:       0-成功。
 * Others:       无。
 */
uint8_t SF_WriteData(uint8_t* buffer, uint32_t addr, uint32_t length)
{
    memcpy((void*)addr, buffer, length);
    return FPSENSOR_OK;
}
/*
 * Function：    SF_ReadData
 * Description： 从addr地址的数据中取lenght字节长度，存储至buffer内。
 * Input：       uint8_t *buffer, uint32_t addr, uint32_t length
 * Output：      无。
 * Return:       0-成功。
 * Others:       无。
 */
uint8_t SF_ReadData(uint8_t* buffer, uint32_t addr, uint32_t length)
{
    memcpy(buffer, (void*)addr, length);
    return FPSENSOR_OK;
}

// SF_Init
// 申请块连续的存储空间（即获取Flash的相关信息），包含起始地址和大小，起始地址使用fpsensor_platfoem.h的
// FINGER_ADDR_START，大小我们来定目前我们使用的是200k FINGER_ADDR_START FINGER_ID_LIST_START
// TEMPLATE_ADDR_START
// 这三个宏，我们操作FLash使用。
// SF_WriteData
// 用途：我们把自学习的模板更新到Flash，或者注册指纹时把数据存到Flash
// SF_ReadData
// 用途：我们从Flash里读我们存储的模板信息，进行比对
// 对于Flash空间，使用Flash还是内存映射还是安全芯片。。。等等，你们来定。