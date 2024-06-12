#include "pn532_spi.h"

static SPI_HandleTypeDef* spi_handle_nfc;

void pn532_spi_init(void)
{
    spi_init_by_device(SPI_NFC);
    spi_handle_nfc = &spi_handles[spi_find_channel_by_device(SPI_NFC)];
}

void pn532_spi_deinit(void)
{
    spi_deinit_by_device(SPI_NFC);
}

void pn532_spi_write(uint8_t* buf, uint32_t size)
{
    HAL_SPI_Transmit(spi_handle_nfc, buf, size, 1000);
}

void pn532_spi_read(uint8_t* buf, uint32_t size)
{
    HAL_SPI_Receive(spi_handle_nfc, buf, size, 1000);
}

void pn532_spi_chip_select(bool enable)
{
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, enable ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static pn532_spi_t pn532_spi_st = {
    .spi_init = pn532_spi_init,
    .spi_deinit = pn532_spi_deinit,
    .chip_select = pn532_spi_chip_select,
    .write = pn532_spi_write,
    .read = pn532_spi_read,
};

pn532_spi_t* get_spi_controller(void)
{
    return &pn532_spi_st;
}
