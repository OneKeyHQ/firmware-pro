#ifndef __NFC_H__
#define __NFC_H__

#include STM32_HAL_H
#include "pn532.h"
#include "display.h"

extern PN532 pn532;
extern SPI_HandleTypeDef hspi6;

#define SET_NFC_SPI_CS_L HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, GPIO_PIN_RESET)
#define SET_NFC_SPI_CS_H HAL_GPIO_WritePin(GPIOJ, GPIO_PIN_4, GPIO_PIN_SET)

#define _SPI_STATREAD                   0x02
#define _SPI_DATAWRITE                  0x01
#define _SPI_DATAREAD                   0x03
#define _SPI_READY                      0x01

#define _SPI_TIMEOUT                    10

char nfc_init(void);

#endif