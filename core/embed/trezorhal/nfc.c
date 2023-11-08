
#include <nfc.h>

static const uint32_t NFC_SPI_TIMEOUT = 10;

static void nfc_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();

    // IRQ    NFC_IRQ      PC4
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // RSTPDn    NFC_RST      PD5
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void nfc_reset(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_Delay(50);
}

static void nfc_chip_sel(bool sel)
{
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, (sel ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

void nfc_init(void)
{
    spi_init_by_device(SPI_NFC);

    nfc_gpio_init();
    nfc_reset();
}

static int nfc_spi_rw(uint8_t* data, uint8_t count)
{
    char ret = PN532_STATUS_OK;
    nfc_chip_sel(false);
    HAL_Delay(1);
    if ( HAL_SPI_TransmitReceive(&spi_handle_nfc, data, data, count, NFC_SPI_TIMEOUT) != HAL_OK )
        ret = PN532_STATUS_ERROR;
    HAL_Delay(1);
    nfc_chip_sel(true);
    return ret;
}

// library
PN532 nfc_pn532;

static const uint8_t _SPI_STATREAD = 0x02;
static const uint8_t _SPI_DATAWRITE = 0x01;
static const uint8_t _SPI_DATAREAD = 0x03;
static const uint8_t _SPI_READY = 0x01;

int PN532_Reset(void)
{
    nfc_reset();
    return PN532_STATUS_OK;
}

int PN532_SPI_ReadData(uint8_t* data, uint16_t count)
{
    int ret = PN532_STATUS_OK;
    uint8_t frame[count + 1];
    frame[0] = _SPI_DATAREAD;
    HAL_Delay(5);
    ret = nfc_spi_rw(frame, count + 1);
    for ( uint8_t i = 0; i < count; i++ )
    {
        data[i] = frame[i + 1];
    }
    return ret;
}

int PN532_SPI_WriteData(uint8_t* data, uint16_t count)
{
    int ret = PN532_STATUS_OK;
    uint8_t frame[count + 1];
    frame[0] = _SPI_DATAWRITE;
    for ( uint8_t i = 0; i < count; i++ )
    {
        frame[i + 1] = data[i];
    }
    ret = nfc_spi_rw(frame, count + 1);
    return ret;
}

bool PN532_SPI_WaitReady(uint32_t timeout)
{
    uint8_t status[] = {_SPI_STATREAD, 0x00};
    uint32_t tickstart = HAL_GetTick();
    while ( HAL_GetTick() - tickstart < timeout )
    {
        HAL_Delay(10);
        nfc_spi_rw(status, sizeof(status));
        if ( status[1] == _SPI_READY )
        {
            return true;
        }
        else
        {
            HAL_Delay(5);
        }
    }
    return false;
}

int PN532_SPI_Wakeup(void)
{
    // Send any special commands/data to wake up PN532
    uint8_t data[] = {0x00};
    nfc_spi_rw(data, 1);
    return PN532_STATUS_OK;
}

void PN532_Log(const char* log)
{
    // display_printf("%s\n", log);
    return;
}

void PN532_LibrarySetup(void)
{
    nfc_pn532.reset = PN532_Reset;
    nfc_pn532.read_data = PN532_SPI_ReadData;
    nfc_pn532.write_data = PN532_SPI_WriteData;
    nfc_pn532.wait_ready = PN532_SPI_WaitReady;
    nfc_pn532.wakeup = PN532_SPI_Wakeup;
    nfc_pn532.log = PN532_Log;
}