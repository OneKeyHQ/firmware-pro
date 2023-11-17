#ifndef __NFC_H__
#define __NFC_H__

#include "pn532.h"
#include "spi.h"

#define spi_handle_nfc spi_handles[spi_find_channel_by_device(SPI_NFC)]
extern PN532* pn532;

void nfc_init(void);

// not really needed since we could use the handle directly
// int nfc_selftest(void);
// bool nfc_send_recv_apdu(uint8_t* send, uint8_t sendLength, uint8_t* response,
// uint8_t* responseLength);

#endif // __NFC_H__