#include "fpsensor_common.h"

extern uint32_t g_u32FPStatusThres;
extern uint32_t g_u16FingerArea;

uint8_t fpsensor_print_HWID(void)
{
    uint8_t status = 0;
    uint8_t buffer[3] = {0};
    buffer[0] = FPSENSOR_REG_HWID;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    status = fpsensor_spi_transceive(buffer, 3);
    return status;
}

uint8_t fpsensor_get_HWID(uint8_t hwid[2])
{
    uint8_t status = 0;
    uint8_t buffer[3] = {0};
    buffer[0] = FPSENSOR_REG_HWID;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    status = fpsensor_spi_transceive(buffer, 3);
    memcpy(hwid, buffer + 1, 2);
    return status;
}

uint8_t fpsensor_print_VID(void)
{
    uint8_t status = 0;
    uint8_t buffer[4] = {0};
    buffer[0] = FPSENSOR_REG_VID;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    buffer[3] = 0x00;
    status = fpsensor_spi_transceive(buffer, 4);
    LOG("Fpsensor VID  = %02x %02x\n", buffer[3], buffer[2]);

#if defined(FPSENSOR_TYPE_7152)
    if ( buffer[2] > 0x14 )
    {
        buffer[0] = FPSENSOR_REG_OSC_TRIM;
        buffer[1] = 0x00;
        buffer[2] = 0x06;
        buffer[3] = 0x0a;
        status |= fpsensor_spi_transceive(buffer, 4);
    }
#endif
    return status;
}

uint8_t fpsensor_set_adc(fpsensor_adc_t* adc)
{
    uint8_t status = 0;
    uint8_t buffer[4] = {0};
    if ( adc == NULL )
    {
        return FPSENSOR_BUFFER_ERROR;
    }

    buffer[0] = FPSENSOR_REG_PXL_CTRL;
    buffer[1] = 0x0f;
    buffer[2] = adc->pixel;
    status |= fpsensor_spi_transceive(buffer, 3);

    buffer[0] = FPSENSOR_REG_ADC_SHIFT_GAIN;
    buffer[1] = adc->shift;
    buffer[2] = adc->gain;
    status |= fpsensor_spi_transceive(buffer, 3);

    if ( FPSENSOR_OK != status )
    {
        LOG("Fpsensor config adc failed.\n");
        return FPSENSOR_SPI_ERROR;
    }

    return FPSENSOR_OK;
}

uint8_t fpsensor_set_capture_crop(
    uint8_t* buffer, uint32_t length, uint32_t rowStart, uint32_t rowCount, uint32_t colStart,
    uint32_t colGroup
)
{
    if ( buffer == NULL || length < 5 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }

    buffer[0] = FPSENSOR_REG_IMG_CAPT_SIZE;
    buffer[1] = rowStart;
    buffer[2] = rowCount;
    buffer[3] = colStart;
    buffer[4] = colGroup;

    return fpsensor_spi_transceive(buffer, 5);
}

uint8_t fpsensor_capture_image()
{
    uint8_t buffer[1] = {FPSENSOR_CMD_CAPTURE_IMAGE};
    return fpsensor_spi_transceive(buffer, 1);
}

uint8_t fpsensor_get_img_data(uint8_t* buffer, uint32_t length)
{
    if ( buffer == NULL || length < 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
    memset(buffer, 0, length);
    buffer[0] = FPSENSOR_CMD_READ_IMAGE;
    buffer[1] = 0x00;
    return fpsensor_spi_transceive(buffer, length);
}

uint8_t fpsensor_soft_reset(void)
{
    uint8_t buffer[1] = {FPSENSOR_CMD_SOFT_RESET};
    uint8_t status = fpsensor_spi_transceive(buffer, 1);
    fpsensor_spi_set_cs(0);
    fpsensor_delay_ms(1);
    fpsensor_spi_set_cs(1);
    return status;
}

uint8_t fpsensor_finger_present_status(uint8_t* buffer, uint32_t length)
{
    uint8_t status = 0;

    if ( buffer == NULL || length < 3 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }

    buffer[0] = FPSENSOR_REG_FNGR_DET_THRES;
    buffer[1] = g_u32FPStatusThres;
    status |= fpsensor_spi_transceive(buffer, 2);

    buffer[0] = FPSENSOR_CMD_FINGER_PRESENT_QUERY;
    status |= fpsensor_spi_transceive(buffer, 1);

    fpsensor_delay_ms(1);
    buffer[0] = FPSENSOR_REG_FINGER_PRESENT_STATUS;
    buffer[1] = 0x00;
    buffer[2] = 0x00;
    status |= fpsensor_spi_transceive(buffer, 3);

    return status;
}

uint8_t fpsensor_fpc_status(uint8_t* buffer, uint32_t length)
{
    uint8_t status = 0;

    if ( buffer == NULL || length < 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
    buffer[0] = FPSENSOR_REG_FPC_STATUS;
    buffer[1] = 0x00;

    status = fpsensor_spi_transceive(buffer, 2);
    //	LOG("fpsensor fpc status %02x. \n", buffer[1]);
    return status;
}

uint8_t fpsensor_set_inverse(uint8_t* buffer, uint32_t length, fpsensor_invert_color_t color)
{
    uint8_t status = 0;
    if ( buffer == NULL || length < 2 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
    buffer[0] = FPSENSOR_REG_IMAGE_SETUP;
    buffer[1] = (uint8_t)color;
    status = fpsensor_spi_transceive(buffer, 2);

    return status;
}

uint8_t fpsensor_set_config_param(uint32_t u32FingerStatusThres, uint16_t u16FingerArea)
{
    if ( u32FingerStatusThres == 0 || u16FingerArea == 0 )
    {
        return FPSENSOR_BUFFER_ERROR;
    }
    g_u32FPStatusThres = u32FingerStatusThres;
    g_u16FingerArea = u16FingerArea;

    return FPSENSOR_OK;
}
