
#include <display.h>
#include <nfc.h>

static const uint32_t NFC_SPI_TIMEOUT = 0xffff;

static PN532_INTERFACE _pn532_interface; // the actuall handle

static void nfc_lowlevel_gpio_init(void)
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

static void nfc_lowlevel_reset_ctl(bool ctl)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, (ctl ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

static void nfc_lowlevel_chip_sel_ctl(bool ctl)
{
    HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, (ctl ? GPIO_PIN_SET : GPIO_PIN_RESET));
}

static bool nfc_lowlevel_irq_get()
{
    return HAL_GPIO_ReadPin(GPIOJ, GPIO_PIN_4) == GPIO_PIN_SET;
}

static bool nfc_lowlevel_spi_rw(uint8_t* w_data, uint16_t w_count, uint8_t* r_data, uint16_t r_count)
{
    bool result = true;

    nfc_lowlevel_chip_sel_ctl(false);
    HAL_Delay(1);

    if ( w_data != NULL )
        if ( HAL_SPI_Transmit(&spi_handle_nfc, w_data, w_count, NFC_SPI_TIMEOUT) != HAL_OK )
            result = false;
    if ( r_data != NULL )
        if ( HAL_SPI_Receive(&spi_handle_nfc, r_data, r_count, NFC_SPI_TIMEOUT) != HAL_OK )
        {
            if ( spi_handle_nfc.ErrorCode != HAL_SPI_ERROR_TIMEOUT )
            {
                result = false;
            }
        }

    HAL_Delay(1);
    nfc_lowlevel_chip_sel_ctl(true);

    return result;
}

static void nfc_lowlevel_delay_ms(uint16_t timeout)
{
    HAL_Delay(timeout);
}

// use display_printf instead!
// static void nfc_lowlevel_log(const char *fmt, ...)
// {
//     va_list va;
//     va_start(va, fmt);
//     char buf[256] = {0};
//     int len = vsnprintf(buf, sizeof(buf), fmt, va);
//     display_print(buf, len);
//     va_end(va);
// }

static bool nfc_lowlevel_init()
{
    return spi_init_by_device(SPI_NFC);
}

static bool nfc_lowlevel_deinit()
{
    return spi_deinit_by_device(SPI_NFC);
}

void nfc_init(void)
{
    nfc_lowlevel_gpio_init();

    _pn532_interface.bus_init = nfc_lowlevel_init;
    _pn532_interface.bus_deinit = nfc_lowlevel_deinit;
    _pn532_interface.reset_ctl = nfc_lowlevel_reset_ctl;
    _pn532_interface.chip_sel_ctl = nfc_lowlevel_chip_sel_ctl;
    _pn532_interface.irq_read = nfc_lowlevel_irq_get;
    _pn532_interface.rw = nfc_lowlevel_spi_rw;
    _pn532_interface.delay_ms = nfc_lowlevel_delay_ms;
    _pn532_interface.log = display_printf;

    if ( !PN532_InterfaceSetup(&_pn532_interface) )
    {
        // this should never happen as long as above filled all members of
        // pn532_interface
        display_printf("PN532_InterfaceSetup failed!");
        return;
    }

    PN532_LibrarySetup();
}