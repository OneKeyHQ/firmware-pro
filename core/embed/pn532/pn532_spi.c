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

// Whole-buffer transfers. The PN532's SPI slave is paced by slowing SCK to
// ~1.5 MHz (SPI6 prescaler 64, see trezorhal/spi.c) rather than by per-byte
// HAL-call overhead, since an unpaced burst at 3.125 MHz corrupts frames longer
// than ~200 bytes. Must stay paired with the exact-length frame read in pn532.c
// (pn532_read_frame_exact): an over-read of the ACK would swallow the
// response frame that becomes ready meanwhile.
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
