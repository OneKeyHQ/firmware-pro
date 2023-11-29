#include <string.h>

#include "camera.h"
#include "i2c.h"
#include "sdram.h"
#include "common.h"
#include "display.h"
#include "mipi_lcd.h"
#include "quirc.h"
#include "irq.h"

static struct quirc* qr_decoder;

static I2C_HandleTypeDef* i2c_handle_camera = NULL;
static volatile bool capture_done = false;

DCMI_HandleTypeDef DCMI_Handle;
DMA_HandleTypeDef DMA_DCMI_Handle;

static void dcmi_init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**DCMI GPIO Configuration
    PG10     ------> DCMI_D2
    PG9     ------> DCMI_VSYNC
    PG11     ------> DCMI_D3
    PD3     ------> DCMI_D5
    PE5     ------> DCMI_D6
    PE4     ------> DCMI_D4
    PA10     ------> DCMI_D1
    PA9     ------> DCMI_D0
    PE6     ------> DCMI_D7
    PA6     ------> DCMI_PIXCLK
    PA4     ------> DCMI_HSYNC
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_9 | GPIO_PIN_6 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF13_DCMI;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_RCC_DCMI_CLK_ENABLE();
    DCMI_Handle.Instance = DCMI;
    DCMI_Handle.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
    DCMI_Handle.Init.PCKPolarity = DCMI_PCKPOLARITY_RISING;
    DCMI_Handle.Init.VSPolarity = DCMI_VSPOLARITY_LOW;
    DCMI_Handle.Init.HSPolarity = DCMI_HSPOLARITY_LOW;
    DCMI_Handle.Init.CaptureRate = DCMI_CR_ALL_FRAME;
    DCMI_Handle.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    DCMI_Handle.Init.JPEGMode = DCMI_JPEG_DISABLE;
    DCMI_Handle.Init.ByteSelectMode = DCMI_BSM_ALL;
    DCMI_Handle.Init.ByteSelectStart = DCMI_OEBS_ODD;
    DCMI_Handle.Init.LineSelectMode = DCMI_LSM_ALL;
    DCMI_Handle.Init.LineSelectStart = DCMI_OELS_ODD;

    NVIC_SetPriority(DCMI_IRQn, IRQ_PRI_DCMI);
    HAL_NVIC_EnableIRQ(DCMI_IRQn);
    HAL_DCMI_Init(&DCMI_Handle);

    __HAL_RCC_DMA2_CLK_ENABLE();
    DMA_DCMI_Handle.Instance = DMA2_Stream1;
    DMA_DCMI_Handle.Init.Request = DMA_REQUEST_DCMI;
    DMA_DCMI_Handle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    DMA_DCMI_Handle.Init.PeriphInc = DMA_PINC_DISABLE;
    DMA_DCMI_Handle.Init.MemInc = DMA_MINC_ENABLE;
    DMA_DCMI_Handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    DMA_DCMI_Handle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    DMA_DCMI_Handle.Init.Mode = DMA_CIRCULAR;
    DMA_DCMI_Handle.Init.Priority = DMA_PRIORITY_HIGH;
    DMA_DCMI_Handle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    DMA_DCMI_Handle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    DMA_DCMI_Handle.Init.MemBurst = DMA_MBURST_SINGLE;
    DMA_DCMI_Handle.Init.PeriphBurst = DMA_PBURST_SINGLE;

    NVIC_SetPriority(DCMI_IRQn, IRQ_PRI_DCMI_DMA);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    HAL_DMA_Init(&DMA_DCMI_Handle);

    __HAL_LINKDMA(&DCMI_Handle, DMA_Handle, DMA_DCMI_Handle);
}

HAL_DCMI_StateTypeDef dcmi_get_state()
{
    return HAL_DCMI_GetState(&DCMI_Handle);
}

uint32_t dcmi_get_error()
{
    return HAL_DCMI_GetError(&DCMI_Handle);
}

/*
 * DMA2_Stream1_IRQHandler
 */
void DMA2_Stream1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&DMA_DCMI_Handle);
}

/*
 * DCMI_IRQHandler
 */
void DCMI_IRQHandler(void)
{
    HAL_DCMI_IRQHandler(&DCMI_Handle);
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef* hdcmi)
{
    // camera_suspend();
    capture_done = true;
}

void camera_io_init()
{
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLDOWN;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStructure.Pin = GPIO_PIN_11;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.Pin = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStructure);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_6, GPIO_PIN_SET);
    HAL_Delay(20);
}

unsigned char camera_sccb_read_reg(unsigned char reg_addr, unsigned char* data)
{
    unsigned char ret = 0;
    if ( HAL_I2C_Mem_Read(i2c_handle_camera, GC2145_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, 1, 1000) !=
         HAL_OK )
        ret = 1;
    return ret;
}

unsigned char camera_sccb_write_reg(unsigned char reg_addr, unsigned char* data)
{
    unsigned char ret = 0;
    if ( HAL_I2C_Mem_Write(i2c_handle_camera, GC2145_ADDR, reg_addr, I2C_MEMADD_SIZE_8BIT, data, 1, 1000) !=
         HAL_OK )
        ret = 1;
    return ret;
}

unsigned short camera_get_id(void)
{
    union ID
    {
        unsigned short u16;
        unsigned char u8[2];
    } myid;
    myid.u16 = 0;

    if ( camera_sccb_read_reg(0xF0, &myid.u8[1]) )
        return myid.u16;
    if ( camera_sccb_read_reg(0xF1, &myid.u8[0]) )
        return myid.u16;

    return myid.u16;
}

unsigned char camera_is_online(void)
{
    unsigned char ret = 0;
    unsigned char read = 0;

    if ( camera_sccb_read_reg(0xFB, &read) )
        return ret;

    ret = (read == GC2145_ADDR) ? 1 : 0;

    return ret;
}

void camera_start(uint8_t* buffer_address, uint32_t mode)
{
    HAL_DCMI_Start_DMA(&DCMI_Handle, mode, (uint32_t)buffer_address, (WIN_W * WIN_H) / 2);
}

void camera_stop(void)
{
    HAL_DCMI_Stop(&DCMI_Handle);
    capture_done = false;
}

void camera_suspend(void)
{
    HAL_DCMI_Suspend(&DCMI_Handle);
}

void camera_resume(void)
{
    HAL_DCMI_Resume(&DCMI_Handle);
}

int camera_init(void)
{
    i2c_handle_camera = &i2c_handles[i2c_find_channel_by_device(I2C_CAMERA)];
    i2c_init_by_device(I2C_CAMERA);

    camera_get_id();

    volatile unsigned char ret = 0;
    for ( int i = 0; default_regs[i][0]; i++ )
    {
        unsigned char temp = default_regs[i][1];
        ret = camera_sccb_write_reg(default_regs[i][0], &temp);
        if ( ret )
        {
            ensure_ex(ret, 0, "camera init failed");
        }
    }

    dcmi_init();

    return ret;
}

uint8_t rgb565_to_gray(uint16_t rgb565)
{
    // Extract R, G, B values from RGB565
    int r = ((rgb565 >> 11) & 0x1F) << 3;
    int g = ((rgb565 >> 5) & 0x3F) << 2;
    int b = (rgb565 & 0x1F) << 3;

    // Convert RGB to grayscale using fast integer approximation
    int gray_value = (r * 76 + g * 150 + b * 29) >> 8;

    return gray_value & 0xff;
}

int quirc_decode_buffer(uint8_t* buffer, uint8_t* out, uint32_t len)
{
    uint8_t* image_data;
    struct quirc_code code;
    struct quirc_data data = {0};
    uint16_t* rgb565_value = (uint16_t*)buffer;

    qr_decoder = quirc_new();

    if ( quirc_resize(qr_decoder, WIN_W, WIN_H) < 0 )
    {
        goto fail;
    }
    image_data = quirc_begin(qr_decoder, NULL, NULL);
    for ( int i = 0; i < WIN_W * WIN_H; i++ )
    {
        image_data[i] = rgb565_to_gray(rgb565_value[i]);
    }
    quirc_end(qr_decoder);

    int num_codes = quirc_count(qr_decoder);
    if ( num_codes != 1 )
    {
        goto fail;
    }
    quirc_extract(qr_decoder, 0, &code);
    quirc_decode_error_t err = quirc_decode(&code, &data);
    if ( err == QUIRC_ERROR_DATA_ECC )
    {
        quirc_flip(&code);
        err = quirc_decode(&code, &data);
    }
    if ( err )
    {
        goto fail;
    }
fail:
    quirc_destroy(qr_decoder);
    if ( len >= data.payload_len )
    {
        memcpy(out, data.payload, data.payload_len);
    }

    return data.payload_len;
}

int camera_qr_decode(uint32_t x, uint32_t y, uint8_t* data, uint32_t data_len)
{

    int len = 0;

    camera_start((uint8_t*)CAM_BUF_ADDRESS, DCMI_MODE_SNAPSHOT);

    int32_t tickstart = HAL_GetTick();

    while ( !capture_done )
    {
        if ( (HAL_GetTick() - tickstart) > 200 )
        {
            camera_stop();
            return 0;
        }
    }
    camera_stop();
    dma2d_copy_buffer(
        (uint32_t*)CAM_BUF_ADDRESS, (uint32_t*)FMC_SDRAM_LTDC_BUFFER_ADDRESS, x, y, WIN_W, WIN_H
    );
    len = quirc_decode_buffer((uint8_t*)CAM_BUF_ADDRESS, data, data_len);
    return len;
}

void camera_test(void)
{
    uint8_t qr_code[256];
    int len;

    display_clear();

    while ( 1 )
    {
        len = camera_qr_decode(80, 80, qr_code, sizeof(qr_code));
        if ( len )
        {
            display_bar(0, 330, 480, 50, COLOR_BLACK);
            display_text(0, 430, (char*)qr_code, len, FONT_NORMAL, COLOR_WHITE, COLOR_BLACK);
        }
    }
}
