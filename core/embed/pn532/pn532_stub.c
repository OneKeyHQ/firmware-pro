#include STM32_HAL_H

#include "pn532_stub.h"

static void pn532_stub_init(void)
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
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = 0; // ignored
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, GPIO_PIN_RESET);
}

static void pn532_stub_reset_ctl(bool enable)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_5, enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

pn532_stub_t pn532_stub_st = {
    .init = pn532_stub_init,
    .chip_reset_ctl = pn532_stub_reset_ctl,
};

pn532_stub_t* get_stub_controller(void)
{
    return &pn532_stub_st;
}
