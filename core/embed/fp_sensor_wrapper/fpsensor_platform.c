#include "fpsensor_platform.h"
#include "fpsensor_driver.h"
#include "irq.h"
#include "systick.h"

SPI_HandleTypeDef spi_fp;
static bool fp_touched = false;
static uint8_t* fp_data_cache = (uint8_t*)0x38000000;
static bool fp_data_sync = false;

typedef struct
{
    bool list_data_valid;
    uint8_t template_data_valid[MAX_FINGERPRINT_COUNT];
} fpsensor_data_cache;

static fpsensor_data_cache fpsensor_cache = {0};

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

    // MISC

    // SPI_INT     FP_IRQ      PB15
    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // RSTn        FP_RST      PB14
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    NVIC_SetPriority(EXTI15_10_IRQn, IRQ_PRI_GPIO);

    return FPSENSOR_OK;
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
}

void fpsensor_state_set(bool state)
{
    fp_touched = state;
}

void fpsensor_irq_enable(void)
{
    fp_touched = false;
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void fpsensor_data_cache_clear(void)
{
    memset(fp_data_cache, 0, FINGER_DATA_TOTAL_SIZE);
    fpsensor_cache.list_data_valid = false;
    memset(fpsensor_cache.template_data_valid, 0, MAX_FINGERPRINT_COUNT);
}

void fpsensor_irq_disable(void)
{
    fp_touched = false;
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
}

int fpsensor_detect(void)
{
    if ( fp_touched )
    {
        fp_touched = false;
        return 1;
    }
    return 0;
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
    dwt_delay_ms(Timeout);
}

#include "secbool.h"

extern secbool se_fp_write(uint16_t offset, const void* val_dest, uint16_t len, uint8_t index, uint8_t total);
extern secbool se_fp_read(uint16_t offset, void* val_dest, uint16_t len, uint8_t index, uint8_t total);

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
    (void)ucMmSize;
    // fp_data_address = startAddr;
    memset(fp_data_cache, 0, FINGER_DATA_TOTAL_SIZE);
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
uint8_t SF_WriteData(uint8_t* buffer, uint32_t offset, uint32_t length)
{
    // memcpy((void*)(offset + fp_data_address), buffer, length);
    // se_fp_write(offset , buffer, length);
    memcpy((void*)(offset + fp_data_cache), buffer, length);
    if ( offset == FINGER_ID_LIST_OFFSET )
    {
        fpsensor_cache.list_data_valid = true;
    }
    else
    {
        for ( uint8_t i = 0; i < MAX_FINGERPRINT_COUNT; i++ )
        {
            if ( offset == TEMPLATE_ADDR_OFFSET + i * TEMPLATE_LENGTH )
            {
                fpsensor_cache.template_data_valid[i] = true;
            }
        }
    }
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
uint8_t SF_ReadData(uint8_t* buffer, uint32_t offset, uint32_t length)
{
    memcpy(buffer, (void*)(offset + fp_data_cache), length);
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

void fpsensor_data_set_sync(bool sync)
{
    fp_data_sync = sync;
}

static uint32_t reflect(uint32_t ref, char ch)
{
    uint32_t value = 0;

    for ( int i = 1; i < (ch + 1); i++ )
    {
        if ( ref & 1 )
            value |= 1 << (ch - i);
        ref >>= 1;
    }

    return value;
}

static uint32_t fp_crc32(uint8_t* buf, uint32_t len)
{
    uint32_t result = 0xFFFFFFFF;
    uint32_t m_Table[256];

    uint32_t ulPolynomial = 0x04C11DB7;

    for ( int i = 0; i <= 0xFF; i++ )
    {
        m_Table[i] = reflect(i, 8) << 24;
        for ( int j = 0; j < 8; j++ )
            m_Table[i] = (m_Table[i] << 1) ^ (m_Table[i] & (1 << 31) ? ulPolynomial : 0);
        m_Table[i] = reflect(m_Table[i], 32);
    }

    while ( len-- )
        result = (result >> 8) ^ m_Table[(result & 0xFF) ^ *buf++];

    result ^= 0xFFFFFFFF;

    return result;
}

bool fpsensor_data_init(void)
{
    static bool data_inited = false;
    uint32_t crc;

    if ( data_inited )
    {
        return true;
    }

    uint8_t* p_data;
    uint8_t counter = 0;
    uint8_t list[MAX_FINGERPRINT_COUNT] = {0};

    for ( uint8_t i = 0; i < MAX_FINGERPRINT_COUNT; i++ )
    {
        p_data = fp_data_cache + TEMPLATE_ADDR_OFFSET + i * TEMPLATE_LENGTH;
        ensure(
            se_fp_read(TEMPLATE_ADDR_OFFSET + i * TEMPLATE_TOTAL_LENGTH, p_data, 2, 0, 0), "se_fp_read failed"
        );
        if ( p_data[0] == 0x32 && p_data[1] == 0x34 )
        {
            list[counter++] = i;
        }
    }

    for ( uint8_t i = 0; i < counter; i++ )
    {
        p_data = fp_data_cache + TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_LENGTH;
        ensure(
            se_fp_read(
                TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH, p_data, TEMPLATE_LENGTH, i, counter
            ),
            "se_fp_read failed"
        );
        ensure(
            se_fp_read(
                TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH + TEMPLATE_LENGTH, (uint8_t*)&crc,
                TEMPLATE_DATA_CRC_LEN, 0, 0
            ),
            "se_fp_read failed"
        );
        if ( crc != fp_crc32(p_data, TEMPLATE_LENGTH) )
        {
            memset(p_data, 0, TEMPLATE_LENGTH);
            se_fp_write(TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH, "\xff\xff\xff\xff", 4, 0, 0);
        }
    }
    data_inited = true;
    return true;
}

bool fpsensor_data_save(bool update_all, uint8_t id)
{
    uint8_t* p_data;
    uint32_t crc;
    uint8_t list[MAX_FINGERPRINT_COUNT] = {0};
    uint8_t counter = 0;
    if ( update_all )
    {
        for ( uint8_t i = 0; i < MAX_FINGERPRINT_COUNT; i++ )
        {
            if ( fpsensor_cache.template_data_valid[i] )
            {
                list[counter++] = i;
            }
        }
    }
    else
    {
        if ( fpsensor_cache.template_data_valid[id] )
        {
            list[0] = id;
            counter = 1;
        }
        else
        {
            return false;
        }
    }

    for ( uint8_t i = 0; i < counter; i++ )
    {
        p_data = fp_data_cache + TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_LENGTH;
        crc = fp_crc32(p_data, TEMPLATE_LENGTH);

        ensure(
            se_fp_write(
                TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH, p_data, TEMPLATE_LENGTH, i, counter
            ),
            "se_fp_write failed"
        );
        ensure(
            se_fp_write(
                TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH + TEMPLATE_LENGTH, (uint8_t*)&crc,
                TEMPLATE_DATA_CRC_LEN, 0, 0
            ),
            "se_fp_write failed"
        );
    }
    return true;
}

bool fpsensor_data_delete(bool all, uint8_t id)
{
    uint8_t* p_data;
    uint8_t list[MAX_FINGERPRINT_COUNT] = {0};
    uint8_t counter = 0;
    if ( all )
    {
        for ( uint8_t i = 0; i < MAX_FINGERPRINT_COUNT; i++ )
        {
            if ( fpsensor_cache.template_data_valid[i] )
            {
                list[counter++] = i;
            }
        }
    }
    else
    {
        if ( fpsensor_cache.template_data_valid[id] )
        {
            list[0] = id;
            counter = 1;
        }
        else
        {
            return false;
        }
    }

    for ( uint8_t i = 0; i < counter; i++ )
    {
        p_data = fp_data_cache + TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_LENGTH;
        memset(p_data, 0, TEMPLATE_LENGTH);

        ensure(
            se_fp_write(TEMPLATE_ADDR_OFFSET + list[i] * TEMPLATE_TOTAL_LENGTH, p_data, 4, 0, 0),
            "se_fp_write failed"
        );
    }
    return true;
}
