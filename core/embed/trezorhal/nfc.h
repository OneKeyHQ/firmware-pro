#ifndef __NFC_H__
#define __NFC_H__

#include "spi.h"
#include "pn532.h"

#define spi_handle_nfc spi_handles[spi_find_channel_by_device(SPI_NFC)]

// low level
void nfc_reset(void);
void nfc_init(void);

// library
extern PN532 nfc_pn532;
void PN532_LibrarySetup(void);

// api

#endif // __NFC_H__