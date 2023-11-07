/*
 * Copyright (c) 2018, Beijing Chipone System Technology Co.,Ltd.
 * All rights reserved.
 *
 * File Name: fpsenor_driver.c
 * Description: sensor 驱动实现
 *
 * Version: 1.1 , czhang 2020-05-14
 *           // 修改ADC初值 不再和采图用同一值 解决不接地无法检测到手指问题
 *           // 固定采图ADC值为原测试过的值
 *
 * History: 1.0, czhang 2020-03-04
 *           a) 实现基本功能
 */

#include "fpsensor_driver.h"
#include "fpsensor_common.h"

// 超时时间
int16_t g_i16ImageTimeout = 10;

// ADC设置
fpsensor_adc_t adc_fixed;

// 按压阈值
uint32_t g_u32FPStatusThres = 0xc0;

// 按压有效面积
uint32_t g_u16FingerArea = 8;

/*
 * Function:     fpsensor_init
 * Description:  初始化寄存器
 * Input:        无
 * Output:       无
 * Return:       无
 * Others:       先复位 后写寄存器
 */
uint8_t fpsensor_init(void)
{
    uint8_t status = 0;
    uint8_t buffer[16] = {0};

    // status |= fpsensor_hard_reset(); // 开发板使用
    status |= fpsensor_read_irq_with_clear(buffer, sizeof(buffer));
    
    buffer[0] = FPSENSOR_REG_FINGER_DRIVE_CONF;
    buffer[1] = 0x12;
    status |= fpsensor_spi_transceive(buffer, 2);

    buffer[0] = FPSENSOR_REG_ADC_SHIFT_GAIN;
    buffer[1] = 0x10;
    buffer[2] = 0x0a;
    status |= fpsensor_spi_transceive(buffer, 3);

    buffer[0] = FPSENSOR_REG_PXL_CTRL;
    buffer[1] = 0x0f;
    buffer[2] = 0x14;
    status |= fpsensor_spi_transceive(buffer, 3);

    buffer[0] = FPSENSOR_REG_IMAGE_SETUP;
    buffer[1] = 0x0b;
    status |= fpsensor_spi_transceive(buffer, 2);

    if ( FPSENSOR_OK != status )
    {
        LOG("Fpsensor init failed, err: 0x%02x\n", status);
        return FPSENSOR_SPI_ERROR;
    }

    return FPSENSOR_OK;
}

/*
 * Function:     fpsensor_set_test
 * Description:  切换到棋盘格的测试模式
 * Input:        forward 方向控制
 * Output:       无
 * Return:       0 成功 1失败
 * Others:       无
 */
uint8_t fpsensor_set_test(uint8_t forward)
{
    uint8_t buffer[4];
    uint8_t status = 0;

    // Set Finger drive connected to pixel test capacitor
    buffer[0] = FPSENSOR_REG_FINGER_DRIVE_CONF;
    buffer[1] = 0x14 | (1 << 2);
    status |= fpsensor_spi_transceive(buffer, 2);

    // Test pattern
    buffer[0] = FPSENSOR_REG_TST_COL_PATTERN_EN;
    buffer[1] = forward ? 0x55 : 0xAA;
    buffer[2] = forward ? 0xAA : 0x55;
    status |= fpsensor_spi_transceive(buffer, 3);

    return status;
}

/*
 * Function:     fpsensor_adc_init
 * Description:  初始化ADC参数 控制图像采集
 * Input:        shift：偏移
 *               gain： 增益
 *               pixel：像素
 * Output:       无
 * Return:       0 成功 1失败
 * Others:       无
 */
uint8_t fpsensor_adc_init(uint8_t shift, uint8_t gain, uint8_t pixel, uint8_t et1)
{
    // shift range 0-31, gain range 0-15, pixel range: 0 4 16 20
    adc_fixed.shift = shift > 31 ? 31 : shift;
    adc_fixed.gain = gain > 15 ? 15 : gain;
    adc_fixed.pixel = pixel < 2 ? 0 : pixel < 10 ? 4 : pixel < 18 ? 16 : 20;
    adc_fixed.et1 = et1;
#if defined(FPSENSOR_TYPE_FPC1021)
    adc_fixed.pixel = pixel;
#endif
    return 0;
}

/*
 * Function:     fpsensor_read_image
 * Description:  读取图像
 * Input:        buffer：图像数据空间
 *               length：数据长度
 *               timeout_seconds：超时秒
 * Output:       无
 * Return:       0 成功 1失败
 * Others:       无
 */
uint8_t fpsensor_read_image(uint8_t* buffer, uint32_t length, uint32_t timeout_seconds)
{
    uint8_t status = 1;
    uint8_t soft_irq = 0;

    fpsensor_adc_t adc = adc_fixed;

    if ( buffer == NULL || length < FPSENSOR_IMAGE_HEIGHT * FPSENSOR_IMAGE_WIDTH + 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
    g_i16ImageTimeout = timeout_seconds;

    if ( fpsensor_finger_down() )
    {
        status = fpsensor_run_cac(buffer, length, &adc);
        status |=
            fpsensor_set_capture_crop(buffer, length, 0, FPSENSOR_IMAGE_HEIGHT, 0, FPSENSOR_IMAGE_WIDTH);
        status |= fpsensor_set_adc(&adc);
        status |= fpsensor_capture_image();

        while ( !soft_irq )
        {
            status |= fpsensor_read_irq_with_clear(buffer, length);
            soft_irq = buffer[1] & FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA;
            fpsensor_delay_ms(1);
            if ( FPSENSOR_OK != status )
            {
                LOG("Fpsensor read image soft irq failed, err: 0x%02x\n", status);
            }
        }
        status |= fpsensor_get_img_data(buffer, FPSENSOR_IMAGE_HEIGHT * FPSENSOR_IMAGE_WIDTH + 2);

        memmove(buffer, buffer + 2, FPSENSOR_IMAGE_HEIGHT * FPSENSOR_IMAGE_WIDTH);
    }

    return status;
}

/*
 * Function:     fpsensor_finger_status_statistics
 * Description:  检测手指状态
 * Input:        无
 * Output:       无
 * Return:       0 成功 1失败
 * Others:       无
 */
uint8_t fpsensor_finger_status_statistics(void)
{
    uint8_t status = 0;
    uint8_t sum = 0;
    uint8_t i = 0;
    uint8_t buffer[10] = {0};
    fpsensor_adc_t adc = adc_fixed;

    status |= fpsensor_set_adc(&adc);
    status |= fpsensor_read_irq_with_clear(buffer, sizeof(buffer));
    fpsensor_delay_ms(1);
    status |= fpsensor_finger_present_status(buffer, sizeof(buffer));

    if ( FPSENSOR_OK != status )
    {
        LOG("Get finger present status failed, err: 0x%02x\n", status);
        return 0;
    }

    buffer[1] &= 0x0f;

    i = 0;
    while ( i < 4 )
    {
        sum += buffer[1] >> i++ & 1;
    }
    i = 0;
    while ( i < 8 )
    {
        sum += buffer[2] >> i++ & 1;
    }
    return sum;
}

/*
 * Function:     fpsensor_active_idle_mode
 * Description:  进入idle模式
 * Input:        无
 * Output:       无
 * Return:       切换模式是否成功
 * Others:       无
 */
uint8_t fpsensor_active_idle_mode(void)
{
    uint8_t buffer[1] = {FPSENSOR_CMD_ACTIVATE_IDLE_MODE};
    return fpsensor_spi_transceive(buffer, sizeof(buffer));
}

uint8_t fpsensor_finger_down(void)
{
    uint8_t sum = fpsensor_finger_status_statistics();
    fpsensor_delay_ms(1);
    return sum > g_u16FingerArea ? 1 : 0;
}

uint8_t fpsensor_finger_off(void)
{
    uint8_t sum1, sum2, sum3;
    sum1 = fpsensor_finger_status_statistics();
    fpsensor_delay_ms(1);
    sum2 = fpsensor_finger_status_statistics();
    fpsensor_delay_ms(1);
    sum3 = fpsensor_finger_status_statistics();
    fpsensor_delay_ms(1);
    if ( sum1 == 0 && sum2 == 0 && sum3 == 0 )
    {
        return 1;
    }
    return 0;
}

uint8_t fpsensor_active_sleep_mode(int32_t fngerDectPeriod)
{
    uint8_t status = 0;
    uint8_t period = 0;
    uint8_t buffer[4];

    if ( fngerDectPeriod <= 0 )
    {
        period = 0x00;
    }
    else if ( fngerDectPeriod >= (64 * 255 * 1000 / SLEEP_MODE_CLK) )
    {
        period = 0xFF;
    }
    else
    {
        period = (fngerDectPeriod * SLEEP_MODE_CLK / 1000) >> 6;
    }
    fpsensor_hard_reset();
    status |= fpsensor_set_adc(&adc_fixed);
    status |= fpsensor_read_irq_with_clear(buffer, sizeof(buffer));

    buffer[0] = FPSENSOR_REG_FNGR_DET_CNTR;
    buffer[1] = 0x00;
    buffer[2] = period;
    status |= fpsensor_spi_transceive(buffer, 3);

    buffer[0] = FPSENSOR_CMD_ACTIVATE_SLEEP_MODE;
    status |= fpsensor_spi_transceive(buffer, 1);

    return status;
}

uint8_t fpsensor_active_sleep_for_test(int32_t fngerDectPeriod)
{
    uint8_t status = 0;
    uint8_t period = 0;
    uint8_t buffer[16];

    if ( fngerDectPeriod <= 0 )
    {
        period = 0x00;
    }
    else if ( fngerDectPeriod >= (64 * 255 * 1000 / SLEEP_MODE_CLK) )
    {
        period = 0xFF;
    }
    else
    {
        period = (fngerDectPeriod * SLEEP_MODE_CLK / 1000) >> 6;
    }

    fpsensor_hard_reset();
    status |= fpsensor_read_irq_with_clear(buffer, 16);
    status |= fpsensor_set_adc(&adc_fixed);

    buffer[0] = FPSENSOR_REG_FNGR_DET_CNTR;
    buffer[1] = 0x00;
    buffer[2] = period;
    status |= fpsensor_spi_transceive(buffer, 3);

    buffer[0] = FPSENSOR_CMD_ACTIVATE_DEEP_SLEEP_MODE;
    status |= fpsensor_spi_transceive(buffer, 1);

    return status;
}
/*
 * Function:     fpsesor_read_testpattern
 * Description:  读取棋盘格图像数据
 * Input:        pu8bufimage：图像数据空间
 *               length：数据长度
 *               u8forward：方向
 * Output:       无
 * Return:       0 成功 1失败
 * Others:       无
 */

uint8_t fpsesor_read_testpattern(uint8_t* pu8bufimage, uint32_t length, uint8_t u8forward)
{
    uint8_t soft_irq = 0;
    uint8_t au8Buf[10] = {0};

    // shift gain pixel et1
    fpsensor_adc_t temp = {12, 12, 16, 3};

    fpsensor_init();
    fpsensor_set_test(u8forward);
    fpsensor_set_capture_crop(au8Buf, sizeof(au8Buf), 0, FPSENSOR_IMAGE_HEIGHT, 0, FPSENSOR_IMAGE_WIDTH);
    fpsensor_set_adc(&temp);
    fpsensor_capture_image();

    while ( !soft_irq )
    {
        fpsensor_delay_ms(1);
        fpsensor_read_irq_with_clear(au8Buf, sizeof(au8Buf));
        soft_irq = au8Buf[1] & FPSENSOR_IRQ_REG_BIT_FIFO_NEW_DATA;
    }
    fpsensor_get_img_data(pu8bufimage + 2, length + 2);
    memcpy(pu8bufimage, pu8bufimage + 4, length);

    return 0;
}

uint8_t fpsensor_read_irq_with_clear(uint8_t* buffer, uint32_t length)
{
    if ( buffer == NULL || length < 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }

    buffer[0] = FPSENSOR_REG_READ_INTERRUPT_WITH_CLEAR;
    buffer[1] = 0x00;
    return fpsensor_spi_transceive(buffer, 2);
}

uint8_t fpsensor_active_quary_finger_mode(void)
{
    uint8_t buffer[2];
    uint8_t status = 0;

    status |= fpsensor_set_adc(&adc_fixed);

    status |= fpsensor_read_irq_with_clear(buffer, sizeof(buffer));

    buffer[0] = FPSENSOR_CMD_WAIT_FOR_FINGER_PRESENT;
    status |= fpsensor_spi_transceive(buffer, 1);

    return status;
}

uint8_t fpsensor_read_irq_no_clear(uint8_t* buffer, uint32_t length)
{
    if ( buffer == NULL || length < 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }

    buffer[0] = FPSENSOR_REG_READ_INTERRUPT;
    buffer[1] = 0x00;
    return fpsensor_spi_transceive(buffer, 2);
}
